#include <QAction>
#include <QApplication>
#include <QComboBox>
#include <QFileDialog>
#include <QFileSystemModel>
#include <QFont>
#include <QFontDialog>
#include <QIcon>
#include <QLineEdit>
#include <QMainWindow>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QPainter>
#include <QProcess>
#include <QScrollBar>
#include <QSettings>
#include <QSplitter>
#include <QStatusBar>
#include <QTextBlock>
#include <QTextEdit>
#include <QTimer>
#include <QToolBar>
#include <QTreeView>
#include <QVBoxLayout>

#include "editor.hpp"

class EditorApp : public QMainWindow {
  Q_OBJECT

signals:
  void fileSaved();

public:
  EditorApp(QWidget *parent = nullptr) : QMainWindow(parent) {
    setupUi();
    // Disable the signal to avoid emitting textChanged signal
    configureEditor();
    setupActions();
    setupMenus();
    setupToolBar();
    loadSettings();
    setupShortcuts();

    // Set the focus to the text editor
    fontDialog->setCurrentFont(textEditor->font());
    textEditor->setFocus();
    textEditor->moveCursor(QTextCursor::End);

    isDirty = false;

    // connect textChanged after 500ms to avoid emitting the signal when loading
    // a file
    QTimer::singleShot(
        500, [this] { connect(textEditor, &QTextEdit::textChanged, [this] { isDirty = true; }); });
  }

  ~EditorApp() override { saveSettings(); }

  // Sets and opens the current file if passed from the command line
  void setCurrentFile(const QString &fileName) {
    currentFile = fileName;

    // If the file exists, open it
    if (QFile::exists(fileName)) {
      openFile(fileName);
    } else {
      QFile file(fileName);
      if (file.open(QFile::WriteOnly | QFile::Text)) {
        file.write("");
        file.close();
        openFile(fileName);
      }
    }
  }

private:
  QSplitter *mainSplitter;        // main splitter for the file tree and text editor
  QTreeView *fileTree;            // file tree view
  AutoIndentTextEdit *textEditor; // text editor for code editing
  QTextEdit *outputView;          // output view for the compiler and run process
  QTextEdit *disAssemblyView;     // disassembly view for the compiled program
  QFileSystemModel *fileModel;    // model for the file tree
  QProcess *compileProcess;       // process to compile the program
  QProcess *runProcess;           // process to run the compiled program
  QProcess *clangFormat;          // process to format the code
  QProcess *disAssembleProcess;   // process to format the code
  QString currentFile;            // current file being edited
  QStringList extraFiles;         // extra files to be compiled
  QStringList recentFiles;        // recently opened files
  QString compiler;               // compiler to use
  QStringList cFlags;             // compiler flags
  QStringList ldFlags;            // linker flags

  // Actions
  QAction *actionOpen;
  QAction *actionSave;
  QAction *actionSaveAs;
  QAction *actionExit;
  QAction *actionUndo;
  QAction *actionRedo;
  QAction *actionCut;
  QAction *actionCopy;
  QAction *actionPaste;

  // Actions for the toolbar
  QAction *actionCompileAndRun;
  QAction *actionNew;
  QAction *actionRecentFiles;
  QAction *actionFormatCode;
  QAction *actionBuild, *actionRun, *actionDisassemble;

  // formatOnSave toggle
  QAction *actionFormatOnSave;

  // select widget for the compiler
  QComboBox *compilerSelect;

  // line edit for the compiler flags
  QLineEdit *cFlagsEdit;

  // line edit for the linker flags
  QLineEdit *ldFlagsEdit;

  // Font for the text editor
  QFont currentFont;

  // fontSelect button
  QAction *fontSelect;

  // font selection dialog
  QFontDialog *fontDialog;

  // Track if the editor is dirty
  bool isDirty = false;

  void setupUi() {
    mainSplitter = new QSplitter(Qt::Horizontal, this);

    fileTree  = new QTreeView(mainSplitter);
    fileModel = new QFileSystemModel(this);
    fileModel->setRootPath(QDir::rootPath());
    fileTree->setModel(fileModel);
    fileTree->setRootIndex(fileModel->index(QDir::currentPath()));

    // show hidden files
    fileModel->setFilter(QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden);

    // set icons
    fileTree->setAnimated(false);
    fileTree->setIndentation(20);
    fileTree->setSortingEnabled(true);
    fileTree->setColumnWidth(0, 250);

    connect(fileTree, &QTreeView::doubleClicked, this, &EditorApp::onFileSelected);
    // On Enter key press, open the file
    connect(fileTree, &QTreeView::activated, this, &EditorApp::onFileSelected);

    auto *rightSplitter = new QSplitter(Qt::Vertical, mainSplitter);

    // Set the syntax highlighter
    textEditor       = new AutoIndentTextEdit(rightSplitter);
    auto highlighter = new DraculaCppSyntaxHighlighter(textEditor->document());
    textEditor->setHighlighter(highlighter);

    outputView = new QTextEdit(rightSplitter);
    outputView->setReadOnly(true);

    mainSplitter->addWidget(fileTree);
    mainSplitter->addWidget(rightSplitter);

    // Set the text editor to take 70% of the available space
    mainSplitter->setStretchFactor(0, 1);
    mainSplitter->setStretchFactor(1, 6);
    mainSplitter->setStretchFactor(2, 3);

    // let the output view take 20% of the available space vertically
    rightSplitter->setStretchFactor(0, 8);
    rightSplitter->setStretchFactor(1, 2);

    // Create a disassembly view
    disAssemblyView = new QTextEdit(mainSplitter);
    disAssemblyView->setReadOnly(true);
    disAssemblyView->setLineWrapMode(QTextEdit::NoWrap);
    disAssemblyView->setStyleSheet(
        "QTextEdit {"
        "  background-color: #282a36;"
        "  color: #faede3;"
        "  selection-background-color: #6272a4;"
        "  selection-color: #f8f8f2;"
        "}");

    // disable the disassembly view by default
    mainSplitter->setStretchFactor(2, 0);

    // set the central widget
    setCentralWidget(mainSplitter);

    setWindowTitle("Edit");
    resize(800, 600);

    // Initialize the processes
    compileProcess     = new QProcess(this);
    runProcess         = new QProcess(this);
    clangFormat        = new QProcess(this);
    disAssembleProcess = new QProcess(this);

    // edits for compiler and flags
    compilerSelect = new QComboBox(this);
    compilerSelect->addItems({"gcc", "g++", "clang", "clang++"});
    compilerSelect->setCurrentText("gcc");

    cFlagsEdit = new QLineEdit(this);
    cFlagsEdit->setPlaceholderText("Compiler flags");

    ldFlagsEdit = new QLineEdit(this);
    ldFlagsEdit->setPlaceholderText("Linker flags");

    fontDialog = new QFontDialog(this);

    connect(fontDialog, &QFontDialog::fontSelected, [this](const QFont &font) {
      if (font != currentFont) {
        textEditor->setFont(font);
        currentFont = font;
      }
    });

    fontSelect = new QAction(QIcon::fromTheme("format-text-bold"), tr("Font"), this);
    connect(fontSelect, &QAction::triggered, [this] { fontDialog->open(); });

    // Add the compiler and flags to the status bar
    statusBar()->addPermanentWidget(compilerSelect);
    statusBar()->addPermanentWidget(cFlagsEdit);
    statusBar()->addPermanentWidget(ldFlagsEdit);

    connect(compilerSelect, &QComboBox::currentTextChanged, [this](const QString &text) {
      compiler = text;
      statusBar()->showMessage(tr("Compiler changed to %1").arg(text), 2000);
    });

    connect(cFlagsEdit, &QLineEdit::textChanged,
            [this](const QString &text) { cFlags = text.split(" ", Qt::SkipEmptyParts); });

    connect(ldFlagsEdit, &QLineEdit::textChanged,
            [this](const QString &text) { ldFlags = text.split(" ", Qt::SkipEmptyParts); });
  }

  void configureEditor() {
    // Set the tab size to 4 spaces
    // Set font family as JetBrainsMonoNL Nerd Font Mono
    QFont font = textEditor->font();
    font.setFamily("JetBrainsMonoNL Nerd Font Mono");
    font.setFixedPitch(true);
    font.setPointSize(18);
    font.setWeight(QFont::Normal);

    // configure fileTree
    fileTree->setRootIndex(fileModel->index(QDir::currentPath()));
    fileTree->setAnimated(false);
    fileTree->setIndentation(20);
    fileTree->setSortingEnabled(true);
    fileTree->setColumnWidth(0, 250);
    fileTree->setHeaderHidden(true);
    fileTree->hideColumn(1);
    fileTree->hideColumn(2);
    fileTree->hideColumn(3);

    // configure outputView
    outputView->setReadOnly(true);
    outputView->setLineWrapMode(QTextEdit::NoWrap);
    // outputView->setStyleSheet("background-color: #282a36; color: #f8f8f2;");
    // Make less dark than the text editor
    outputView->setStyleSheet(
        "QTextEdit {"
        "  background-color: #44475a;"
        "  color: #f8f8f2;"
        "  selection-background-color: #6272a4;"
        "  selection-color: #f8f8f2;"
        "}");

    outputView->setFont(font);

    // allow clearing the output view
    outputView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(outputView, &QTextEdit::customContextMenuRequested, [this](const QPoint &pos) {
      QMenu *currentMenu = outputView->createStandardContextMenu();
      //   set pointer cursor to the menu
      currentMenu->setCursor(Qt::PointingHandCursor);

      auto *clearAction = new QAction(QIcon::fromTheme("edit-clear"), "Clear", this);
      connect(clearAction, &QAction::triggered, [this] { outputView->clear(); });
      currentMenu->addAction(clearAction);
      currentMenu->exec(outputView->mapToGlobal(pos));
      currentMenu->setStyleSheet("QMenu::item { padding: 5px 20px; }");
    });
  }

  void setupShortcuts() {
    // Ctrl + ` to toggle the output view
    auto *toggleOutputView = new QAction(this);
    toggleOutputView->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_QuoteLeft));
    connect(toggleOutputView, &QAction::triggered,
            [this] { outputView->isHidden() ? outputView->show() : outputView->hide(); });
    addAction(toggleOutputView);
  }

  void setupActions() {
    // File actions
    actionOpen = new QAction(QIcon::fromTheme("document-open"), tr("&Open..."), this);
    actionOpen->setShortcut(QKeySequence::Open);
    connect(actionOpen, &QAction::triggered, this, &EditorApp::openFileDialog);

    actionSave = new QAction(QIcon::fromTheme("document-save"), tr("&Save"), this);
    actionSave->setShortcut(QKeySequence::Save);
    connect(actionSave, &QAction::triggered, this, &EditorApp::saveFile);

    actionSaveAs = new QAction(QIcon::fromTheme("document-save-as"), tr("Save &As..."), this);
    actionSaveAs->setShortcut(QKeySequence::SaveAs);
    connect(actionSaveAs, &QAction::triggered, this, &EditorApp::saveFileAs);

    actionExit = new QAction(QIcon::fromTheme("application-exit"), tr("E&xit"), this);
    actionExit->setShortcut(QKeySequence::Quit);
    connect(actionExit, &QAction::triggered, this, &EditorApp::close);

    // Edit actions
    actionUndo = new QAction(QIcon::fromTheme("edit-undo"), tr("&Undo"), this);
    actionUndo->setShortcut(QKeySequence::Undo);
    actionRedo = new QAction(QIcon::fromTheme("edit-redo"), tr("&Redo"), this);
    actionRedo->setShortcut(QKeySequence::Redo);
    actionCut = new QAction(QIcon::fromTheme("edit-cut"), tr("Cu&t"), this);
    actionCut->setShortcut(QKeySequence::Cut);
    actionCopy = new QAction(QIcon::fromTheme("edit-copy"), tr("&Copy"), this);
    actionCopy->setShortcut(QKeySequence::Copy);
    actionPaste = new QAction(QIcon::fromTheme("edit-paste"), tr("&Paste"), this);
    actionPaste->setShortcut(QKeySequence::Paste);

    connect(actionUndo, &QAction::triggered, textEditor, &QTextEdit::undo);
    connect(actionRedo, &QAction::triggered, textEditor, &QTextEdit::redo);
    connect(actionCut, &QAction::triggered, textEditor, &QTextEdit::cut);
    connect(actionCopy, &QAction::triggered, textEditor, &QTextEdit::copy);
    connect(actionPaste, &QAction::triggered, textEditor, &QTextEdit::paste);

    // Build actions
    actionCompileAndRun = new QAction(QIcon::fromTheme("system-run"), tr("&Compile and Run"), this);
    actionCompileAndRun->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_B));
    connect(actionCompileAndRun, &QAction::triggered, this, &EditorApp::compileAndRun);

    actionFormatCode = new QAction(QIcon::fromTheme("format-indent-more"), tr("Format Code"), this);
    actionFormatCode->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_I));
    connect(actionFormatCode, &QAction::triggered, this, &EditorApp::formatCode);

    actionBuild = new QAction(QIcon::fromTheme("media-playback-start"), tr("Build"), this);
    actionBuild->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_B));
    connect(actionBuild, &QAction::triggered, this, &EditorApp::compile);

    actionRun = new QAction(QIcon::fromTheme("media-playback-start"), tr("Run"), this);
    actionRun->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_R));
    connect(actionRun, &QAction::triggered, this, &EditorApp::run);

    actionDisassemble =
        new QAction(QIcon::fromTheme("media-playback-start"), tr("Disassemble"), this);
    actionDisassemble->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_D));
    connect(actionDisassemble, &QAction::triggered, this, &EditorApp::disassemble);

    actionNew = new QAction(QIcon::fromTheme("document-new"), tr("&New"), this);
    actionNew->setShortcut(QKeySequence::New);

    connect(actionNew, &QAction::triggered, [this] {
      textEditor->clear();
      currentFile.clear();

      // clear the output view
      outputView->clear();

      statusBar()->showMessage(tr("New file created"), 2000);

      // set the focus to the text editor
      textEditor->setFocus();

      // update the window title
      setWindowTitle("untitled - Edit");
    });

    actionRecentFiles =
        new QAction(QIcon::fromTheme("document-open-recent"), tr("Recent Files"), this);
    actionRecentFiles->setEnabled(false);
    actionRecentFiles->setMenu(new QMenu(this));

    // Format on save action
    actionFormatOnSave = new QAction(tr("Format on Save"), this);
    actionFormatOnSave->setCheckable(true);
    actionFormatOnSave->setChecked(true);
  }

  void setupMenus() {
    QMenu *fileMenu = menuBar()->addMenu(tr("&File"));
    fileMenu->addAction(actionNew);
    fileMenu->addAction(actionOpen);
    fileMenu->addSeparator();
    fileMenu->addAction(actionSave);
    fileMenu->addAction(actionSaveAs);
    fileMenu->addSeparator();
    fileMenu->addAction(actionRecentFiles);
    fileMenu->addAction(actionExit);

    QMenu *editMenu = menuBar()->addMenu(tr("&Edit"));
    editMenu->addAction(actionUndo);
    editMenu->addAction(actionRedo);
    editMenu->addSeparator();
    editMenu->addAction(actionCut);
    editMenu->addAction(actionCopy);
    editMenu->addAction(actionPaste);
    editMenu->addSeparator();
    editMenu->addAction(actionFormatOnSave);

    QMenu *buildMenu = menuBar()->addMenu(tr("&Build"));
    buildMenu->addAction(actionCompileAndRun);
    buildMenu->addAction(actionBuild);
    buildMenu->addAction(actionRun);
    buildMenu->addAction(actionDisassemble);
    buildMenu->addSeparator();
    buildMenu->addAction(actionFormatCode);
  }

  void setupToolBar() {
    QToolBar *toolBar = addToolBar(tr("Main"));
    toolBar->setObjectName("MainToolBar");
    toolBar->setLayoutDirection(Qt::LeftToRight);
    toolBar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    toolBar->setMovable(false);

    // Add actions to the toolbar
    toolBar->addAction(actionNew);
    toolBar->addAction(actionOpen);
    toolBar->addAction(actionSave);
    toolBar->addSeparator();
    toolBar->addAction(actionCompileAndRun);
    toolBar->addAction(actionBuild);
    toolBar->addAction(actionRun);
    toolBar->addAction(actionDisassemble);
    toolBar->addSeparator();
    toolBar->addAction(actionFormatCode);
    toolBar->addSeparator();
    toolBar->addAction(fontSelect);
  }

  void loadSettings() {
    QSettings settings("Yo Medical Files (U) LTD", "Edit");
    restoreGeometry(settings.value("geometry").toByteArray());
    restoreState(settings.value("windowState").toByteArray());

    // Load recently opened file
    recentFiles = settings.value("recentFiles").toStringList();
    recentFiles.removeDuplicates();

    if (!recentFiles.isEmpty()) {
      openFile(recentFiles.first());
    }

    actionRecentFiles->setEnabled(!recentFiles.isEmpty());
    if (!recentFiles.isEmpty()) {
      // Populate the recent files menu
      auto *recentMenu = new QMenu(tr("Recent Files"), this);
      for (const QString &file : recentFiles) {
        QAction *action = recentMenu->addAction(file);
        connect(action, &QAction::triggered, [this, file] { openFile(file); });
      }
      actionRecentFiles->setMenu(recentMenu);
    }

    // Load the compiler and flags
    compiler = settings.value("compiler", "gcc").toString();
    cFlags   = settings.value("cFlags", QStringList({"-Wall", "-Werror", "-Wextra", "-O3"}))
                 .toStringList();
    ldFlags = settings.value("ldFlags", QStringList({"-lm", "-lpthread"})).toStringList();

    compilerSelect->setCurrentText(compiler);
    cFlagsEdit->setText(cFlags.join(" "));
    ldFlagsEdit->setText(ldFlags.join(" "));

    actionFormatOnSave->setChecked(settings.value("formatOnSave", true).toBool());

    // font settings
    currentFont =
        settings.value("font", QFont("JetBrainsMonoNL Nerd Font Mono", 18)).value<QFont>();
    textEditor->setFont(currentFont);
    fontDialog->setCurrentFont(currentFont);

    qDebug() << "Loaded Font: " << currentFont;
  }

  void saveSettings() {
    QSettings settings("Yo Medical Files (U) LTD", "Edit");
    settings.setValue("geometry", saveGeometry());
    settings.setValue("windowState", saveState());
    settings.setValue("recentFiles", recentFiles);

    // Save the current file
    if (!currentFile.isEmpty()) {
      recentFiles.prepend(currentFile);
      recentFiles.removeDuplicates();

      if (recentFiles.size() > 5) {
        recentFiles.removeLast();
      }
    }

    settings.setValue("compiler", compiler);
    settings.setValue("cFlags", cFlags);
    settings.setValue("ldFlags", ldFlags);
    settings.setValue("formatOnSave", actionFormatOnSave->isChecked());

    // Font settings
    settings.setValue("font", currentFont);
    qDebug() << "Saved Font: " << currentFont;
    settings.sync();
  }

  QString getBaseName(const QString &fileName) { return QFileInfo(fileName).baseName(); }

private slots:

  void onFileSelected(const QModelIndex &index) {
    if (!fileModel->isDir(index)) {
      currentFile = fileModel->filePath(index);
      openFile(currentFile);
    }
  }

  void openFileDialog() {
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open File"), QDir::currentPath());
    if (!fileName.isEmpty()) {
      openFile(fileName);
    }
  }

  void openFile(const QString &fileName) {
    QFile file(fileName);
    if (file.open(QFile::ReadOnly | QFile::Text)) {
      // block signals to avoid emitting textChanged signal
      // otherwise the editor will be marked as dirty when loading a file
      textEditor->blockSignals(true);
      textEditor->setPlainText(file.readAll());
      textEditor->blockSignals(false);
      isDirty = false;

      currentFile = fileName;
      statusBar()->showMessage(tr("File loaded"), 2000);
      file.close();

      // Add the file to the recent files list
      if (recentFiles.contains(currentFile)) {
        recentFiles.removeAll(currentFile);
      }
      recentFiles.prepend(currentFile);

      // update the window title
      setWindowTitle(QString("%1 - Edit").arg(currentFile));

      // emit the fileSaved signal to format the code if formatOnSave is enabled
      if (actionFormatOnSave->isChecked()) {
        formatCode();
      }
    } else {
      QMessageBox::warning(this, tr("Error"), tr("Could not open file"));
    }
  }

  void saveFile() {
    if (currentFile.isEmpty()) {
      saveFileAs();
    } else {
      saveToFile(currentFile);
    }

    isDirty = false;
    if (actionFormatOnSave->isChecked()) {
      formatCode();
    }
  }

  void saveFileAs() {
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save File As"), QDir::currentPath());
    if (!fileName.isEmpty()) {
      saveToFile(fileName);
    }
  }

  void saveToFile(const QString &fileName) {
    QFile file(fileName);
    if (file.open(QFile::WriteOnly | QFile::Text)) {
      file.write(textEditor->toPlainText().toUtf8());
      currentFile = fileName;
      statusBar()->showMessage(tr("File saved"), 2000);
    } else {
      QMessageBox::warning(this, tr("Error"), tr("Could not save file"));
    }
  }

  void compileAndRun() {
    if (currentFile.isEmpty()) {
      QMessageBox::warning(this, tr("Error"), tr("No file to compile"));
      return;
    }

    if (compile()) {
      run();
    }
  }

  void updateOutput() {
    outputView->append(compileProcess->readAllStandardOutput());
    outputView->append(compileProcess->readAllStandardError());

    // scroll to the bottom
    outputView->verticalScrollBar()->setValue(outputView->verticalScrollBar()->maximum());
  }

  bool compile() {
    if (currentFile.isEmpty()) {
      QMessageBox::warning(this, tr("Error"), tr("No file to compile"));
      return false;
    }

    // clear the output view
    outputView->clear();

    // if file ends with .cpp, use g++
    if (currentFile.endsWith(".cpp")) {
      compiler = "g++";
    }

    const QString output        = getBaseName(currentFile);
    const QStringList otherArgs = {"-o", output};

    // compose final args
    QStringList args;
    args << cFlags << ldFlags << otherArgs << currentFile;

    compileProcess->setWorkingDirectory(QFileInfo(currentFile).path());
    compileProcess->setProgram(compiler);
    compileProcess->setArguments(args);

    // log the command and arguments to the output view
    outputView->append(QString("Running: %1 %2\n").arg(compiler).arg(args.join(" ")));

    compileProcess->start();

    connect(compileProcess, &QProcess::readyReadStandardOutput, this, &EditorApp::updateOutput);
    connect(compileProcess, &QProcess::readyReadStandardError, this, &EditorApp::updateOutput);

    // Wait for the compilation process to finish
    compileProcess->waitForFinished();

    statusBar()->showMessage(tr("Compilation finished"), 2000);

    return compileProcess->exitCode() == 0;
  }

  void run() {
    if (currentFile.isEmpty()) {
      QMessageBox::warning(this, tr("Error"), tr("No file to run"));
      return;
    }

    // check if the output file exists
    if (!QFile::exists(QFileInfo(currentFile).path() + "/" + getBaseName(currentFile))) {
      QMessageBox::warning(this, tr("Error"), tr("No executable to run"));
      return;
    }

    // clear the output view
    outputView->clear();

    // Run the compiled program
    runProcess->setWorkingDirectory(QFileInfo(currentFile).path());
    runProcess->setProcessChannelMode(QProcess::SeparateChannels);
    runProcess->setProgram("./" + getBaseName(currentFile));

    connect(runProcess, &QProcess::readyReadStandardOutput, this, &EditorApp::updateRunOutput);
    connect(runProcess, &QProcess::readyReadStandardError, this, &EditorApp::updateRunOutput);

    // Start the process
    runProcess->start();

    // Check if the process started successfully
    if (!runProcess->waitForStarted()) {
      qDebug() << "Failed to start the process:" << runProcess->errorString();
      return;
    }

    // Wait for the process to finish
    runProcess->waitForFinished();
    statusBar()->showMessage(tr("Process finished with exit code: %1").arg(runProcess->exitCode()),
                             2000);
  }

  void disassemble() {
    disAssemblyView->setFont(textEditor->font());
    disAssemblyView->clear();

    if (currentFile.isEmpty()) {
      QMessageBox::warning(this, tr("Error"), tr("No file to disassemble"));
      return;
    }

    // check if the output file exists
    if (!QFile::exists(QFileInfo(currentFile).path() + "/" + getBaseName(currentFile))) {
      QMessageBox::warning(this, tr("Error"), tr("No executable to disassemble"));
      return;
    }

    // Disassemble the compiled program
    disAssembleProcess->setWorkingDirectory(QFileInfo(currentFile).path());
    disAssembleProcess->setProcessChannelMode(QProcess::SeparateChannels);
    disAssembleProcess->setProgram("objdump");

    QStringList args = {"-d", getBaseName(currentFile), "-M", "intel", "--no-show-raw-insn"};
    // If in C++ mode, use c++filt to demangle the symbols
    if (currentFile.endsWith(".cpp")) {
      args.append("--demangle");
    }

    // if using debug, show source code
    if (cFlags.contains("-g") || cFlags.contains("-ggdb")) {
      args.append("--source");
    }

    disAssembleProcess->setArguments(args);

    // log the command and arguments to the output view
    disAssemblyView->append(
        QString("Running: %1 %2\n").arg(disAssembleProcess->program()).arg(args.join(" ")));

    connect(disAssembleProcess, &QProcess::readyReadStandardOutput, this,
            &EditorApp::updateDisassembly);
    connect(disAssembleProcess, &QProcess::readyReadStandardError, this,
            &EditorApp::updateDisassembly);
    // Start the process
    disAssembleProcess->start();

    // Check if the process started successfully
    if (!disAssembleProcess->waitForStarted()) {
      qDebug() << "Failed to start the process:" << disAssembleProcess->errorString();
      return;
    }

    // Wait for the process to finish
    disAssembleProcess->waitForFinished();

    mainSplitter->setStretchFactor(0, 1); // First widget (side panel) - less priority for expansion
    mainSplitter->setStretchFactor(1, 1); // Second widget
    mainSplitter->setStretchFactor(
        2, 3); // Third widget (disassembly view) - more priority for expansion
  }

  void updateDisassembly() {
    disAssemblyView->append(disAssembleProcess->readAllStandardOutput());
    disAssemblyView->append(disAssembleProcess->readAllStandardError());
  }

  void updateRunOutput() {
    outputView->append(runProcess->readAllStandardOutput());
    outputView->append(runProcess->readAllStandardError());
  }

  void formatCode() {
    // Format the code in the text editor without losing undo history
    clangFormat->setProgram("clang-format");

    QStringList args = {"-style=Google"};

    // if there is a .clang-format file in the current directory, use it
    if (QFile::exists(".clang-format")) {
      args.append("--assume-filename=.clang-format");
    }

    clangFormat->setArguments(args);

    // Start clang-format process
    clangFormat->start();

    // Send the current content to clang-format via standard input
    clangFormat->write(textEditor->toPlainText().toUtf8());
    clangFormat->closeWriteChannel();

    if (!clangFormat->waitForStarted()) {
      QMessageBox::warning(this, tr("Error"), tr("Failed to start clang-format"));
      return;
    }

    if (!clangFormat->waitForFinished()) {
      QMessageBox::warning(this, tr("Error"), tr("Failed to format the code"));
      return;
    }

    // Get the formatted output from clang-format's standard output
    QString formattedCode = clangFormat->readAllStandardOutput();

    // Block signals to avoid triggering textChanged during formatting
    textEditor->blockSignals(true);

    // Replace the content of the editor with the formatted code
    QTextCursor cursor = textEditor->textCursor();
    cursor.beginEditBlock();
    cursor.select(QTextCursor::Document);
    cursor.insertText(formattedCode);
    cursor.endEditBlock();

    // Re-enable signals
    textEditor->blockSignals(false);

    statusBar()->showMessage(tr("Code formatted"), 2000);
  }

protected:
  void closeEvent(QCloseEvent *event) override {
    if (isDirty) {
      QMessageBox::StandardButton reply = QMessageBox::question(
          this, "Unsaved Changes", "You have unsaved changes. Do you want to save them?",
          QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);

      if (reply == QMessageBox::Save) {
        saveFile();
      } else if (reply == QMessageBox::Cancel) {
        event->ignore();
        return;
      }
    }

    event->accept();
  }
};

void enableDarkMode(QApplication &app) {
  // enable mode for dark theme based on the system theme
  QPalette p = app.palette();
  p.setColor(QPalette::Window, QColor("#282a36"));
  p.setColor(QPalette::WindowText, QColor("#f8f8f2"));
  p.setColor(QPalette::Base, QColor("#282a36"));
  p.setColor(QPalette::AlternateBase, QColor("#44475a"));
  p.setColor(QPalette::ToolTipBase, QColor("#44475a"));
  p.setColor(QPalette::ToolTipText, QColor("#f8f8f2"));
  p.setColor(QPalette::Text, QColor("#f8f8f2"));
  p.setColor(QPalette::Button, QColor("#44475a"));
  p.setColor(QPalette::ButtonText, QColor("#f8f8f2"));
  p.setColor(QPalette::BrightText, QColor("#ff5555"));
  p.setColor(QPalette::Link, QColor("#6272a4"));
  p.setColor(QPalette::Highlight, QColor("#44475a"));
  p.setColor(QPalette::HighlightedText, QColor("#f8f8f2"));
  app.setPalette(p);
}

int main(int argc, char *argv[]) {
  QApplication app(argc, argv);
  app.setApplicationName("Edit");
  app.setOrganizationName("Yo Medical Files (U) LTD");
  app.setOrganizationDomain("yomedicalfiles.com");
  app.setApplicationVersion("1.0");
  app.setStyle("Fusion");

  // check for file from the command line
  if (app.arguments().size() > 1) {
    EditorApp editor;
    editor.setCurrentFile(app.arguments().at(1));
  }

  EditorApp editor;

  editor.show();
  return app.exec();
}

#include "main.moc"
