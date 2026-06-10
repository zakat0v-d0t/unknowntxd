#pragma once

#include <string>

#include "MainWindow.h"

struct GLFWwindow;

class Application
{
public:
    Application();
    ~Application();

    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;

    bool Initialise();
    void Run();
    void LoadFile(const std::string& Path);

private:
    void Shutdown();
    static void OnFilesDropped(GLFWwindow* Window, int Count, const char** Paths);

    GLFWwindow* Window = nullptr;
    MainWindow Editor;
    bool Initialised = false;
};
