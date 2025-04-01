#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QFont>
#include <QFontDatabase>
#include <QApplication>
#include <QScreen>
#include <QPainter>
#include <QPixmap>
#include <QProcess>
#include <QMessageBox>
#include <QDir>
#include <QFileInfo>
#include <QFile>
#include <QSpacerItem>
#include <QSizePolicy>
#include <QDateTime>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRandomGenerator>
#include <QUuid>
#include <QDebug>
#include <QCoreApplication>
#include <QIcon>
#include <QTimer>
#include <QDesktopServices>
#include <QUrl>
#include <QListWidget>
#include <QListWidgetItem>
#include <QTextEdit>
#include <QFrame>
#include <QScrollBar>
#include <QRegularExpression>
#include <QCloseEvent>
#include <QTextCodec>
#include <QStandardPaths>

#ifdef Q_OS_WIN
#include <windows.h>
#include <shellapi.h>
#endif

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_powerShellRunner(nullptr)
    , m_macRunner(nullptr)
{
    // 设置全局编码 - 尝试使用系统编码而不是强制UTF-8
    QTextCodec *codec = QTextCodec::codecForLocale();
    QTextCodec::setCodecForLocale(codec);

    // 设置处理QProcess输出的编码
    #ifdef Q_OS_WIN
    _putenv_s("PYTHONIOENCODING", "utf-8");  // Windows特定
    #else
    setenv("PYTHONIOENCODING", "utf-8", 1);  // Unix系统
    #endif

    ui->setupUi(this);

    // Set window icon
    QPixmap iconPixmap(":/images/cursor_logo.png");
    QIcon icon(iconPixmap);
    setWindowIcon(icon);

    // 设置窗口属性
    setWindowTitle("Cursor重置工具 v1.0");
    setStyleSheet("QMainWindow { background-color: #1a1a1a; }");

    // 初始化系统特定的运行器
    #ifdef Q_OS_WIN
    m_powerShellRunner = new PowerShellRunner(this);
    connect(m_powerShellRunner, &PowerShellRunner::operationCompleted, this, &MainWindow::onOperationCompleted);
    connect(m_powerShellRunner, &PowerShellRunner::backupCompleted, this, &MainWindow::onBackupCompleted);
    connect(m_powerShellRunner, &PowerShellRunner::modifyCompleted, this, &MainWindow::onModifyCompleted);
    connect(m_powerShellRunner, &PowerShellRunner::scriptOutput, this, &MainWindow::onScriptOutput);
    connect(m_powerShellRunner, &PowerShellRunner::scriptError, this, &MainWindow::onScriptError);
    #else
    m_macRunner = new MacRunner(this);
    connect(m_macRunner, &MacRunner::operationCompleted, this, &MainWindow::onOperationCompleted);
    connect(m_macRunner, &MacRunner::backupCompleted, this, &MainWindow::onBackupCompleted);
    connect(m_macRunner, &MacRunner::modifyCompleted, this, &MainWindow::onModifyCompleted);
    connect(m_macRunner, &MacRunner::scriptOutput, this, &MainWindow::onScriptOutput);
    connect(m_macRunner, &MacRunner::scriptError, this, &MainWindow::onScriptError);
    #endif

    QWidget *centralWidget = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(15);

    // 初始化状态标签
    statusLabel = new QLabel("准备就绪");
    statusLabel->setStyleSheet("QLabel { color: #888888; font-size: 12px; }");
    statusLabel->setAlignment(Qt::AlignCenter);
    statusLabel->setWordWrap(true);  // 启用自动换行
    statusLabel->setFixedHeight(40);  // 设置固定高度
    statusLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);  // 水平方向可伸缩，垂直方向固定

    // CURSOR Logo
    QLabel *logoLabel = new QLabel();
    logoLabel->setAlignment(Qt::AlignCenter);

    // 创建 CURSOR logo
    QPixmap pixmap(400, 80);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);

    // 设置抗锯齿
    painter.setRenderHint(QPainter::Antialiasing);

    // 设置字体和颜色
    QFont font("Arial", 50, QFont::Bold);
    painter.setFont(font);
    painter.setPen(QColor("#00a8ff"));

    // 绘制文本
    painter.drawText(pixmap.rect(), Qt::AlignCenter, "CURSOR");

    painter.end();
    logoLabel->setPixmap(pixmap);

    mainLayout->addWidget(logoLabel, 0, Qt::AlignCenter);

    // 添加一些顶部间距
    QSpacerItem *topSpacer = new QSpacerItem(20, 20, QSizePolicy::Minimum, QSizePolicy::Fixed);
    mainLayout->addItem(topSpacer);

    // 添加状态标签到布局
    mainLayout->addWidget(statusLabel);

    // 添加说明文本
    QLabel *infoLabel = new QLabel("本工具用于重置Cursor试用期，请按照以下步骤操作：");
    infoLabel->setStyleSheet("QLabel { color: white; font-size: 14px; }");
    infoLabel->setAlignment(Qt::AlignCenter);  // 设置文本居中对齐
    mainLayout->addWidget(infoLabel);

    // 创建步骤按钮
    auto createStepButton = [this](const QString &text, const QString &style) -> QPushButton* {
        QPushButton *button = new QPushButton(text);
        button->setStyleSheet(style);
        button->setFixedHeight(40);
        button->setFixedWidth(400);  // 设置固定宽度
        return button;
    };

    QString buttonStyle = "QPushButton { \
        background-color: #2196F3; \
        color: white; \
        border: none; \
        padding: 0 15px; \
        font-size: 14px; \
        text-align: center; \
    } \
    QPushButton:hover { \
        background-color: #1976D2; \
    }";

    // 创建按钮容器并设置居中对齐
    QWidget *buttonContainer = new QWidget();
    QVBoxLayout *buttonLayout = new QVBoxLayout(buttonContainer);
    buttonLayout->setAlignment(Qt::AlignCenter);
    buttonLayout->setSpacing(10);
    buttonLayout->setContentsMargins(0, 0, 0, 0);

    // 步骤1：关闭Cursor
    QPushButton *step1Button = createStepButton("1. 关闭Cursor程序", buttonStyle);
    connect(step1Button, &QPushButton::clicked, this, &MainWindow::closeCursor);
    buttonLayout->addWidget(step1Button);

    // 步骤2：删除Cursor账号
    QPushButton *deleteAccountButton = createStepButton("2. 删除Cursor网站账号",
        "QPushButton { \
            background-color: #FF5722; \
            color: white; \
            border: none; \
            padding: 0 15px; \
            font-size: 14px; \
            text-align: center; \
        } \
        QPushButton:hover { \
            background-color: #E64A19; \
        }");
    connect(deleteAccountButton, &QPushButton::clicked, this, &MainWindow::openCursorAccount);
    buttonLayout->addWidget(deleteAccountButton);

    // 步骤3：清除数据
    QPushButton *step3Button = createStepButton("3. 清除Cursor数据", buttonStyle);
    connect(step3Button, &QPushButton::clicked, this, &MainWindow::clearCursorData);
    buttonLayout->addWidget(step3Button);

    // 步骤4：启动Cursor
    QPushButton *step4Button = createStepButton("4. 启动Cursor", buttonStyle);
    connect(step4Button, &QPushButton::clicked, this, &MainWindow::restartCursor);
    buttonLayout->addWidget(step4Button);

    // 添加查看备份按钮
    QPushButton *viewBackupsButton = createStepButton("查看备份",
        "QPushButton { \
            background-color: #607D8B; \
            color: white; \
            border: none; \
            padding: 0 15px; \
            font-size: 14px; \
            text-align: center; \
        } \
        QPushButton:hover { \
            background-color: #455A64; \
        }");
    connect(viewBackupsButton, &QPushButton::clicked, this, &MainWindow::showBackups);
    buttonLayout->addWidget(viewBackupsButton);

    mainLayout->addWidget(buttonContainer);

    // 添加弹性空间
    mainLayout->addStretch();

    // 添加日志文本区域
    QFrame *logFrame = new QFrame();
    logFrame->setFrameShape(QFrame::StyledPanel);
    logFrame->setFrameShadow(QFrame::Sunken);
    logFrame->setStyleSheet("QFrame { border: 1px solid #bbbbbb; background-color: #1a1a1a; }");
    QVBoxLayout *logLayout = new QVBoxLayout(logFrame);

    logTextArea = new QTextEdit();
    logTextArea->setReadOnly(true);
    logTextArea->setStyleSheet("QTextEdit { background-color: #1a1a1a; border: none; }"
                              "QTextEdit QScrollBar:vertical { width: 0px; }"  // 隐藏垂直滚动条
                              "QTextEdit QScrollBar:horizontal { height: 0px; }");  // 隐藏水平滚动条
    logTextArea->setMinimumHeight(150);
    logTextArea->setPlaceholderText("执行日志将显示在这里...");

    // 使用更亮的颜色和适合显示彩色文本的字体
    QFont logFont("Consolas, 'Microsoft YaHei'", 9);
    logTextArea->setFont(logFont);

    // 设置文本编辑器的基本样式
    QString styleSheet = "span { font-family: Consolas, 'Microsoft YaHei', sans-serif; }"
                         "span.timestamp { color: #888888; }"
                         "span.info { color: #00E5FF; }"
                         "span.success { color: #00E676; }"
                         "span.error { color: #FF1744; }";
    logTextArea->document()->setDefaultStyleSheet(styleSheet);

    logLayout->addWidget(logTextArea);
    mainLayout->addWidget(logFrame);

    // 添加版权信息
    QHBoxLayout *footerLayout = new QHBoxLayout();
    QLabel *copyrightLabel = new QLabel("© 2025 Cursor重置工具 - 仅供学习交流使用");
    copyrightLabel->setStyleSheet("QLabel { color: #666666; font-size: 12px; }");
    copyrightLabel->setAlignment(Qt::AlignCenter);

    QPushButton *qqGroupButton = new QPushButton("QQ交流群");
    qqGroupButton->setStyleSheet("QPushButton { \
        background-color: #009688; \
        color: white; \
        border: none; \
        padding: 8px 15px; \
        font-size: 14px; \
        border-radius: 4px; \
    } \
    QPushButton:hover { \
        background-color: #00796B; \
    }");
    connect(qqGroupButton, &QPushButton::clicked, [this]() {
        QDesktopServices::openUrl(QUrl("https://qm.qq.com/cgi-bin/qm/qr?k=-5rT6zS5nr0NqkUiphSPYHC2769qs21x&jump_from=webapi&authKey=PfidbLKACpBMOLaKrgqsJ9HS61vL2SaFr6KL2WW22njec2wxIZNTBok7wqr50lVt"));
        logInfo("正在打开QQ交流群...");
    });

    footerLayout->addWidget(copyrightLabel, 1);
    footerLayout->addWidget(qqGroupButton, 0);

    mainLayout->addLayout(footerLayout);

    setCentralWidget(centralWidget);

    // 初始化完成后显示欢迎信息和状态
    QTimer::singleShot(100, this, [this]() {
        // 显示欢迎信息和使用提示
        logInfo("欢迎使用 Cursor重置工具 v1.0");
        logInfo("请按顺序执行以下步骤:");
        logInfo("1. 关闭 Cursor 程序");
        logInfo("2. 删除 Cursor 网站账号");
        logInfo("3. 清除 Cursor 数据");
        logInfo("4. 启动 Cursor");
    });
}

MainWindow::~MainWindow()
{
    delete ui;
}

bool MainWindow::isMacOS() const
{
    #ifdef Q_OS_MAC
    return true;
    #else
    return false;
    #endif
}

QString MainWindow::getBackupPath() const
{
    QString basePath;
    if (isMacOS()) {
        basePath = QDir::homePath() + "/Library/Application Support/Cursor/Backups";
    } else {
        basePath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/Cursor/User/globalStorage/backups";
    }
    
    QDir dir(basePath);
    if (!dir.exists()) {
        dir.mkpath(".");
    }
    return basePath;
}

void MainWindow::closeCursor()
{
    logInfo("正在关闭 Cursor...");
    
    if (isMacOS()) {
        QProcess process;
        process.start("pkill", QStringList() << "-f" << "Cursor");
        process.waitForFinished();
    } else {
        QProcess process;
        process.start("taskkill", QStringList() << "/F" << "/IM" << "cursor.exe");
        process.waitForFinished();
    }
    
    logSuccess("Cursor 已关闭");
}

void MainWindow::clearCursorData()
{
    logInfo("正在清除 Cursor 数据...");
    
    // 首先进行备份
    if (isMacOS()) {
        if (m_macRunner) {
            QString backupDir = getBackupPath();
            QDir().mkpath(backupDir);
            m_macRunner->backupConfig(backupDir);
        }
        
        // 清除数据
        QString homeDir = QDir::homePath();
        QStringList paths = {
            homeDir + "/Library/Application Support/Cursor/User/globalStorage",
            homeDir + "/Library/Caches/Cursor",
            homeDir + "/Library/Preferences/com.cursor.Cursor.plist"
        };
        
        for (const QString &path : paths) {
            QDir dir(path);
            if (dir.exists()) {
                dir.removeRecursively();
                logInfo("已删除: " + path);
            }
        }
        
        // 修改设备 ID
        if (m_macRunner) {
            QString newGuid = generateUUID();
            m_macRunner->modifyConfig(newGuid);
        }
    } else {
        // Windows 特定的清理代码保持不变
        // ... existing code ...
    }
    
    logSuccess("Cursor 数据已清除");
}

void MainWindow::restartCursor()
{
    logInfo("正在启动 Cursor...");
    
    if (isMacOS()) {
        QProcess::startDetached("open", QStringList() << "-a" << "Cursor");
    } else {
        QProcess::startDetached("cursor.exe");
    }
    
    logSuccess("Cursor 已启动");
}

void MainWindow::backupRegistry()
{
    if (isMacOS()) {
        if (m_macRunner) {
            m_macRunner->backupConfig(getBackupPath());
        }
    } else {
        if (m_powerShellRunner) {
            m_powerShellRunner->backupRegistry(getBackupPath());
        }
    }
}

bool MainWindow::modifyRegistry()
{
    QString newGuid = generateUUID();
    
    if (isMacOS()) {
        if (m_macRunner) {
            m_macRunner->modifyConfig(newGuid);
            return true;
        }
    } else {
        if (m_powerShellRunner) {
            m_powerShellRunner->modifyRegistry(newGuid);
            return true;
        }
    }
    
    return false;
}

void MainWindow::showBackups() {
    QString backupDir = getBackupPath();
    
    QDir dir(backupDir);

    if (!dir.exists()) {
        logError("备份目录不存在");
        QMessageBox::information(this, "无备份", "没有找到备份文件。");
        return;
    }

    // 获取所有备份目录
    QFileInfoList backups = dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Time);

    if (backups.isEmpty()) {
        logError("未找到任何备份文件");
        QMessageBox::information(this, "无备份", "备份目录中没有找到任何备份文件。");
        return;
    }

    // 创建备份列表对话框
    QDialog dialog(this);
    dialog.setWindowTitle("备份文件");
    dialog.setMinimumWidth(500);
    dialog.setMinimumHeight(400);

    QVBoxLayout *layout = new QVBoxLayout(&dialog);

    QLabel *label = new QLabel("选择要还原的备份:");
    layout->addWidget(label);

    QListWidget *listWidget = new QListWidget();
    layout->addWidget(listWidget);

    // 添加备份目录到列表
    for (const QFileInfo &fileInfo : backups) {
        if (fileInfo.fileName().startsWith("cursor_backup_")) {
            QListWidgetItem *item = new QListWidgetItem(fileInfo.fileName());
            item->setData(Qt::UserRole, fileInfo.absoluteFilePath());
            item->setIcon(QIcon::fromTheme("folder"));
            listWidget->addItem(item);
        }
    }

    // 添加按钮
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    QPushButton *restoreButton = new QPushButton("还原");
    QPushButton *openFolderButton = new QPushButton("打开备份文件夹");
    QPushButton *cancelButton = new QPushButton("取消");

    buttonLayout->addWidget(restoreButton);
    buttonLayout->addWidget(openFolderButton);
    buttonLayout->addWidget(cancelButton);
    layout->addLayout(buttonLayout);

    // 连接信号
    connect(restoreButton, &QPushButton::clicked, [this, listWidget, &dialog]() {
        QListWidgetItem *currentItem = listWidget->currentItem();
        if (!currentItem) {
            QMessageBox::information(&dialog, "未选择", "请选择一个备份。");
            return;
        }

        QString backupPath = currentItem->data(Qt::UserRole).toString();
        QString userDir = QDir::homePath() + "/Library/Application Support/Cursor/User";
        
        // 删除当前的 User 目录
        QDir currentUserDir(userDir);
        if (currentUserDir.exists()) {
            currentUserDir.removeRecursively();
        }
        
        // 从备份恢复 User 目录
        QString backupUserDir = backupPath + "/User";
        if (QDir(backupUserDir).exists()) {
            if (QDir().mkpath(userDir)) {
                if (copyDirectory(backupUserDir, userDir)) {
                    logSuccess("备份还原成功!");
                    QMessageBox::information(&dialog, "成功", "备份还原成功!");
                    dialog.accept();
                } else {
                    logError("备份还原失败");
                    QMessageBox::warning(&dialog, "错误", "备份还原失败。");
                }
            } else {
                logError("无法创建目标目录");
                QMessageBox::warning(&dialog, "错误", "无法创建目标目录。");
            }
        } else {
            logError("备份文件不完整");
            QMessageBox::warning(&dialog, "错误", "备份文件不完整。");
        }
    });

    connect(openFolderButton, &QPushButton::clicked, [backupDir]() {
        QDesktopServices::openUrl(QUrl::fromLocalFile(backupDir));
    });

    connect(cancelButton, &QPushButton::clicked, &dialog, &QDialog::reject);

    dialog.exec();
}

bool MainWindow::copyDirectory(const QString &sourcePath, const QString &destPath)
{
    QDir sourceDir(sourcePath);
    QDir destDir(destPath);
    
    if (!destDir.exists()) {
        QDir().mkpath(destPath);
    }

    QStringList files = sourceDir.entryList(QDir::Files);
    for (const QString &file : files) {
        QString srcName = sourcePath + "/" + file;
        QString destName = destPath + "/" + file;
        if (!QFile::copy(srcName, destName)) {
            return false;
        }
    }

    QStringList dirs = sourceDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QString &dir : dirs) {
        QString srcName = sourcePath + "/" + dir;
        QString destName = destPath + "/" + dir;
        if (!copyDirectory(srcName, destName)) {
            return false;
        }
    }

    return true;
}

void MainWindow::logMessage(const QString &message, const QString &color) {
    QTextCursor cursor = logTextArea->textCursor();
    cursor.movePosition(QTextCursor::End);
    logTextArea->setTextCursor(cursor);

    // 添加时间戳
    QString timestamp = QDateTime::currentDateTime().toString("[HH:mm:ss] ");
    logTextArea->insertHtml("<span class='timestamp'>" + timestamp + "</span>");

    // 转义HTML特殊字符以确保消息正确显示
    QString escapedMessage = message;
    escapedMessage.replace("<", "&lt;").replace(">", "&gt;");

    // 确定消息类型
    QString cssClass = "info";
    if (color == "#00E676") {
        cssClass = "success";
    } else if (color == "#FF1744") {
        cssClass = "error";
    }

    // 使用CSS类显示消息
    logTextArea->insertHtml("<span class='" + cssClass + "'>" + escapedMessage + "</span><br>");

    // 滚动到底部
    QScrollBar *scrollBar = logTextArea->verticalScrollBar();
    scrollBar->setValue(scrollBar->maximum());

    // 保持日志不超过最大行数
    const int MAX_LOG_LINES = 500;
    QStringList lines = logTextArea->toPlainText().split('\n');
    if (lines.size() > MAX_LOG_LINES) {
        int linesToRemove = lines.size() - MAX_LOG_LINES;
        QTextCursor tc = logTextArea->textCursor();
        tc.movePosition(QTextCursor::Start);
        for (int i = 0; i < linesToRemove; i++) {
            tc.movePosition(QTextCursor::EndOfLine, QTextCursor::KeepAnchor);
            tc.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor);
            tc.removeSelectedText();
        }
    }
}

void MainWindow::logInfo(const QString &message) {
    logMessage(message, "#00E5FF");  // 青色
    statusLabel->setText(message);

    // 强制更新UI
    QApplication::processEvents();
}

void MainWindow::logSuccess(const QString &message) {
    logMessage(message, "#00E676");  // 明亮的绿色
    statusLabel->setText(message);
}

void MainWindow::logError(const QString &message) {
    logMessage(message, "#FF1744");  // 明亮的红色
    statusLabel->setText(message);
}

// 辅助函数：使用正确的编码解码进程输出
QString MainWindow::decodeProcessOutput(const QByteArray &output) {
    if (output.isEmpty()) return QString();

    // Windows下命令行输出一般是本地代码页编码，在中文Windows系统上通常是GBK或GB18030
    #ifdef Q_OS_WIN
    QTextCodec *codec = nullptr;

    // 尝试检测特定的错误模式，例如包含乱码的系统找不到指定的注册表项消息
    if (output.contains("\\xef\\xbf\\xbd\\xef\\xbf\\xbd") ||
        output.contains("\xef\xbf\xbd") ||
        output.contains("")) {
        // 尝试使用特定编码强制转换
        static const char* encodings[] = {"GBK", "GB18030", "GB2312", "CP936"};
        for (const char* encoding : encodings) {
            codec = QTextCodec::codecForName(encoding);
            if (codec) {
                QString decoded = codec->toUnicode(output);
                // 判断是否成功解码了常见的Windows注册表错误消息
                if (decoded.contains("找不到") ||
                    decoded.contains("系统找不到") ||
                    decoded.contains("指定的注册表")) {
                    return decoded.trimmed();
                }
            }
        }
    }

    // 如果没有特定模式匹配，尝试常规编码
    codec = QTextCodec::codecForName("GBK");
    if (!codec) codec = QTextCodec::codecForName("GB18030");
    if (!codec) codec = QTextCodec::codecForName("System");
    if (!codec) codec = QTextCodec::codecForLocale();

    QString result = codec->toUnicode(output).trimmed();

    // 如果结果中包含乱码标记，替换为通用消息
    if (result.contains("\xef\xbf\xbd") || result.contains("")) {
        return "系统找不到指定的注册表项或值";
    }

    return result;
    #else
    // 非Windows系统使用默认编码
    return QTextCodec::codecForLocale()->toUnicode(output).trimmed();
    #endif
}

void MainWindow::openCursorAccount() {
    // Cursor账号管理页面URL
    const QString cursorAccountUrl = "https://www.cursor.com/cn/settings";

    logInfo("正在打开Cursor账号管理页面...");

    // 使用系统默认浏览器打开网页
    if(QDesktopServices::openUrl(QUrl(cursorAccountUrl))) {
        logSuccess("已在浏览器中打开Cursor账号页面");
        logInfo("请在网页中登录并删除您的账号");
    } else {
        logError("无法打开默认浏览器");
        QMessageBox::warning(this, "错误", "无法打开默认浏览器，请手动访问: " + cursorAccountUrl);
    }
}

bool MainWindow::restoreRegistryBackup(const QString &backupFile) {
    QFileInfo fileInfo(backupFile);
    if (!fileInfo.exists()) {
        logError("备份文件不存在: " + backupFile);
        return false;
    }

    logInfo("正在还原注册表备份: " + fileInfo.fileName());

    QProcess process;
    process.setProcessChannelMode(QProcess::MergedChannels);

    #ifdef Q_OS_WIN
    process.setProcessEnvironment(QProcessEnvironment::systemEnvironment());
    #endif

    process.start("reg", QStringList() << "import" << backupFile);
    process.waitForFinished();

    if (process.exitCode() == 0) {
        logSuccess("注册表还原成功!");
        QMessageBox::information(this, "成功", "注册表还原成功!");
        return true;
    } else {
        QByteArray output = process.readAll();
        QString errorMessage = decodeProcessOutput(output);
        logError("注册表还原失败: " + errorMessage);
        QMessageBox::warning(this, "错误", "注册表还原失败: " + errorMessage);
        return false;
    }
}

void MainWindow::onOperationCompleted(bool success, const QString &message)
{
    if (success) {
        logSuccess(message);
    } else {
        logError(message);
    }
}

void MainWindow::onBackupCompleted(bool success, const QString &backupFile, const QString &currentGuid)
{
    if (success) {
        logSuccess("备份完成: " + backupFile);
        logInfo("当前设备ID: " + currentGuid);
    } else {
        logError("备份失败");
    }
}

void MainWindow::onModifyCompleted(bool success, const QString &newGuid, const QString &previousGuid)
{
    if (success) {
        logSuccess("设备ID修改成功");
        logInfo("原设备ID: " + previousGuid);
        logInfo("新设备ID: " + newGuid);
    } else {
        logError("设备ID修改失败");
    }
}

void MainWindow::onScriptOutput(const QString &output)
{
    logInfo(output);
}

void MainWindow::onScriptError(const QString &error)
{
    logError(error);
}

QString MainWindow::generateMachineId() {
    // 生成一个随机的机器ID
    QByteArray id;
    const int idLength = 32;  // 默认长度

    // 确保获取足够的随机数
    for(int i = 0; i < idLength; i++) {
        int random = QRandomGenerator::global()->bounded(0, 16);
        id.append(QString::number(random, 16).toLatin1());
    }

    return QString(id);
}

QString MainWindow::generateMacMachineId() {
    QUuid uuid = QUuid::createUuid();

    // 获取没有花括号的UUID字符串
    QString uuidStr = uuid.toString(QUuid::WithoutBraces);

    // 修改格式，使其符合macOS的格式
    QString formattedId = uuidStr.mid(0, 8) + "-" +
                          uuidStr.mid(9, 4) + "-" +
                          uuidStr.mid(14, 4) + "-" +
                          uuidStr.mid(19, 4) + "-" +
                          uuidStr.mid(24);

    return formattedId;
}

QString MainWindow::generateUUID() {
    QUuid uuid = QUuid::createUuid();
    return uuid.toString(QUuid::WithoutBraces);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    // 忽略关闭事件，改为隐藏窗口
    hide();
    event->ignore();
}
