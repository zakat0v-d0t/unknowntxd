#pragma once

#include <future>
#include <string>

#include "GlTexture.h"
#include "RenderWare/TextureDictionary.h"

class MainWindow
{
public:
    void OpenFile(const std::string& Path);
    void Render();

private:
    void RenderMenuBar();
    void RenderTextureList();
    void RenderPreview();
    void RenderResizePopup();

    void SelectTexture(int Index);
    void UpdatePreview();

    void RequestOpenDialog();
    void RequestExportDialog();
    void RequestReplaceDialog();
    void RequestSaveAsDialog();
    void SaveCurrent();
    void ApplyReplacement(const std::string& ImagePath);

    void PollPendingOpen();
    void PollPendingExport();
    void PollPendingReplace();
    void PollPendingSaveAs();

    bool HasSelection() const;

    RenderWare::TextureDictionary Dictionary;
    GlTexture PreviewTexture;
    int SelectedIndex = -1;
    float PreviewZoom = 1.0f;
    std::string StatusMessage = "No file loaded";

    std::future<std::string> PendingOpen;
    bool OpenDialogActive = false;

    std::future<std::string> PendingExport;
    bool ExportDialogActive = false;
    RenderWare::Image ExportImage;

    std::future<std::string> PendingReplace;
    bool ReplaceDialogActive = false;
    int ReplaceTargetIndex = -1;

    std::future<std::string> PendingSaveAs;
    bool SaveAsDialogActive = false;

    bool ResizePopupRequested = false;
    int ResizeWidth = 0;
    int ResizeHeight = 0;
};
