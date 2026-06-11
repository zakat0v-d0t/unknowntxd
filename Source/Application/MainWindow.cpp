#include "MainWindow.h"

#include <chrono>
#include <cstdlib>

#include "FileDialog.h"
#include "RenderWare/ImageReader.h"
#include "RenderWare/ImageWriter.h"
#include "imgui.h"

using RenderWare::CompressionType;

namespace
{
    int NearestPowerOfTwo(int Value)
    {
        if (Value < 1)
            Value = 1;
        if (Value > 4096)
            Value = 4096;
        int Lower = 1;
        while (Lower * 2 <= Value)
            Lower *= 2;
        int Upper = Lower * 2;
        return (Value - Lower < Upper - Value) ? Lower : Upper;
    }
}

void MainWindow::OpenFile(const std::string& Path)
{
    std::string Error;
    if (Dictionary.LoadFromFile(Path, Error))
    {
        SelectedIndex = -1;
        PreviewTexture.Release();
        if (!Dictionary.Textures().empty())
            SelectTexture(0);
        StatusMessage = Path + "  (" + std::to_string(Dictionary.Textures().size()) + " textures)";
    }
    else
    {
        StatusMessage = "Failed to load: " + Error;
    }
}

bool MainWindow::HasSelection() const
{
    return SelectedIndex >= 0 && SelectedIndex < static_cast<int>(Dictionary.Textures().size());
}

void MainWindow::SelectTexture(int Index)
{
    SelectedIndex = Index;
    PreviewZoom = 1.0f;
    UpdatePreview();
}

void MainWindow::UpdatePreview()
{
    if (!HasSelection())
    {
        PreviewTexture.Release();
        return;
    }
    PreviewTexture.Upload(Dictionary.Textures()[SelectedIndex].Pixels());
}

void MainWindow::RequestOpenDialog()
{
    if (OpenDialogActive)
        return;
    OpenDialogActive = true;
    PendingOpen = std::async(std::launch::async, []() -> std::string {
        return FileDialog::OpenFile("Open Texture Dictionary", "RenderWare TXD", "*.txd");
    });
}

void MainWindow::RequestExportDialog()
{
    if (ExportDialogActive || !HasSelection())
        return;

    const RenderWare::Texture& Item = Dictionary.Textures()[SelectedIndex];
    ExportImage = Item.Pixels();
    std::string Suggested = (Item.Name().empty() ? "texture" : Item.Name()) + ".png";

    ExportDialogActive = true;
    PendingExport = std::async(std::launch::async, [Suggested]() -> std::string {
        return FileDialog::SaveFile("Export Texture", Suggested, "PNG image", "*.png");
    });
}

void MainWindow::RequestReplaceDialog()
{
    if (ReplaceDialogActive || !HasSelection())
        return;
    ReplaceTargetIndex = SelectedIndex;
    ReplaceDialogActive = true;
    PendingReplace = std::async(std::launch::async, []() -> std::string {
        return FileDialog::OpenFile("Replace Texture", "Image files", "*.png *.jpg *.jpeg *.bmp *.tga");
    });
}

void MainWindow::RequestSaveAsDialog()
{
    if (SaveAsDialogActive || !Dictionary.IsLoaded())
        return;

    std::string Suggested = "texture.txd";
    const std::string& Source = Dictionary.SourcePath();
    std::size_t Slash = Source.find_last_of('/');
    if (Slash != std::string::npos && Slash + 1 < Source.size())
        Suggested = Source.substr(Slash + 1);

    SaveAsDialogActive = true;
    PendingSaveAs = std::async(std::launch::async, [Suggested]() -> std::string {
        return FileDialog::SaveFile("Save Texture Dictionary", Suggested, "RenderWare TXD", "*.txd");
    });
}

void MainWindow::SaveCurrent()
{
    if (!Dictionary.IsLoaded())
        return;
    std::string Error;
    if (Dictionary.SaveToFile(Dictionary.SourcePath(), Error))
        StatusMessage = "Saved: " + Dictionary.SourcePath();
    else
        StatusMessage = "Save failed: " + Error;
}

void MainWindow::ApplyReplacement(const std::string& ImagePath)
{
    RenderWare::Image Loaded;
    if (!RenderWare::ImageReader::Load(ImagePath, Loaded))
    {
        StatusMessage = "Failed to load image: " + ImagePath;
        return;
    }

    if (Dictionary.ReplaceTexture(ReplaceTargetIndex, Loaded))
    {
        if (ReplaceTargetIndex == SelectedIndex)
            UpdatePreview();
        StatusMessage = "Replaced texture from " + ImagePath;
    }
}

void MainWindow::PollPendingOpen()
{
    if (!OpenDialogActive || !PendingOpen.valid())
        return;
    if (PendingOpen.wait_for(std::chrono::seconds(0)) != std::future_status::ready)
        return;

    std::string Path = PendingOpen.get();
    OpenDialogActive = false;
    if (!Path.empty())
        OpenFile(Path);
}

void MainWindow::PollPendingExport()
{
    if (!ExportDialogActive || !PendingExport.valid())
        return;
    if (PendingExport.wait_for(std::chrono::seconds(0)) != std::future_status::ready)
        return;

    std::string Path = PendingExport.get();
    ExportDialogActive = false;
    if (Path.empty())
        return;

    if (Path.size() < 4 || Path.compare(Path.size() - 4, 4, ".png") != 0)
        Path += ".png";

    if (RenderWare::ImageWriter::SavePng(Path, ExportImage))
        StatusMessage = "Exported: " + Path;
    else
        StatusMessage = "Export failed: " + Path;
}

void MainWindow::PollPendingReplace()
{
    if (!ReplaceDialogActive || !PendingReplace.valid())
        return;
    if (PendingReplace.wait_for(std::chrono::seconds(0)) != std::future_status::ready)
        return;

    std::string Path = PendingReplace.get();
    ReplaceDialogActive = false;
    if (!Path.empty())
        ApplyReplacement(Path);
}

void MainWindow::PollPendingSaveAs()
{
    if (!SaveAsDialogActive || !PendingSaveAs.valid())
        return;
    if (PendingSaveAs.wait_for(std::chrono::seconds(0)) != std::future_status::ready)
        return;

    std::string Path = PendingSaveAs.get();
    SaveAsDialogActive = false;
    if (Path.empty())
        return;

    if (Path.size() < 4 || Path.compare(Path.size() - 4, 4, ".txd") != 0)
        Path += ".txd";

    std::string Error;
    if (Dictionary.SaveToFile(Path, Error))
        StatusMessage = "Saved: " + Path;
    else
        StatusMessage = "Save failed: " + Error;
}

void MainWindow::Render()
{
    PollPendingOpen();
    PollPendingExport();
    PollPendingReplace();
    PollPendingSaveAs();

    RenderMenuBar();

    const ImGuiViewport* Viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(Viewport->WorkPos);
    ImGui::SetNextWindowSize(Viewport->WorkSize);

    ImGuiWindowFlags WindowFlags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                                   ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::Begin("UnknownTxd", nullptr, WindowFlags);
    ImGui::PopStyleVar(2);

    ImGui::TextUnformatted(StatusMessage.c_str());
    ImGui::Separator();

    float ListWidth = 260.0f;
    ImGui::BeginChild("TextureList", ImVec2(ListWidth, 0), ImGuiChildFlags_Borders);
    RenderTextureList();
    ImGui::EndChild();

    ImGui::SameLine();

    ImGui::BeginChild("Preview", ImVec2(0, 0), ImGuiChildFlags_Borders);
    RenderPreview();
    ImGui::EndChild();

    RenderResizePopup();

    ImGui::End();
}

void MainWindow::RenderMenuBar()
{
    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("Open", "Ctrl+O"))
                RequestOpenDialog();
            if (ImGui::MenuItem("Save", "Ctrl+S", false, Dictionary.IsLoaded()))
                SaveCurrent();
            if (ImGui::MenuItem("Save As", nullptr, false, Dictionary.IsLoaded()))
                RequestSaveAsDialog();
            ImGui::Separator();
            if (ImGui::MenuItem("Quit", "Alt+F4"))
                std::exit(0);
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Texture", HasSelection()))
        {
            if (ImGui::MenuItem("Export"))
                RequestExportDialog();
            if (ImGui::MenuItem("Replace"))
                RequestReplaceDialog();
            if (ImGui::MenuItem("Resize"))
            {
                ResizeWidth = Dictionary.Textures()[SelectedIndex].Width();
                ResizeHeight = Dictionary.Textures()[SelectedIndex].Height();
                ResizePopupRequested = true;
            }
            if (ImGui::BeginMenu("Compression"))
            {
                if (ImGui::MenuItem("DXT1"))
                    Dictionary.SetTextureCompression(SelectedIndex, CompressionType::Dxt1);
                if (ImGui::MenuItem("DXT3"))
                    Dictionary.SetTextureCompression(SelectedIndex, CompressionType::Dxt3);
                if (ImGui::MenuItem("DXT5"))
                    Dictionary.SetTextureCompression(SelectedIndex, CompressionType::Dxt5);
                if (ImGui::MenuItem("RGBA8888 (uncompressed)"))
                    Dictionary.SetTextureCompression(SelectedIndex, CompressionType::None);
                ImGui::EndMenu();
            }
            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
    }

    ImGuiIO& Io = ImGui::GetIO();
    if (Io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_O))
        RequestOpenDialog();
    if (Io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_S) && Dictionary.IsLoaded())
        SaveCurrent();
}

void MainWindow::RenderTextureList()
{
    const std::vector<RenderWare::Texture>& Textures = Dictionary.Textures();
    for (int Index = 0; Index < static_cast<int>(Textures.size()); ++Index)
    {
        const RenderWare::Texture& Item = Textures[Index];
        std::string Label = Item.Name().empty() ? "(unnamed)" : Item.Name();
        if (Item.IsEdited())
            Label += " *";
        Label += "##" + std::to_string(Index);
        if (ImGui::Selectable(Label.c_str(), SelectedIndex == Index))
            SelectTexture(Index);
    }
}

void MainWindow::RenderPreview()
{
    if (!HasSelection())
        return;

    const RenderWare::Texture& Item = Dictionary.Textures()[SelectedIndex];

    ImGui::Text("Name: %s", Item.Name().c_str());
    if (!Item.MaskName().empty())
        ImGui::Text("Mask: %s", Item.MaskName().c_str());
    ImGui::Text("Size: %d x %d", Item.Width(), Item.Height());
    ImGui::Text("Format: %s", Item.FormatName().c_str());
    ImGui::Text("Mip levels: %d", Item.MipLevels());
    ImGui::Text("Alpha: %s", Item.HasAlpha() ? "yes" : "no");
    ImGui::SameLine();
    ImGui::Text("    Zoom: %.0f%%", PreviewZoom * 100.0f);
    ImGui::Separator();

    if (!PreviewTexture.IsValid())
        return;

    ImGui::BeginChild("PreviewImage", ImVec2(0, 0), ImGuiChildFlags_None, ImGuiWindowFlags_HorizontalScrollbar);

    if (ImGui::IsWindowHovered())
    {
        float Wheel = ImGui::GetIO().MouseWheel;
        if (Wheel != 0.0f)
        {
            PreviewZoom *= (Wheel > 0.0f) ? 1.1f : (1.0f / 1.1f);
            if (PreviewZoom < 0.05f)
                PreviewZoom = 0.05f;
            if (PreviewZoom > 32.0f)
                PreviewZoom = 32.0f;
        }
    }

    ImVec2 DisplaySize(PreviewTexture.Width() * PreviewZoom, PreviewTexture.Height() * PreviewZoom);
    ImGui::Image(static_cast<ImTextureID>(PreviewTexture.Handle()), DisplaySize);
    ImGui::EndChild();
}

void MainWindow::RenderResizePopup()
{
    if (ResizePopupRequested)
    {
        ImGui::OpenPopup("Resize Texture");
        ResizePopupRequested = false;
    }

    if (ImGui::BeginPopupModal("Resize Texture", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::InputInt("Width", &ResizeWidth);
        ImGui::InputInt("Height", &ResizeHeight);

        ImGui::TextDisabled("Sizes snap to powers of two (game requirement)");

        if (ImGui::Button("Apply", ImVec2(120, 0)))
        {
            if (HasSelection() && ResizeWidth > 0 && ResizeHeight > 0)
            {
                int FinalWidth = NearestPowerOfTwo(ResizeWidth);
                int FinalHeight = NearestPowerOfTwo(ResizeHeight);
                Dictionary.ResizeTexture(SelectedIndex, FinalWidth, FinalHeight);
                UpdatePreview();
                StatusMessage = "Resized to " + std::to_string(FinalWidth) + " x " + std::to_string(FinalHeight);
            }
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0)))
            ImGui::CloseCurrentPopup();

        ImGui::EndPopup();
    }
}
