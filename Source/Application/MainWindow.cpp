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
#include <QSettings>

#include "RenderWare/ImageReader.h"
#include "RenderWare/ImageWriter.h"
#include "HotkeyDialog.h"

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
    LoadSettings();
    ApplyTheme();
}

void MainWindow::SetupUi()
{
    setWindowTitle("UnknownTxd");
    resize(1100, 720);

    QSplitter* splitter = new QSplitter(Qt::Horizontal, this);
    setCentralWidget(splitter);

    QWidget* leftWidget = new QWidget(this);
    QVBoxLayout* leftLayout = new QVBoxLayout(leftWidget);
    leftLayout->setContentsMargins(0, 0, 0, 0);

    SearchBar = new QLineEdit(this);
    SearchBar->setPlaceholderText("Search...");
    connect(SearchBar, &QLineEdit::textChanged, this, &MainWindow::OnSearchTextChanged);
    leftLayout->addWidget(SearchBar);

    TextureList = new QListWidget(this);
    TextureList->setAlternatingRowColors(true);
    TextureList->setSelectionMode(QAbstractItemView::ExtendedSelection);
    TextureList->setDragDropMode(QAbstractItemView::InternalMove);
    connect(TextureList->model(), &QAbstractItemModel::rowsMoved, this, &MainWindow::OnTextureMoved);
    connect(TextureList, &QListWidget::currentRowChanged, this, &MainWindow::OnTextureSelected);
    leftLayout->addWidget(TextureList);

    splitter->addWidget(leftWidget);

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

    ActionHotkeySettings = MenuView->addAction("Hotkey Settings");
    connect(ActionHotkeySettings, &QAction::triggered, this, &MainWindow::RequestHotkeySettings);

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
        ActionHotkeySettings->setText("Hotkey Settings");
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
        ActionHotkeySettings->setText("Настройки горячих клавиш");
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

void MainWindow::closeEvent(QCloseEvent* event)
{
    SaveSettings();
    QMainWindow::closeEvent(event);
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
    QList<int> selectedRows;
    for (auto item : TextureList->selectedItems()) {
        selectedRows.append(TextureList->row(item));
    }

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
    
    if (!Textures.empty())
    {
        if (SelectedIndex < 0 || static_cast<std::size_t>(SelectedIndex) >= Textures.size()) {
            SelectedIndex = 0;
        }
        
        if (selectedRows.isEmpty()) {
            TextureList->setCurrentRow(SelectedIndex);
        } else {
            for (int r : selectedRows) {
                if (r >= 0 && r < TextureList->count()) {
                    TextureList->item(r)->setSelected(true);
                }
            }
            TextureList->setCurrentItem(TextureList->item(SelectedIndex));
        }
    }
    else
    {
        SelectedIndex = -1;
        UpdatePreview();
    }
    TextureList->blockSignals(false);
    OnSearchTextChanged(SearchBar->text());
}

void MainWindow::OnTextureSelected(int Index)
{
    SelectedIndex = Index;
    PreviewZoom = 1.0f;
    UpdatePreview();
}

void MainWindow::OnSearchTextChanged(const QString& text)
{
    QString lowerText = text.toLower();
    for (int i = 0; i < TextureList->count(); ++i)
    {
        QListWidgetItem* item = TextureList->item(i);
        item->setHidden(!item->text().toLower().contains(lowerText));
    }
}

void MainWindow::OnTextureMoved(const QModelIndex &, int start, int end, const QModelIndex &, int row)
{
    if (start == row || start == row - 1) return;
    int toIndex = (row > start) ? row - 1 : row;
    
    if (Dictionary.MoveTexture(start, toIndex))
    {
        SelectedIndex = toIndex;
        // Don't call UpdateTextureList() because QListWidget already moved the item!
        // But we should update selection silently if needed.
        TextureList->blockSignals(true);
        TextureList->setCurrentRow(SelectedIndex);
        TextureList->blockSignals(false);
    }
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
    auto items = TextureList->selectedItems();
    if (items.isEmpty()) return;

    if (items.size() == 1) {
        const auto& Item = Dictionary.Textures()[TextureList->row(items.first())];
        QString Suggested = QString::fromStdString(Item.Name().empty() ? "texture.png" : Item.Name() + ".png");

        QString path = QFileDialog::getSaveFileName(this, "Export Texture", Suggested, "PNG image (*.png)");
        if (path.isEmpty()) return;

        if (!path.endsWith(".png", Qt::CaseInsensitive))
            path += ".png";

        if (RenderWare::ImageWriter::SavePng(path.toStdString(), Item.Pixels()))
            StatusLabel->setText("Exported: " + path);
        else
            StatusLabel->setText("Export failed: " + path);
    } else {
        QString dir = QFileDialog::getExistingDirectory(this, "Select Export Folder for Selected");
        if (dir.isEmpty()) return;

        int success = 0, failed = 0;
        for (auto item : items) {
            const auto& Item = Dictionary.Textures()[TextureList->row(item)];
            QString name = QString::fromStdString(Item.Name().empty() ? "unnamed" : Item.Name());
            QString path = dir + "/" + name + ".png";

            if (RenderWare::ImageWriter::SavePng(path.toStdString(), Item.Pixels())) success++;
            else failed++;
        }
        QString msg = QString("Exported %1 selected texture(s).").arg(success);
        if (failed > 0) msg += QString(" Failed: %1.").arg(failed);
        StatusLabel->setText(msg);
    }
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
        
        bool hasAlpha = Dictionary.Textures()[SelectedIndex].HasAlpha();
        Dictionary.SetTextureCompression(SelectedIndex, hasAlpha ? CompressionType::Dxt5 : CompressionType::Dxt1);

        UpdateTextureList();
        UpdatePreview();
        StatusLabel->setText(QString::fromStdString("Added texture: " + baseName + (hasAlpha ? " (DXT5)" : " (DXT1)")));
    }
}

void MainWindow::RequestDeleteSelected()
{
    auto selectedItems = TextureList->selectedItems();
    if (selectedItems.isEmpty()) return;
    
    QList<int> indices;
    for (auto item : selectedItems) indices.append(TextureList->row(item));
    std::sort(indices.begin(), indices.end(), std::greater<int>());
    
    int deletedCount = 0;
    for (int i : indices) {
        RenderWare::Texture deletedItem = Dictionary.Textures()[i];
        DeleteUndoStack.push_back({i, deletedItem});
        
        if (Dictionary.RemoveTexture(i)) {
            deletedCount++;
        }
    }
    
    if (deletedCount > 0) {
        if (SelectedIndex < 0 || static_cast<std::size_t>(SelectedIndex) >= Dictionary.Textures().size())
            SelectedIndex = Dictionary.Textures().size() - 1;
            
        UpdateTextureList();
        UpdatePreview();
        StatusLabel->setText(QString("Deleted %1 texture(s).").arg(deletedCount));
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
    auto items = TextureList->selectedItems();
    if (items.isEmpty()) return;

    int firstIndex = TextureList->row(items.first());
    const auto& Item = Dictionary.Textures()[firstIndex];
    bool ok;
    int width = QInputDialog::getInt(this, "Resize Texture(s)", "Width:", Item.Width(), 1, 4096, 1, &ok);
    if (!ok) return;
    int height = QInputDialog::getInt(this, "Resize Texture(s)", "Height:", Item.Height(), 1, 4096, 1, &ok);
    if (!ok) return;

    int FinalWidth = NearestPowerOfTwo(width);
    int FinalHeight = NearestPowerOfTwo(height);
    
    for (auto item : items) {
        Dictionary.ResizeTexture(TextureList->row(item), FinalWidth, FinalHeight);
    }
    
    UpdatePreview();
    UpdateTextureList();
    StatusLabel->setText(QString("Resized %1 texture(s) to %2 x %3").arg(items.size()).arg(FinalWidth).arg(FinalHeight));
}

void MainWindow::SetCompressionDxt1() { 
    auto items = TextureList->selectedItems();
    for (auto item : items) Dictionary.SetTextureCompression(TextureList->row(item), CompressionType::Dxt1);
    if (!items.isEmpty()) { UpdatePreview(); UpdateTextureList(); StatusLabel->setText(QString("Set DXT1 to %1 texture(s)").arg(items.size())); }
}
void MainWindow::SetCompressionDxt3() { 
    auto items = TextureList->selectedItems();
    for (auto item : items) Dictionary.SetTextureCompression(TextureList->row(item), CompressionType::Dxt3);
    if (!items.isEmpty()) { UpdatePreview(); UpdateTextureList(); StatusLabel->setText(QString("Set DXT3 to %1 texture(s)").arg(items.size())); }
}
void MainWindow::SetCompressionDxt5() { 
    auto items = TextureList->selectedItems();
    for (auto item : items) Dictionary.SetTextureCompression(TextureList->row(item), CompressionType::Dxt5);
    if (!items.isEmpty()) { UpdatePreview(); UpdateTextureList(); StatusLabel->setText(QString("Set DXT5 to %1 texture(s)").arg(items.size())); }
}
void MainWindow::SetCompressionNone() { 
    auto items = TextureList->selectedItems();
    for (auto item : items) Dictionary.SetTextureCompression(TextureList->row(item), CompressionType::None);
    if (!items.isEmpty()) { UpdatePreview(); UpdateTextureList(); StatusLabel->setText(QString("Set RGBA8888 to %1 texture(s)").arg(items.size())); }
}

void MainWindow::CreateMipLevels()
{
    auto items = TextureList->selectedItems();
    int count = 0;
    for (auto item : items) {
        if (Dictionary.SetTextureMipmaps(TextureList->row(item), true)) count++;
    }
    if (count > 0) {
        UpdatePreview(); UpdateTextureList(); StatusLabel->setText(QString("Generated mip levels for %1 texture(s)").arg(count));
    }
}

void MainWindow::ClearMipLevels()
{
    auto items = TextureList->selectedItems();
    int count = 0;
    for (auto item : items) {
        if (Dictionary.SetTextureMipmaps(TextureList->row(item), false)) count++;
    }
    if (count > 0) {
        UpdatePreview(); UpdateTextureList(); StatusLabel->setText(QString("Cleared mip levels for %1 texture(s)").arg(count));
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

void MainWindow::LoadSettings()
{
    QSettings settings("UnknownTeam", "UnknownTxd");
    CurrentLanguage = static_cast<Language>(settings.value("Language", static_cast<int>(Language::English)).toInt());
    IsDarkTheme = settings.value("DarkTheme", true).toBool();
    
    if (settings.contains("Shortcut_Add")) ActionAdd->setShortcut(QKeySequence(settings.value("Shortcut_Add").toString()));
    if (settings.contains("Shortcut_Rename")) ActionRename->setShortcut(QKeySequence(settings.value("Shortcut_Rename").toString()));
    if (settings.contains("Shortcut_Replace")) ActionReplace->setShortcut(QKeySequence(settings.value("Shortcut_Replace").toString()));
    if (settings.contains("Shortcut_Delete")) ActionDelete->setShortcut(QKeySequence(settings.value("Shortcut_Delete").toString()));
    if (settings.contains("Shortcut_Export")) ActionExport->setShortcut(QKeySequence(settings.value("Shortcut_Export").toString()));
    if (settings.contains("Shortcut_ExportAll")) ActionExportAll->setShortcut(QKeySequence(settings.value("Shortcut_ExportAll").toString()));
    if (settings.contains("Shortcut_Resize")) ActionResize->setShortcut(QKeySequence(settings.value("Shortcut_Resize").toString()));
    
    RetranslateUi();
}

void MainWindow::SaveSettings()
{
    QSettings settings("UnknownTeam", "UnknownTxd");
    settings.setValue("Language", static_cast<int>(CurrentLanguage));
    settings.setValue("DarkTheme", IsDarkTheme);
    
    settings.setValue("Shortcut_Add", ActionAdd->shortcut().toString());
    settings.setValue("Shortcut_Rename", ActionRename->shortcut().toString());
    settings.setValue("Shortcut_Replace", ActionReplace->shortcut().toString());
    settings.setValue("Shortcut_Delete", ActionDelete->shortcut().toString());
    settings.setValue("Shortcut_Export", ActionExport->shortcut().toString());
    settings.setValue("Shortcut_ExportAll", ActionExportAll->shortcut().toString());
    settings.setValue("Shortcut_Resize", ActionResize->shortcut().toString());
}

void MainWindow::RequestHotkeySettings()
{
    QList<QAction*> editActions = {
        ActionAdd, ActionRename, ActionReplace, ActionDelete,
        ActionExport, ActionExportAll, ActionResize
    };
    
    HotkeyDialog dialog(editActions, this);
    if (dialog.exec() == QDialog::Accepted)
    {
        auto newShortcuts = dialog.getNewShortcuts();
        for (auto it = newShortcuts.begin(); it != newShortcuts.end(); ++it)
        {
            it.key()->setShortcut(it.value());
        }
    }
}

