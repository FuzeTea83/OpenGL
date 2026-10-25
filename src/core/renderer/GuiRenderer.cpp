#include "GuiRenderer.h"

#include <array>
#include <format>

#include <glm/gtc/type_ptr.hpp>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "core/Engine.h"
#include "utils/IconsMaterialSymbols.h"
#include "utils/OpenGLUtils.h"


GuiRenderer::GuiRenderer(const Engine &engine) : mEngine(engine)
{
    assertGlThread();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;

    ImGuiStyle &style = ImGui::GetStyle();
    style.FontScaleDpi = 1.3f;
    style.ScaleAllSizes(1.3f);

    io.Fonts->AddFontDefault();

    ImFontConfig config;
    config.MergeMode = true;
    config.GlyphOffset.y = 2.0f;

    static constexpr ImWchar icon_ranges[] = {ICON_MIN_MS, ICON_MAX_16_MS, 0};
    io.Fonts->AddFontFromFileTTF("assets/fonts/MaterialSymbolsOutlined[FILL,GRAD,opsz,wght].ttf", 13.0f, &config,
                                 icon_ranges);

    ImGui_ImplGlfw_InitForOpenGL(engine.getWindow().getNativeHandle(), true);
    ImGui_ImplOpenGL3_Init("#version 330");

    engine.getWindow().addFramebufferSizeCallback([this](const int width, const int height)
    {
        for (const auto &win: mWindows | std::views::values) { win->onFramebufferSizeChanged(width, height); }
    });

    mViewMenu = createMenu("View", 1);
}

GuiRenderer::~GuiRenderer()
{
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void GuiRenderer::render()
{
    assertGlThread();
    if (!mShowEditor) { return; }
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    if (ImGui::BeginMainMenuBar())
    {
        for (const auto &button: mMenus | std::views::values) { button->render(); }

        ImGui::EndMainMenuBar();
    }

    const float menuBarHeight = ImGui::GetFrameHeight();


    for (const auto &win: mWindows | std::views::values)
    {

        win->render();
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

const Engine &GuiRenderer::getEngine() const { return mEngine; }

void GuiRenderer::renderCodec(const Codec &codec)
{
    if (!ImGui::BeginTable("form", 2, ImGuiTableFlags_BordersInnerV)) { return; }
    ImGui::TableSetupColumn("label", ImGuiTableColumnFlags_WidthFixed, 200.0f);
    ImGui::TableSetupColumn("widget", ImGuiTableColumnFlags_WidthStretch);

    for (auto property: codec.properties)
    {
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(property.first.c_str());
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-FLT_MIN);

        switch (property.second.first)
        {
            case Codec::Type::BOOL:
            {
                ImGui::Checkbox(
                    std::string("##" + property.first).c_str(),
                    std::get<bool *>(property.second.second));
                break;
            }
            case Codec::Type::INT:
            {
                ImGui::DragInt(
                    std::string("##" + property.first).c_str(),
                    std::get<int *>(property.second.second));
                break;
            }
            case Codec::Type::FLOAT:
            {
                ImGui::DragFloat(
                    std::string("##" + property.first).c_str(),
                    std::get<float *>(property.second.second));
                break;
            }
            case Codec::Type::VEC2:
            {
                ImGui::DragFloat2(
                    std::string("##" + property.first).c_str(),
                    glm::value_ptr(*std::get<glm::vec2 *>(property.second.second)));
                break;
            }
            case Codec::Type::VEC3:
            {
                ImGui::DragFloat3(
                    std::string("##" + property.first).c_str(),
                    glm::value_ptr(*std::get<glm::vec3 *>(property.second.second)));
                break;
            }
            case Codec::Type::VEC4:
            {
                ImGui::DragFloat4(
                    std::string("##" + property.first).c_str(),
                    glm::value_ptr(*std::get<glm::vec4 *>(property.second.second)));
                break;
            }
            case Codec::Type::QUATERNION:
            {
                ImGui::DragFloat4(
                    std::string("##" + property.first).c_str(),
                    glm::value_ptr(*std::get<glm::quat *>(property.second.second)));
                break;
            }
            case Codec::Type::STRING:
            {
                ImGui::InputText(
                    std::string("##" + property.first).c_str(),
                    std::get<std::string *>(property.second.second)->data(),
                    std::get<std::string *>(property.second.second)->capacity());
                break;
            }
            case Codec::Type::COLOR3:
            {
                ImGui::ColorEdit3(
                    std::string("##" + property.first).c_str(),
                    glm::value_ptr(*std::get<glm::vec3 *>(property.second.second)));
                break;
            }
            case Codec::Type::COLOR4:
            {
                ImGui::ColorEdit4(
                    std::string("##" + property.first).c_str(),
                    glm::value_ptr(*std::get<glm::vec4 *>(property.second.second)));
                break;
            }
            case Codec::Type::OPTION:
            {
                auto *option = std::get<Codec::Option *>(property.second.second);
                if (!ImGui::BeginCombo(std::string("##" + property.first).c_str(),
                                       option->selectedOption.c_str())) { break; }

                for (const auto &opt: option->options)
                {
                    const bool isSelected = (opt == option->selectedOption);
                    if (ImGui::Selectable(opt.c_str(), isSelected))
                    {
                        option->selectedOption = opt;
                        if (option->onSelected) { option->onSelected(option->selectedOption); }
                    }
                    if (isSelected) { ImGui::SetItemDefaultFocus(); }
                }
                ImGui::EndCombo();
                break;
            }
            case Codec::Type::CODEC:
            {
                ImGui::BeginChild(std::format("Codec: {}", property.first).c_str(),
                                  ImVec2(0, 0), ImGuiChildFlags_AutoResizeY);
                renderCodec(*std::get<Codec *>(property.second.second));
                ImGui::EndChild();
                break;
            }
            case Codec::Type::CUSTOM_JSON:
            {
                break;
            }
        }
    }

    ImGui::EndTable();
}

void GuiRenderer::enableAllWindows() const
{
    for (const auto &val: mWindows | std::views::values) { val->disable(false); }
}

void GuiRenderer::disableAllWindowsExcept(const GuiWindow &window) const
{
    for (const auto &val: mWindows | std::views::values) { if (val.get() != &window) { val->disable(true); } }
}
