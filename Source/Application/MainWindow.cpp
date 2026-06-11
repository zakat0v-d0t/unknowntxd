#include "MainWindow.h"

#include <QApplication>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QFileDialog>
#include <QInputDialog>
#include <QLineEdit>
#include <QMessageBox>
#include <QImage>
#include <QPixmap>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QWheelEvent>
#include <QMimeData>
#include <QFileInfo>
#include <QPalette>
#include <QShortcut>

#include "RenderWare/ImageReader.h"
#include "RenderWare/ImageWriter.h"

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

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent)
{
    setAcceptDrops(true);
    SetupUi();
    SetupMenus();
    ApplyTheme();
}

void MainWindow::SetupUi()
{
    setWindowTitle("UnknownTxd");
    resize(1100, 720);

    QSplitter* splitter = new QSplitter(Qt::Horizontal, this);
    setCentralWidget(splitter);

    TextureList = new QListWidget(this);
    TextureList->setAlternatingRowColors(true);
    splitter->addWidget(TextureList);
    connect(TextureList, &QListWidget::currentRowChanged, this, &MainWindow::OnTextureSelected);

    QWidget* rightWidget = new QWidget(this);
    QVBoxLayout* layout = new QVBoxLayout(rightWidget);
    layout->setContentsMargins(0, 0, 0, 0);

    QScrollArea* scrollArea = new QScrollArea(this);
    PreviewLabel = new QLabel(this);
    PreviewLabel->setAlignment(Qt::AlignCenter);
    scrollArea->setWidget(PreviewLabel);
    scrollArea->setWidgetResizable(true);
    layout->addWidget(scrollArea, 1);

    InfoLabel = new QLabel(this);
    InfoLabel->setAlignment(Qt::AlignCenter);
    InfoLabel->setMargin(10);
    layout->addWidget(InfoLabel, 0);

    StatusLabel = new QLabel("No file loaded", this);
    StatusLabel->setMargin(5);
    layout->addWidget(StatusLabel, 0);

    splitter->addWidget(rightWidget);
    splitter->setSizes({260, 840});
}

void MainWindow::SetupMenus()
{
    MenuFile = menuBar()->addMenu("File");

    ActionOpen = MenuFile->addAction("Open");
    ActionOpen->setShortcut(QKeySequence::Open);
    connect(ActionOpen, &QAction::triggered, this, &MainWindow::RequestOpenDialog);

    ActionSave = MenuFile->addAction("Save");
    ActionSave->setShortcut(QKeySequence::Save);
    connect(ActionSave, &QAction::triggered, this, &MainWindow::SaveCurrent);

    ActionSaveAs = MenuFile->addAction("Save As");
    connect(ActionSaveAs, &QAction::triggered, this, &MainWindow::RequestSaveAsDialog);

    MenuFile->addSeparator();
    ActionQuit = MenuFile->addAction("Quit");
    ActionQuit->setShortcut(QKeySequence::Quit);
    connect(ActionQuit, &QAction::triggered, this, &QMainWindow::close);

    MenuEdit = menuBar()->addMenu("Edit");

    // Undo — only via Ctrl+Z, no visible menu item
    QShortcut* undoShortcut = new QShortcut(QKeySequence::Undo, this);
    connect(undoShortcut, &QShortcut::activated, this, &MainWindow::UndoDelete);

    ActionAdd = MenuEdit->addAction("Add");
    connect(ActionAdd, &QAction::triggered, this, &MainWindow::RequestAddDialog);

    ActionRename = MenuEdit->addAction("Rename");
    connect(ActionRename, &QAction::triggered, this, &MainWindow::RequestRename);

    ActionReplace = MenuEdit->addAction("Replace");
    connect(ActionReplace, &QAction::triggered, this, &MainWindow::RequestReplaceDialog);

    ActionDelete = MenuEdit->addAction("Delete");
    ActionDelete->setShortcut(QKeySequence::Delete);
    connect(ActionDelete, &QAction::triggered, this, &MainWindow::RequestDeleteSelected);

    MenuEdit->addSeparator();

    ActionExport = MenuEdit->addAction("Export");
    connect(ActionExport, &QAction::triggered, this, &MainWindow::RequestExportDialog);

    ActionExportAll = MenuEdit->addAction("Export All");
    connect(ActionExportAll, &QAction::triggered, this, &MainWindow::RequestExportAllDialog);

    ActionResize = MenuEdit->addAction("Resize");
    connect(ActionResize, &QAction::triggered, this, &MainWindow::RequestResize);

    MenuComp = MenuEdit->addMenu("Compression");
    ActionCompDxt1 = MenuComp->addAction("DXT1");
    connect(ActionCompDxt1, &QAction::triggered, this, &MainWindow::SetCompressionDxt1);
    
    ActionCompDxt3 = MenuComp->addAction("DXT3");
    connect(ActionCompDxt3, &QAction::triggered, this, &MainWindow::SetCompressionDxt3);
    
    ActionCompDxt5 = MenuComp->addAction("DXT5");
    connect(ActionCompDxt5, &QAction::triggered, this, &MainWindow::SetCompressionDxt5);
    
    ActionCompNone = MenuComp->addAction("RGBA8888 (uncompressed)");
    connect(ActionCompNone, &QAction::triggered, this, &MainWindow::SetCompressionNone);

    MenuEdit->addSeparator();
    
    ActionCreateMip = MenuEdit->addAction("Create mip levels");
    connect(ActionCreateMip, &QAction::triggered, this, &MainWindow::CreateMipLevels);
    
    ActionClearMip = MenuEdit->addAction("Clear mip levels");
    connect(ActionClearMip, &QAction::triggered, this, &MainWindow::ClearMipLevels);

    MenuView = menuBar()->addMenu("View");
    ActionTheme = MenuView->addAction("Toggle Theme");
    connect(ActionTheme, &QAction::triggered, this, &MainWindow::ToggleTheme);

    MenuLang = MenuView->addMenu("Language / Язык");
    
    ActionLangEn = MenuLang->addAction("English");
    connect(ActionLangEn, &QAction::triggered, this, &MainWindow::SetLanguageEnglish);
    
    ActionLangRu = MenuLang->addAction("Русский");
    connect(ActionLangRu, &QAction::triggered, this, &MainWindow::SetLanguageRussian);

    RetranslateUi();
}

void MainWindow::SetLanguageEnglish()
{
    CurrentLanguage = Language::English;
    RetranslateUi();
}

void MainWindow::SetLanguageRussian()
{
    CurrentLanguage = Language::Russian;
    RetranslateUi();
}

void MainWindow::RetranslateUi()
{
    if (CurrentLanguage == Language::English)
    {
        MenuFile->setTitle("File");
        ActionOpen->setText("Open");
        ActionSave->setText("Save");
        ActionSaveAs->setText("Save As");
        ActionQuit->setText("Quit");

        MenuEdit->setTitle("Edit");
        ActionAdd->setText("Add");
        ActionRename->setText("Rename");
        ActionReplace->setText("Replace");
        ActionDelete->setText("Delete");
        ActionExport->setText("Export");
        ActionExportAll->setText("Export All");
        ActionResize->setText("Resize");

        MenuComp->setTitle("Compression");
        ActionCompNone->setText("RGBA8888 (uncompressed)");

        ActionCreateMip->setText("Create mip levels");
        ActionClearMip->setText("Clear mip levels");

        MenuView->setTitle("View");
        ActionTheme->setText("Toggle Theme");
        MenuLang->setTitle("Language / Язык");
        
        if (StatusLabel->text() == "Файл не загружен")
            StatusLabel->setText("No file loaded");
    }
    else
    {
        MenuFile->setTitle("Файл");
        ActionOpen->setText("Открыть");
        ActionSave->setText("Сохранить");
        ActionSaveAs->setText("Сохранить как");
        ActionQuit->setText("Выход");

        MenuEdit->setTitle("Правка");
        ActionAdd->setText("Добавить");
        ActionRename->setText("Переименовать");
        ActionReplace->setText("Заменить");
        ActionDelete->setText("Удалить");
        ActionExport->setText("Экспорт");
        ActionExportAll->setText("Экспортировать всё");
        ActionResize->setText("Изменить размер");

        MenuComp->setTitle("Сжатие");
        ActionCompNone->setText("RGBA8888 (без сжатия)");

        ActionCreateMip->setText("Создать mip-уровни");
        ActionClearMip->setText("Удалить mip-уровни");

        MenuView->setTitle("Вид");
        ActionTheme->setText("Сменить тему");
        MenuLang->setTitle("Language / Язык");
        
        if (StatusLabel->text() == "No file loaded")
            StatusLabel->setText("Файл не загружен");
    }
}

void MainWindow::ApplyTheme()
{
    QPalette palette;
    if (IsDarkTheme)
    {
        palette.setColor(QPalette::Window, QColor(53, 53, 53));
        palette.setColor(QPalette::WindowText, Qt::white);
        palette.setColor(QPalette::Base, QColor(25, 25, 25));
        palette.setColor(QPalette::AlternateBase, QColor(53, 53, 53));
        palette.setColor(QPalette::ToolTipBase, Qt::white);
        palette.setColor(QPalette::ToolTipText, Qt::white);
        palette.setColor(QPalette::Text, Qt::white);
        palette.setColor(QPalette::Button, QColor(53, 53, 53));
        palette.setColor(QPalette::ButtonText, Qt::white);
        palette.setColor(QPalette::BrightText, Qt::red);
        palette.setColor(QPalette::Link, QColor(42, 130, 218));
        palette.setColor(QPalette::Highlight, QColor(42, 130, 218));
        palette.setColor(QPalette::HighlightedText, Qt::black);
    }
    else
    {
        palette = qApp->style()->standardPalette();
    }
    qApp->setPalette(palette);
    
    QString infoStyle = IsDarkTheme ? "color: #b0c4de; font-size: 14px;" : "color: #333333; font-size: 14px;";
    InfoLabel->setStyleSheet(infoStyle);
}

void MainWindow::ToggleTheme()
{
    IsDarkTheme = !IsDarkTheme;
    ApplyTheme();
}

void MainWindow::dragEnterEvent(QDragEnterEvent* event)
{
    if (event->mimeData()->hasUrls())
        event->acceptProposedAction();
}

void MainWindow::dropEvent(QDropEvent* event)
{
    const QMimeData* mimeData = event->mimeData();
    if (mimeData->hasUrls())
    {
        QList<QUrl> urlList = mimeData->urls();
        for (const QUrl& url : urlList)
        {
            QString filePath = url.toLocalFile();
            if (filePath.endsWith(".txd", Qt::CaseInsensitive))
            {
                OpenFile(filePath.toStdString());
                break;
            }
            else if (filePath.endsWith(".png", Qt::CaseInsensitive) || 
                     filePath.endsWith(".jpg", Qt::CaseInsensitive) ||
                     filePath.endsWith(".jpeg", Qt::CaseInsensitive) ||
                     filePath.endsWith(".bmp", Qt::CaseInsensitive) ||
                     filePath.endsWith(".tga", Qt::CaseInsensitive))
            {
                ApplyAddition(filePath.toStdString());
            }
        }
    }
}

void MainWindow::OpenFile(const std::string& Path)
{
    std::string Error;
    if (Dictionary.LoadFromFile(Path, Error))
    {
        DeleteUndoStack.clear();
        SelectedIndex = -1;
        StatusLabel->setText(QString::fromStdString(Path + "  (" + std::to_string(Dictionary.Textures().size()) + " textures)"));
        UpdateTextureList();
    }
    else
    {
        StatusLabel->setText(QString::fromStdString("Failed to load: " + Error));
    }
}

bool MainWindow::HasSelection() const
{
    return SelectedIndex >= 0 && SelectedIndex < static_cast<int>(Dictionary.Textures().size());
}

void MainWindow::UpdateTextureList()
{
    TextureList->blockSignals(true);
    TextureList->clear();
    const auto& Textures = Dictionary.Textures();
    for (const auto& Item : Textures)
    {
        std::string Label = Item.Name().empty() ? "(unnamed)" : Item.Name();
        if (Item.IsEdited())
            Label += " *";
        TextureList->addItem(QString::fromStdString(Label));
    }
    if (!Textures.empty() && SelectedIndex >= 0 && static_cast<std::size_t>(SelectedIndex) < Textures.size())
    {
        TextureList->setCurrentRow(SelectedIndex);
    }
    else if (!Textures.empty())
    {
        TextureList->setCurrentRow(0);
        SelectedIndex = 0;
    }
    else
    {
        SelectedIndex = -1;
        UpdatePreview();
    }
    TextureList->blockSignals(false);
}

void MainWindow::OnTextureSelected(int Index)
{
    SelectedIndex = Index;
    PreviewZoom = 1.0f;
    UpdatePreview();
}

void MainWindow::UpdatePreview()
{
    if (!HasSelection())
    {
        PreviewLabel->clear();
        InfoLabel->clear();
        return;
    }

    const auto& Item = Dictionary.Textures()[SelectedIndex];

    QString info = QString("<b>Name:</b> %1 | ").arg(QString::fromStdString(Item.Name()));
    if (!Item.MaskName().empty())
        info += QString("<b>Mask:</b> %1 | ").arg(QString::fromStdString(Item.MaskName()));
    info += QString("<b>Size:</b> %1 x %2 | <b>Format:</b> %3<br><b>Mip levels:</b> %4 | <b>Alpha:</b> %5 | <b>Zoom:</b> %6%")
                .arg(Item.Width()).arg(Item.Height())
                .arg(QString::fromStdString(Item.FormatName()))
                .arg(Item.MipLevels())
                .arg(Item.HasAlpha() ? "yes" : "no")
                .arg(static_cast<int>(PreviewZoom * 100.0f));
    InfoLabel->setText(info);

    RenderWare::Image img = Item.Pixels();
    if (img.IsEmpty()) {
        PreviewLabel->clear();
        return;
    }

    QImage qimg(img.Data(), img.Width(), img.Height(), QImage::Format_RGBA8888);
    
    QPixmap pixmap = QPixmap::fromImage(qimg);
    if (PreviewZoom != 1.0f)
    {
        pixmap = pixmap.scaled(pixmap.width() * PreviewZoom, pixmap.height() * PreviewZoom, Qt::KeepAspectRatio, Qt::FastTransformation);
    }
    PreviewLabel->setPixmap(pixmap);
}

void MainWindow::wheelEvent(QWheelEvent* event)
{
    if (!HasSelection()) return;
    
    float Wheel = event->angleDelta().y() / 120.0f;
    if (Wheel != 0.0f)
    {
        PreviewZoom *= (Wheel > 0.0f) ? 1.1f : (1.0f / 1.1f);
        if (PreviewZoom < 0.05f)
            PreviewZoom = 0.05f;
        if (PreviewZoom > 32.0f)
            PreviewZoom = 32.0f;
        UpdatePreview();
    }
}

void MainWindow::RequestOpenDialog()
{
    QString path = QFileDialog::getOpenFileName(this, "Open Texture Dictionary", "", "RenderWare TXD (*.txd)");
    if (!path.isEmpty())
        OpenFile(path.toStdString());
}

void MainWindow::RequestExportDialog()
{
    if (!HasSelection()) return;

    const auto& Item = Dictionary.Textures()[SelectedIndex];
    QString Suggested = QString::fromStdString(Item.Name().empty() ? "texture.png" : Item.Name() + ".png");

    QString path = QFileDialog::getSaveFileName(this, "Export Texture", Suggested, "PNG image (*.png)");
    if (path.isEmpty()) return;

    if (!path.endsWith(".png", Qt::CaseInsensitive))
        path += ".png";

    if (RenderWare::ImageWriter::SavePng(path.toStdString(), Item.Pixels()))
        StatusLabel->setText("Exported: " + path);
    else
        StatusLabel->setText("Export failed: " + path);
}

void MainWindow::RequestExportAllDialog()
{
    if (!Dictionary.IsLoaded() || Dictionary.Textures().empty()) return;

    QString dir = QFileDialog::getExistingDirectory(this, "Select Export Folder");
    if (dir.isEmpty()) return;

    int success = 0, failed = 0;
    for (const auto& Item : Dictionary.Textures())
    {
        QString name = QString::fromStdString(Item.Name().empty() ? "unnamed" : Item.Name());
        QString path = dir + "/" + name + ".png";

        if (RenderWare::ImageWriter::SavePng(path.toStdString(), Item.Pixels()))
            ++success;
        else
            ++failed;
    }

    QString msg = QString("Exported %1 texture(s).").arg(success);
    if (failed > 0)
        msg += QString(" Failed: %1.").arg(failed);
    StatusLabel->setText(msg);
}

void MainWindow::RequestAddDialog()
{
    if (!Dictionary.IsLoaded())
    {
        QMessageBox::warning(this, "Warning", "Please open or create a TXD file first.");
        return;
    }

    QString path = QFileDialog::getOpenFileName(this, "Add Texture", "", "Image files (*.png *.jpg *.jpeg *.bmp *.tga)");
    if (!path.isEmpty())
        ApplyAddition(path.toStdString());
}

void MainWindow::ApplyAddition(const std::string& ImagePath)
{
    if (!Dictionary.IsLoaded())
    {
        QMessageBox::warning(this, "Warning", "Please open or create a TXD file first.");
        return;
    }

    RenderWare::Image Loaded;
    if (!RenderWare::ImageReader::Load(ImagePath, Loaded))
    {
        StatusLabel->setText(QString::fromStdString("Failed to load image: " + ImagePath));
        return;
    }

    QFileInfo fileInfo(QString::fromStdString(ImagePath));

    bool Ok = false;
    QString Entered = QInputDialog::getText(this, "Add Texture", "Texture name:",
        QLineEdit::Normal, fileInfo.completeBaseName(), &Ok);
    if (!Ok)
        return;
    std::string baseName = Entered.left(31).toStdString();
    if (baseName.empty())
        return;

    if (Dictionary.AddTexture(baseName, Loaded))
    {
        SelectedIndex = Dictionary.Textures().size() - 1;
        UpdateTextureList();
        UpdatePreview();
        StatusLabel->setText(QString::fromStdString("Added texture: " + baseName));
    }
}

void MainWindow::RequestDeleteSelected()
{
    if (!HasSelection()) return;
    
    RenderWare::Texture deletedItem = Dictionary.Textures()[SelectedIndex];
    DeleteUndoStack.push_back({SelectedIndex, deletedItem});
    
    if (Dictionary.RemoveTexture(SelectedIndex))
    {
        if (SelectedIndex < 0 || static_cast<std::size_t>(SelectedIndex) >= Dictionary.Textures().size())
            SelectedIndex = Dictionary.Textures().size() - 1;
            
        UpdateTextureList();
        UpdatePreview();
        StatusLabel->setText("Deleted texture.");
    }
}

void MainWindow::UndoDelete()
{
    if (DeleteUndoStack.empty() || !Dictionary.IsLoaded()) return;
    
    auto lastDeleted = DeleteUndoStack.back();
    DeleteUndoStack.pop_back();
    
    if (Dictionary.InsertTexture(lastDeleted.first, lastDeleted.second))
    {
        SelectedIndex = lastDeleted.first;
        UpdateTextureList();
        UpdatePreview();
        StatusLabel->setText("Undid deletion.");
    }
}

void MainWindow::RequestReplaceDialog()
{
    if (!HasSelection()) return;

    QString path = QFileDialog::getOpenFileName(this, "Replace Texture", "", "Image files (*.png *.jpg *.jpeg *.bmp *.tga)");
    if (!path.isEmpty())
        ApplyReplacement(path.toStdString());
}

void MainWindow::ApplyReplacement(const std::string& ImagePath)
{
    RenderWare::Image Loaded;
    if (!RenderWare::ImageReader::Load(ImagePath, Loaded))
    {
        StatusLabel->setText(QString::fromStdString("Failed to load image: " + ImagePath));
        return;
    }

    if (Dictionary.ReplaceTexture(SelectedIndex, Loaded))
    {
        UpdatePreview();
        UpdateTextureList();
        StatusLabel->setText(QString::fromStdString("Replaced texture from " + ImagePath));
    }
}

void MainWindow::RequestSaveAsDialog()
{
    if (!Dictionary.IsLoaded()) return;

    QString Suggested = "texture.txd";
    QString Source = QString::fromStdString(Dictionary.SourcePath());
    int Slash = Source.lastIndexOf('/');
    if (Slash != -1)
        Suggested = Source.mid(Slash + 1);

    QString path = QFileDialog::getSaveFileName(this, "Save Texture Dictionary", Suggested, "RenderWare TXD (*.txd)");
    if (path.isEmpty()) return;

    if (!path.endsWith(".txd", Qt::CaseInsensitive))
        path += ".txd";

    std::string Error;
    if (Dictionary.SaveToFile(path.toStdString(), Error))
    {
        StatusLabel->setText("Saved: " + path);
        UpdateTextureList();
    }
    else
        StatusLabel->setText(QString::fromStdString("Save failed: " + Error));
}

void MainWindow::SaveCurrent()
{
    if (!Dictionary.IsLoaded()) return;

    std::string Error;
    if (Dictionary.SaveToFile(Dictionary.SourcePath(), Error))
    {
        StatusLabel->setText(QString::fromStdString("Saved: " + Dictionary.SourcePath()));
        UpdateTextureList();
    }
    else
        StatusLabel->setText(QString::fromStdString("Save failed: " + Error));
}

void MainWindow::RequestResize()
{
    if (!HasSelection()) return;

    const auto& Item = Dictionary.Textures()[SelectedIndex];
    bool ok;
    int width = QInputDialog::getInt(this, "Resize Texture", "Width:", Item.Width(), 1, 4096, 1, &ok);
    if (!ok) return;
    int height = QInputDialog::getInt(this, "Resize Texture", "Height:", Item.Height(), 1, 4096, 1, &ok);
    if (!ok) return;

    int FinalWidth = NearestPowerOfTwo(width);
    int FinalHeight = NearestPowerOfTwo(height);
    Dictionary.ResizeTexture(SelectedIndex, FinalWidth, FinalHeight);
    UpdatePreview();
    UpdateTextureList();
    StatusLabel->setText(QString::fromStdString("Resized to " + std::to_string(FinalWidth) + " x " + std::to_string(FinalHeight)));
}

void MainWindow::SetCompressionDxt1() { if (HasSelection()) Dictionary.SetTextureCompression(SelectedIndex, CompressionType::Dxt1); }
void MainWindow::SetCompressionDxt3() { if (HasSelection()) Dictionary.SetTextureCompression(SelectedIndex, CompressionType::Dxt3); }
void MainWindow::SetCompressionDxt5() { if (HasSelection()) Dictionary.SetTextureCompression(SelectedIndex, CompressionType::Dxt5); }
void MainWindow::SetCompressionNone() { if (HasSelection()) Dictionary.SetTextureCompression(SelectedIndex, CompressionType::None); }

void MainWindow::CreateMipLevels()
{
    if (!HasSelection())
        return;
    if (Dictionary.SetTextureMipmaps(SelectedIndex, true))
    {
        UpdatePreview();
        UpdateTextureList();
        StatusLabel->setText(QString("Generated %1 mip levels").arg(Dictionary.Textures()[SelectedIndex].MipLevels()));
    }
}

void MainWindow::ClearMipLevels()
{
    if (!HasSelection())
        return;
    if (Dictionary.SetTextureMipmaps(SelectedIndex, false))
    {
        UpdatePreview();
        UpdateTextureList();
        StatusLabel->setText("Cleared mip levels");
    }
}

void MainWindow::RequestRename()
{
    if (!HasSelection())
        return;

    const RenderWare::Texture& Item = Dictionary.Textures()[SelectedIndex];

    bool Ok = false;
    QString Name = QInputDialog::getText(this, "Rename Texture", "Texture name:",
        QLineEdit::Normal, QString::fromStdString(Item.Name()), &Ok);
    if (!Ok)
        return;
    Name = Name.left(31);

    QString Mask = QInputDialog::getText(this, "Rename Texture", "Mask name (optional):",
        QLineEdit::Normal, QString::fromStdString(Item.MaskName()), &Ok);
    if (!Ok)
        return;
    Mask = Mask.left(31);

    if (Dictionary.RenameTexture(SelectedIndex, Name.toStdString(), Mask.toStdString()))
    {
        UpdateTextureList();
        UpdatePreview();
        StatusLabel->setText("Renamed to " + Name);
    }
}
