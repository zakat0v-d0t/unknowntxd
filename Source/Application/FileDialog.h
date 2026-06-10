#pragma once

#include <string>

class FileDialog
{
public:
    static std::string OpenFile(const std::string& Title, const std::string& FilterName, const std::string& FilterPattern);
    static std::string SaveFile(const std::string& Title, const std::string& SuggestedName, const std::string& FilterName, const std::string& FilterPattern);

private:
    static bool CommandExists(const std::string& Name);
    static std::string RunCommand(const std::string& Command);
    static std::string Quote(const std::string& Value);
};
