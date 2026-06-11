#pragma once

#include <QMainWindow>
#include <QListWidget>
#include <QLabel>
#include <QSplitter>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <string>
#include <vector>
#include <utility>
#include "RenderWare/TextureDictionary.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    void OpenFile(const std::string& Path);

private slots:
    void RequestOpenDialog();
    void RequestExportDialog();
    void RequestExportAllDialog();
    void RequestReplaceDialog();
    void RequestSaveAsDialog();
    void RequestAddDialog();
    void RequestRename();
    void RequestDeleteSelected();
    void UndoDelete();
    void SaveCurrent();
    void OnTextureSelected(int Index);
    void RequestResize();
    void ToggleTheme();
    
    void SetCompressionDxt1();
    void SetCompressionDxt3();
    void SetCompressionDxt5();
    void SetCompressionNone();

    void CreateMipLevels();
    void ClearMipLevels();

protected:
    void wheelEvent(QWheelEvent* event) override;
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dropEvent(QDropEvent* event) override;

private:
    void SetupUi();
    void SetupMenus();
    void UpdateTextureList();
    void UpdatePreview();
    void ApplyReplacement(const std::string& ImagePath);
    void ApplyAddition(const std::string& ImagePath);
    void ApplyTheme();
    bool HasSelection() const;

    RenderWare::TextureDictionary Dictionary;
    int SelectedIndex = -1;
    bool IsDarkTheme = true;

    std::vector<std::pair<int, RenderWare::Texture>> DeleteUndoStack;

    QListWidget* TextureList;
    QLabel* PreviewLabel;
    QLabel* InfoLabel;
    QLabel* StatusLabel;
    float PreviewZoom = 1.0f;
};
