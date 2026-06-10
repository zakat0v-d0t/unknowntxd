#include "FileDialog.h"

#include <array>
#include <cstdio>
#include <cstdlib>

namespace
{
    bool DesktopIsKde()
    {
        const char* Desktop = std::getenv("XDG_CURRENT_DESKTOP");
        if (Desktop == nullptr)
            return false;
        std::string Value(Desktop);
        return Value.find("KDE") != std::string::npos;
    }

    std::string HomeDirectory()
    {
        const char* Home = std::getenv("HOME");
        return Home != nullptr ? std::string(Home) : std::string(".");
    }
}

bool FileDialog::CommandExists(const std::string& Name)
{
    std::string Probe = "command -v " + Name + " >/dev/null 2>&1";
    return std::system(Probe.c_str()) == 0;
}

std::string FileDialog::RunCommand(const std::string& Command)
{
    std::FILE* Pipe = popen(Command.c_str(), "r");
    if (Pipe == nullptr)
        return {};

    std::string Output;
    std::array<char, 1024> Buffer;
    while (std::fgets(Buffer.data(), static_cast<int>(Buffer.size()), Pipe) != nullptr)
        Output += Buffer.data();
    pclose(Pipe);

    while (!Output.empty() && (Output.back() == '\n' || Output.back() == '\r'))
        Output.pop_back();
    return Output;
}

std::string FileDialog::Quote(const std::string& Value)
{
    std::string Result = "'";
    for (char Character : Value)
    {
        if (Character == '\'')
            Result += "'\\''";
        else
            Result += Character;
    }
    Result += "'";
    return Result;
}

std::string FileDialog::OpenFile(const std::string& Title, const std::string& FilterName, const std::string& FilterPattern)
{
    bool HasKdialog = CommandExists("kdialog");
    bool HasZenity = CommandExists("zenity");
    bool PreferKdialog = DesktopIsKde();

    auto RunKdialog = [&]() {
        std::string Command = "kdialog --title " + Quote(Title) + " --getopenfilename " +
                              Quote(HomeDirectory()) + " " + Quote(FilterPattern + "|" + FilterName);
        return RunCommand(Command);
    };

    auto RunZenity = [&]() {
        std::string Command = "zenity --file-selection --title=" + Quote(Title) +
                              " --file-filter=" + Quote(FilterName + " | " + FilterPattern);
        return RunCommand(Command);
    };

    if (PreferKdialog && HasKdialog)
        return RunKdialog();
    if (HasZenity)
        return RunZenity();
    if (HasKdialog)
        return RunKdialog();
    return {};
}

std::string FileDialog::SaveFile(const std::string& Title, const std::string& SuggestedName, const std::string& FilterName, const std::string& FilterPattern)
{
    bool HasKdialog = CommandExists("kdialog");
    bool HasZenity = CommandExists("zenity");
    bool PreferKdialog = DesktopIsKde();

    std::string StartPath = HomeDirectory() + "/" + SuggestedName;

    auto RunKdialog = [&]() {
        std::string Command = "kdialog --title " + Quote(Title) + " --getsavefilename " +
                              Quote(StartPath) + " " + Quote(FilterPattern + "|" + FilterName);
        return RunCommand(Command);
    };

    auto RunZenity = [&]() {
        std::string Command = "zenity --file-selection --save --confirm-overwrite --title=" + Quote(Title) +
                              " --filename=" + Quote(StartPath) +
                              " --file-filter=" + Quote(FilterName + " | " + FilterPattern);
        return RunCommand(Command);
    };

    if (PreferKdialog && HasKdialog)
        return RunKdialog();
    if (HasZenity)
        return RunZenity();
    if (HasKdialog)
        return RunKdialog();
    return {};
}
