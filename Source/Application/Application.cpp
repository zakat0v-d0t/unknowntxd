#include "Application.h"

#include <cstdio>

#include <GLFW/glfw3.h>

#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"

namespace
{
    constexpr int DefaultWidth = 1100;
    constexpr int DefaultHeight = 720;
    constexpr const char* WindowTitle = "UnknownTxd";
    constexpr const char* GlslVersion = "#version 130";
}

Application::Application() = default;

Application::~Application()
{
    Shutdown();
}

bool Application::Initialise()
{
    glfwSetErrorCallback([](int Code, const char* Message) {
        std::fprintf(stderr, "GLFW error %d: %s\n", Code, Message);
    });

    if (!glfwInit())
        return false;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

    Window = glfwCreateWindow(DefaultWidth, DefaultHeight, WindowTitle, nullptr, nullptr);
    if (Window == nullptr)
    {
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(Window);
    glfwSwapInterval(1);

    glfwSetWindowUserPointer(Window, this);
    glfwSetDropCallback(Window, &Application::OnFilesDropped);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(Window, true);
    ImGui_ImplOpenGL3_Init(GlslVersion);

    Initialised = true;
    return true;
}

void Application::Run()
{
    while (!glfwWindowShouldClose(Window))
    {
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        Editor.Render();

        ImGui::Render();

        int FramebufferWidth = 0;
        int FramebufferHeight = 0;
        glfwGetFramebufferSize(Window, &FramebufferWidth, &FramebufferHeight);
        glViewport(0, 0, FramebufferWidth, FramebufferHeight);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(Window);
    }
}

void Application::LoadFile(const std::string& Path)
{
    Editor.OpenFile(Path);
}

void Application::OnFilesDropped(GLFWwindow* Window, int Count, const char** Paths)
{
    if (Count <= 0)
        return;
    auto* Self = static_cast<Application*>(glfwGetWindowUserPointer(Window));
    if (Self != nullptr)
        Self->Editor.OpenFile(Paths[0]);
}

void Application::Shutdown()
{
    if (!Initialised)
        return;

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    if (Window != nullptr)
        glfwDestroyWindow(Window);
    glfwTerminate();

    Window = nullptr;
    Initialised = false;
}
