#version 330 core

#define MAX_LIGHTS 128

struct Light {
    vec4 position;      // xyz = Position, w = Light Type (0 = directional, 1 = point, 2 = spot, 3 = neon)
    vec4 color;         // xyz = Color, w = Specular Factor (Phong Legacy)
    vec4 direction;     // xyz = Direction, w = Ambient Factor (Phong legacy)
    vec4 attenuation;   // x = Constant, y = Linear, z = Quadratic, w = Diffuse Factor (Phong legacy)
    vec4 spotParams;    // x = Cos Inner, y = Cos Outer, z = Size, w = Intensity (PBR)
};

layout (std140) uniform LightBlock
{
    Light lights[MAX_LIGHTS];
    int lightCount;
};

layout (std140) uniform MaterialBlock
{
    vec4 ambient;           // xyz = Ambient Color, w = Is PBR (1.0 = PBR, 0.0 = Phong/Blinn-Phong)
    vec4 diffuse;           // xyz = Diffuse/Base Color, w = Has Diffuse Map
    vec4 specular;          // xyz = Specular Color, w = Has Specular Map
    vec4 emissive;          // xyz = Emissive Color, w = Intensity
    vec4 pbrFactors;        // x = Roughness, y = Metallic, z = AO, w = Shininess (Phong)
    vec4 pbrMaps;           // x = hasNormalMap, y = hasMetallicRoughnessMap, z = hasEmissiveMap, w = hasAoMap
} material;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;

out vec4 FragColor;

uniform vec3 globalAmbient;

uniform sampler2D diffuseMap;
uniform sampler2D specularMap;
uniform sampler2D normalMap;
uniform sampler2D metallicRoughnessMap;
uniform sampler2D emissiveMap;
uniform sampler2D aoMap;

uniform vec3 viewPos;

uniform bool emissiveOnly;
uniform bool alphaMasked;
uniform float alphaCutoff;

const float PI = 3.14159265359;

// --- Helpers Math ---

float distanceToSegment(vec3 p, vec3 a, vec3 b)
{
    vec3 ab = b - a;
    float t = clamp(dot(p - a, ab) / dot(ab, ab), 0.0, 1.0);
    vec3 closestPoint = a + t * ab;
    return length(p - closestPoint);
}

float calculateAttenuation(vec4 attenuation, float distance)
{
    return 1.0 / max(attenuation.x + attenuation.y * distance + attenuation.z * (distance * distance), 0.0001);
}

vec3 getNormalFromMap(vec3 N, vec3 p, vec2 uv)
{
    vec3 tangentNormal = texture(normalMap, uv).xyz * 2.0 - 1.0;

    vec3 dp1 = dFdx(p);
    vec3 dp2 = dFdy(p);
    vec2 duv1 = dFdx(uv);
    vec2 duv2 = dFdy(uv);

    vec3 dp2perp = cross(dp2, N);
    vec3 dp1perp = cross(N, dp1);

    vec3 T = dp2perp * duv1.x + dp1perp * duv2.x;
    vec3 B = dp2perp * duv1.y + dp1perp * duv2.y;

    float invmax = inversesqrt(max(dot(T, T), dot(B, B)));
    mat3 TBN = mat3(T * invmax, B * invmax, N);

    return normalize(TBN * tangentNormal);
}

// --- PBR Functions ---

float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float nom = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return nom / max(denom, 0.0000001);
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;

    float nom = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / max(denom, 0.0000001);
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

void main()
{
    if (emissiveOnly)
    {
        FragColor = vec4(material.emissive.rgb * material.emissive.w, 1.0);
        return;
    }

    // Materials & Textures Setup
    vec4 texDiffuseColor = (material.diffuse.w > 0.5) ? texture(diffuseMap, TexCoord) : vec4(1.0);
    if (alphaMasked && texDiffuseColor.a < alphaCutoff) discard;

    vec3 albedo = material.diffuse.rgb * texDiffuseColor.rgb;

    float metallic = material.pbrFactors.y;
    float roughness = material.pbrFactors.x;
    float ao = material.pbrFactors.z;
    float shininess = material.pbrFactors.w; // Use only if !isPBR

    if (material.pbrMaps.y > 0.5)
    {
        vec4 mr = texture(metallicRoughnessMap, TexCoord);
        roughness *= mr.g;
        metallic *= mr.b;
    }

    if (material.pbrMaps.w > 0.5)
    {
        ao *= texture(aoMap, TexCoord).r;
    }

    roughness = clamp(roughness, 0.04, 1.0);
    metallic = clamp(metallic, 0.0, 1.0);

    vec3 N = normalize(Normal);
    if (material.pbrMaps.x > 0.5)
    {
        N = getNormalFromMap(N, FragPos, TexCoord);
    }

    vec3 V = normalize(viewPos - FragPos);

    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metallic);

    vec3 Lo = vec3(0.0);
    vec3 ambientAccum = vec3(0.0);


    bool isPBR = (material.ambient.w > 0.5 ? true : false);

    for (int i = 0; i < lightCount; ++i)
    {
        int type = int(lights[i].position.w + 0.5);
        vec3 lightColor = lights[i].color.rgb;
        float intensity = lights[i].color.w; // Intensité globale

        // Phong Legacy
        float ambFactor = lights[i].direction.w;
        float diffFactor = lights[i].attenuation.w;
        float specFactor = lights[i].spotParams.w;
        ambientAccum += lightColor * ambFactor * intensity;

        vec3 lightDir = vec3(0.0, 1.0, 0.0);
        float attenuation = 1.0;

        if (type == 0) // Directional light
        {
            lightDir = length(lights[i].direction.xyz) > 0.0001 ? normalize(-lights[i].direction.xyz) : vec3(0.0, 1.0, 0.0);
            attenuation = intensity;
        }
        else if (type >= 1 && type <= 5) // Point / Spot / Neon / Sphere / Area lights
        {
            vec3 toLight = lights[i].position.xyz - FragPos;
            float dist = length(toLight);
            lightDir = dist > 0.0001 ? normalize(toLight) : vec3(0.0, 1.0, 0.0);

            if (isPBR) {
                attenuation = intensity / max(dist * dist, 0.01);
            } else {
                attenuation = calculateAttenuation(lights[i].attenuation, dist) * intensity;
            }

            if (type == 2) // Spot light
            {
                vec3 spotDir = length(lights[i].direction.xyz) > 0.0001 ? normalize(-lights[i].direction.xyz) : vec3(0.0, 1.0, 0.0);
                float theta = dot(lightDir, spotDir);
                float epsilon = lights[i].spotParams.x - lights[i].spotParams.y;
                float cone = clamp((theta - lights[i].spotParams.y) / max(epsilon, 0.0001), 0.0, 1.0);
                attenuation *= cone;
            }

            // Position = start, Direction = end
            // spotParams.z = radius
            if (type == 3) // Neon light
            {
                vec3 neonStart = lights[i].position.xyz;
                vec3 neonEnd = neonStart + lights[i].direction.xyz;

                float distToTube = distanceToSegment(FragPos, neonStart, neonEnd);
                float neonRadius = max(lights[i].spotParams.z, 0.0001);

                if (distToTube > neonRadius) {
                    attenuation = 0.0;
                } else {
                    float normalizedDist = distToTube / neonRadius;
                    attenuation = intensity * exp(-normalizedDist * 4.0);
                }

                vec3 ab = neonEnd - neonStart;
                float abLenSq = max(dot(ab, ab), 0.0001);
                float t = clamp(dot(FragPos - neonStart, ab) / abLenSq, 0.0, 1.0);
                vec3 closestPoint = neonStart + t * ab;
                vec3 toClosest = closestPoint - FragPos;
                lightDir = length(toClosest) > 0.0001 ? normalize(toClosest) : vec3(0.0, 1.0, 0.0);
            }

            // Position = center
            // spotParams.z = radius
            if (type == 4) // Sphere light
            {
                vec3 sphereCenter = lights[i].position.xyz;
                float sphereRadius = lights[i].spotParams.z;

                vec3 toCenter = sphereCenter - FragPos;
                float distToCenter = length(toCenter);

                if (distToCenter < sphereRadius) {
                    attenuation = intensity * 2.0;
                } else {
                    float distFromSurface = distToCenter - sphereRadius;
                    float falloffRange = max(lights[i].attenuation.y, 0.0001);

                    if (distFromSurface > falloffRange) {
                        attenuation = 0.0;
                    } else {
                        float normalizedDist = distFromSurface / falloffRange;
                        attenuation = intensity * (1.0 - normalizedDist) * (1.0 - normalizedDist);
                    }
                }

                lightDir = distToCenter > 0.0001 ? normalize(toCenter) : vec3(0.0, 1.0, 0.0);
            }

            // Position = center, Direction = normal
            // spotParams.x = width, spotParams.y = height

            if (type == 5) // Area light
            {
                vec3 center = lights[i].position.xyz;
                vec3 normal = length(lights[i].direction.xyz) > 0.0001 ? normalize(lights[i].direction.xyz) : vec3(0.0, -1.0, 0.0);
                float width = lights[i].spotParams.x;
                float height = lights[i].spotParams.y;

                vec3 toFrag = FragPos - center;
                float distToPlane = abs(dot(toFrag, normal));

                float maxDim = max(width, height);
                if (distToPlane > maxDim * 2.0) {
                    attenuation = 0.0;
                } else {
                    attenuation = intensity / (1.0 + distToPlane * distToPlane);
                }

                vec3 toCenter = center - FragPos;
                lightDir = length(toCenter) > 0.0001 ? normalize(toCenter) : vec3(0.0, 1.0, 0.0);
            }
        }

        vec3 L = lightDir;
        vec3 H = normalize(V + L);

        vec3 radiance = lightColor * attenuation;

        if (isPBR)
        {
            float NDF = DistributionGGX(N, H, roughness);
            float G = GeometrySmith(N, V, L, roughness);
            vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);

            vec3 numerator = NDF * G * F;
            float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
            vec3 specular = numerator / denominator;

            vec3 kS = F;
            vec3 kD = (vec3(1.0) - kS) * (1.0 - metallic);

            float NdotL = max(dot(N, L), 0.0);
            Lo += (kD * albedo / PI + specular) * radiance * NdotL;
        }
        else
        {
            float NdotL = max(dot(N, L), 0.0);

            // Diffuse
            vec3 diffuseTerm = NdotL * albedo * diffFactor;

            // Specular
            float spec = 0.0;
            if (shininess > 0.0) {
                spec = pow(max(dot(N, H), 0.0), shininess);
            }
            vec3 specularTerm = spec * material.specular.rgb * specFactor;

            Lo += (diffuseTerm + specularTerm) * radiance;
        }
    }


    vec3 ambient = globalAmbient * albedo * ao;

    vec3 emissive = albedo * material.emissive.rgb * material.emissive.w;
    if (material.pbrMaps.z > 0.5)
    {
        emissive *= texture(emissiveMap, TexCoord).rgb;
    }

    vec3 color = ambient + Lo + emissive;

    FragColor = vec4(color, texDiffuseColor.a);
}