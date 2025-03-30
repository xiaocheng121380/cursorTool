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
#include <windows.h>
#include <QDesktopServices>
#include <QUrl>
#include <QListWidget>
#include <QListWidgetItem>
#include <QTextEdit>
#include <QFrame>
#include <QScrollBar>
#include <QRegularExpression>
#include <shellapi.h>
#include <QCloseEvent>
#include <QTextCodec>
#include "powershellrunner.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
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
    QLabel *copyrightLabel = new QLabel("© 2024 Cursor重置工具 - 仅供学习交流使用");
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

void MainWindow::closeEvent(QCloseEvent *event)
{
    // 忽略关闭事件，改为隐藏窗口
    hide();
    event->ignore();
}

void MainWindow::closeCursor() {
    logInfo("正在关闭 Cursor 进程...");
    
    // 在Windows上使用wmic命令强制关闭进程
    #ifdef Q_OS_WIN
    QProcess process;
    process.setProcessChannelMode(QProcess::MergedChannels);
    process.setProcessEnvironment(QProcessEnvironment::systemEnvironment());
    
    process.start("wmic", QStringList() << "process" << "where" << "name='Cursor.exe'" << "delete");
    process.waitForFinished();
    
    if (process.exitCode() == 0) {
        logSuccess("✅ Cursor 进程已关闭");
    } else {
        QByteArray output = process.readAll();
        QString errorMessage = decodeProcessOutput(output);
        if (!errorMessage.isEmpty() && !errorMessage.contains("没有")) {
            logError("关闭进程时出错: " + errorMessage);
        } else {
            logInfo("没有找到运行中的 Cursor 进程");
        }
    }
    #else
    // 在其他平台上使用pkill
    QProcess process;
    process.setProcessChannelMode(QProcess::MergedChannels);
    
    process.start("pkill", QStringList() << "-f" << "Cursor");
    process.waitForFinished();
    
    if (process.exitCode() == 0) {
        logSuccess("✅ Cursor 进程已关闭");
    } else {
        QByteArray output = process.readAll();
        QString errorMessage = decodeProcessOutput(output);
        if (!errorMessage.isEmpty()) {
            logError("关闭进程时出错: " + errorMessage);
        } else {
            logInfo("没有找到运行中的 Cursor 进程");
        }
    }
    #endif
}

void MainWindow::clearCursorData() {
    #ifdef Q_OS_WIN
    // 定义路径
    QString appDataPath = QDir::homePath() + "/AppData/Roaming/Cursor";
    QString storageFile = appDataPath + "/User/globalStorage/storage.json";
    QString backupDir = appDataPath + "/User/globalStorage/backups";
    
    logInfo("🔍 开始Cursor重置过程");
    logInfo("正在创建备份目录...");
    // 创建备份目录
    QDir().mkpath(backupDir);
    
    // 备份注册表并修改MachineGuid - 改为只调用一次这个操作
    logInfo("正在备份并修改 MachineGuid...");
    bool regModified = modifyRegistry();
    if (regModified) {
        logSuccess("成功修改系统标识符");
    } else {
        logError("修改系统标识符失败，但将继续执行其他操作");
        logInfo("您可以之后尝试手动重启程序并重试修改注册表");
    }
    
    // 删除注册表项 - 使用新方法，不再直接执行删除
    logInfo("正在清理 Cursor 注册表项...");
    
    QProcess regDelete;
    regDelete.start("reg", QStringList() << "delete" 
                                       << "HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\cursor" 
                                       << "/f");
    regDelete.waitForFinished();
    
    if (regDelete.exitCode() != 0) {
        logInfo("注册表项 HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\cursor 可能不存在");
    } else {
        logSuccess("成功删除注册表项: HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\cursor");
    }
    
    // 删除第二个注册表项
    QProcess regDelete2;
    regDelete2.start("reg", QStringList() << "delete" 
                                        << "HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\App Paths\\cursor.exe" 
                                        << "/f");
    regDelete2.waitForFinished();
    
    if (regDelete2.exitCode() != 0) {
        logInfo("注册表项 HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\App Paths\\cursor.exe 可能不存在");
    } else {
        logSuccess("成功删除注册表项: HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\App Paths\\cursor.exe");
    }
    
    // 删除第三个注册表项
    QProcess regDelete3;
    regDelete3.start("reg", QStringList() << "delete" 
                                        << "HKCU\\Software\\Cursor" 
                                        << "/f");
    regDelete3.waitForFinished();
    
    if (regDelete3.exitCode() != 0) {
        logInfo("注册表项 HKCU\\Software\\Cursor 可能不存在");
    } else {
        logSuccess("成功删除注册表项: HKCU\\Software\\Cursor");
    }
    
    // 备份现有配置
    if(QFile::exists(storageFile)) {
        logInfo("正在备份配置文件...");
        QString backupName = "storage.json.backup_" + 
            QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");
        QString fullBackupPath = backupDir + "/" + backupName;
        if (QFile::copy(storageFile, fullBackupPath)) {
            logSuccess("配置文件备份成功: " + backupName);
        } else {
            logError("警告: 配置文件备份失败");
        }
    }
    
    logInfo("正在生成新的设备ID...");
    // 生成新的 ID
    QString machineId = generateMachineId();
    QString macMachineId = generateMacMachineId();
    
    // 更新 storage.json 文件
    if(QFile::exists(storageFile)) {
        QFile file(storageFile);
        if(file.open(QIODevice::ReadWrite)) {
            QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
            QJsonObject obj = doc.object();
            
            // 更新设备 ID
            obj["telemetry.machineId"] = machineId;
            obj["telemetry.macMachineId"] = macMachineId;
            obj["telemetry.devDeviceId"] = generateUUID();
            
            // 重置试用信息
            obj.remove("usage.cursorFreeUserDeadline");
            obj.remove("usage.didStartTrial");
            obj.remove("usage.hasSeenInAppTrial");
            
            file.resize(0);  // 清空文件
            file.write(QJsonDocument(obj).toJson());
            file.close();
            
            logSuccess("✅ Cursor 数据清除完成！可以进行下一步操作。");
            
            // 显示完整信息
            logInfo("已更新的字段:");
            logInfo("machineId: " + machineId);
            logInfo("macMachineId: " + macMachineId);
            logInfo("devDeviceId: " + obj["telemetry.devDeviceId"].toString());
            
            // 不显示弹窗，只在日志区域显示信息
        } else {
            logError("错误：无法访问配置文件");
            QMessageBox::warning(this, "错误", "无法访问配置文件，请确保 Cursor 已关闭。");
        }
    } else {
        logError("错误：未找到配置文件");
        QMessageBox::warning(this, "错误", "未找到配置文件，请确保 Cursor 已安装并运行过。");
    }
    #else
    // Linux/macOS 的处理
    QStringList possiblePaths;
    possiblePaths << QDir::homePath() + "/.config/Cursor"
                 << QDir::homePath() + "/.local/share/Cursor";
    
    bool foundAny = false;
    bool allSuccess = true;
    
    statusLabel->setText("正在查找 Cursor 数据目录...");
    for(const QString &path : possiblePaths) {
        QDir dir(path);
        if(dir.exists()) {
            foundAny = true;
            statusLabel->setText("正在删除目录: " + path);
            if(!dir.removeRecursively()) {
                allSuccess = false;
            }
        }
    }
    
    if(!foundAny) {
        statusLabel->setText("错误：未找到任何 Cursor 数据目录");
    } else if(allSuccess) {
        statusLabel->setText("Cursor 数据清除完成！");
    } else {
        statusLabel->setText("警告：部分数据清除失败，请手动删除");
    }
    #endif
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

void MainWindow::restartCursor() {
    #ifdef Q_OS_WIN
    QString cursorPath;
    
    // 尝试查找Cursor安装路径
    QProcess regQuery;
    regQuery.setProcessChannelMode(QProcess::MergedChannels);
    regQuery.setProcessEnvironment(QProcessEnvironment::systemEnvironment());
    
    regQuery.start("reg", QStringList() << "query" 
                                      << "HKEY_CURRENT_USER\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\cursor" 
                                      << "/v" << "InstallLocation");
    regQuery.waitForFinished();
    
    QString output = decodeProcessOutput(regQuery.readAll());
    QRegularExpression regex("InstallLocation\\s+REG_SZ\\s+(.*?)\\s*$");
    QRegularExpressionMatch match = regex.match(output);
    
    if (match.hasMatch()) {
        cursorPath = match.captured(1) + "\\Cursor.exe";
    } else {
        // 尝试在标准安装位置找到Cursor
        QStringList possiblePaths = {
            QDir::homePath() + "/AppData/Local/Programs/Cursor/Cursor.exe",
            "C:/Program Files/Cursor/Cursor.exe",
            "C:/Program Files (x86)/Cursor/Cursor.exe"
        };
        
        for (const QString &path : possiblePaths) {
            if (QFileInfo::exists(path)) {
                cursorPath = path;
                break;
            }
        }
    }
    
    if (!cursorPath.isEmpty() && QFileInfo::exists(cursorPath)) {
        logInfo("正在启动 Cursor: " + cursorPath);
        QProcess::startDetached(cursorPath, QStringList());
        logSuccess("✅ Cursor 已启动！重置过程完成。");
    } else {
        logError("错误：无法找到 Cursor 可执行文件。");
        QMessageBox::warning(this, "错误", "无法找到 Cursor 可执行文件，请手动启动 Cursor。");
    }
    #else
    QProcess::startDetached("cursor", QStringList());
    #endif
}

// 添加被其他方法使用的方法
bool MainWindow::checkAndCreateRegistryPath(const QString &path) {
    QProcess process;
    process.setProcessChannelMode(QProcess::MergedChannels);
    process.setProcessEnvironment(QProcessEnvironment::systemEnvironment());
    
    process.start("reg", QStringList() << "query" << path);
    process.waitForFinished();
    
    // 如果路径不存在，则创建
    if (process.exitCode() != 0) {
        QProcess createProcess;
        createProcess.setProcessChannelMode(QProcess::MergedChannels);
        createProcess.setProcessEnvironment(QProcessEnvironment::systemEnvironment());
        
        createProcess.start("reg", QStringList() << "add" << path << "/f");
        createProcess.waitForFinished();
        
        if (createProcess.exitCode() != 0) {
            QByteArray output = createProcess.readAll();
            QString errorMessage = decodeProcessOutput(output);
            logError("创建注册表路径失败: " + errorMessage);
            return false;
        }
    }
    
    return true;
}

// 此方法保留但在clearCursorData中不再调用，避免重复备份
void MainWindow::backupRegistry() {
    QString backupDir = QDir::homePath() + "/AppData/Roaming/Cursor/User/globalStorage/backups";
    QDir().mkpath(backupDir);
    
    // 备份 MachineGuid 注册表项
    QString backupFileNameSys = "Registry_MachineGuid_" + QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss") + ".reg";
    QString backupPathSys = backupDir + "/" + backupFileNameSys;
    
    logInfo("正在备份系统 MachineGuid 注册表项...");
    
    // 使用PowerShellRunner备份MachineGuid
    PowerShellRunner runnerSys;
    QEventLoop loopSys;
    bool sysBackupSuccess = false;
    
    // 连接信号槽处理结果
    connect(&runnerSys, &PowerShellRunner::operationCompleted, [&](bool success, const QString &message) {
        sysBackupSuccess = success;
        loopSys.quit();
    });
    
    connect(&runnerSys, &PowerShellRunner::backupCompleted, [&](bool success, const QString &backupFile, const QString &currentGuid) {
        if (success) {
            logSuccess("系统 MachineGuid 备份成功: " + backupFileNameSys);
            if (!currentGuid.isEmpty()) {
                logInfo("当前 MachineGuid 值: " + currentGuid);
            }
        } else {
            logError("系统 MachineGuid 备份失败");
        }
    });
    
    connect(&runnerSys, &PowerShellRunner::scriptError, [&](const QString &error) {
        logError("脚本错误: " + error);
    });
    
    // 执行备份
    runnerSys.backupRegistry(backupPathSys);
    loopSys.exec();
    
    // 继续使用传统方式备份其他注册表项
    
    // 备份 Cursor 卸载注册表项
    QString backupFileName = "Registry_CursorUninstall_" + QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss") + ".reg";
    QString backupPath = backupDir + "/" + backupFileName;
    
    logInfo("正在备份 Cursor 卸载注册表项...");
    
    // 使用GBK编码执行命令
    QProcess process;
    process.setProcessChannelMode(QProcess::MergedChannels);  // 合并标准输出和错误输出
    
    // 为Windows命令行设置正确的字符编码
    #ifdef Q_OS_WIN
    process.setProcessEnvironment(QProcessEnvironment::systemEnvironment());
    process.setCreateProcessArgumentsModifier([](QProcess::CreateProcessArguments *args) {
        args->flags |= CREATE_NEW_CONSOLE;
    });
    #endif
    
    process.start("reg", QStringList() << "export" 
                                     << "HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\cursor" 
                                     << backupPath 
                                     << "/y");
    process.waitForFinished();
    
    if (process.exitCode() == 0) {
        logSuccess("Cursor 卸载注册表项备份成功: " + backupFileName);
    } else {
        // 使用GBK编码读取错误信息
        QByteArray output = process.readAll();
        QString errorMessage = decodeProcessOutput(output);
        if (errorMessage.contains("找不到") || errorMessage.contains("系统找不到")) {
            logInfo("注册表路径 HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\cursor 可能不存在");
        } else {
            logError("注册表备份失败：" + errorMessage);
        }
    }
    
    // 备份第二个注册表路径
    QString backupFileName2 = "Registry_AppPaths_" + QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss") + ".reg";
    QString backupPath2 = backupDir + "/" + backupFileName2;
    
    logInfo("正在备份 Cursor AppPaths 注册表项...");
    
    QProcess process2;
    process2.setProcessChannelMode(QProcess::MergedChannels);
    
    #ifdef Q_OS_WIN
    process2.setProcessEnvironment(QProcessEnvironment::systemEnvironment());
    process2.setCreateProcessArgumentsModifier([](QProcess::CreateProcessArguments *args) {
        args->flags |= CREATE_NEW_CONSOLE;
    });
    #endif
    
    process2.start("reg", QStringList() << "export" 
                                      << "HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\App Paths\\cursor.exe" 
                                      << backupPath2 
                                      << "/y");
    process2.waitForFinished();
    
    if (process2.exitCode() == 0) {
        logSuccess("Cursor AppPaths 注册表项备份成功: " + backupFileName2);
    } else {
        // 使用GBK编码读取错误信息
        QByteArray output = process2.readAll();
        QString errorMessage = decodeProcessOutput(output);
        
        // 检查常见错误模式
        if (errorMessage.contains("找不到") || errorMessage.contains("系统找不到") || errorMessage.contains("指定")) {
            logInfo("注册表路径 HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\App Paths\\cursor.exe 可能不存在");
        } else {
            logError("注册表备份失败：" + errorMessage);
        }
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

void MainWindow::showBackups() {
    QString backupDir = QDir::homePath() + "/AppData/Roaming/Cursor/User/globalStorage/backups";
    QDir dir(backupDir);
    
    if (!dir.exists()) {
        logError("备份目录不存在");
        QMessageBox::information(this, "无备份", "没有找到备份文件。");
        return;
    }
    
    // 获取所有备份文件
    QStringList nameFilters;
    nameFilters << "*.reg" << "*.json*";
    QFileInfoList backups = dir.entryInfoList(nameFilters, QDir::Files, QDir::Time);
    
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
    
    QLabel *label = new QLabel("选择要还原的备份文件:");
    layout->addWidget(label);
    
    QListWidget *listWidget = new QListWidget();
    layout->addWidget(listWidget);
    
    // 添加备份文件到列表
    for (const QFileInfo &fileInfo : backups) {
        QListWidgetItem *item = new QListWidgetItem(fileInfo.fileName());
        item->setData(Qt::UserRole, fileInfo.absoluteFilePath());
        
        // 设置图标
        if (fileInfo.suffix().toLower() == "reg") {
            item->setIcon(QIcon::fromTheme("document-save"));
        } else {
            item->setIcon(QIcon::fromTheme("document-properties"));
        }
        
        listWidget->addItem(item);
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
            QMessageBox::information(&dialog, "未选择", "请选择一个备份文件。");
            return;
        }
        
        QString filePath = currentItem->data(Qt::UserRole).toString();
        if (filePath.endsWith(".reg")) {
            bool success = restoreRegistryBackup(filePath);
            if (success) {
                dialog.accept();
            }
        } else if (filePath.contains("storage.json")) {
            // 处理配置文件还原
            QString destPath = QDir::homePath() + "/AppData/Roaming/Cursor/User/globalStorage/storage.json";
            
            if (QFile::exists(destPath)) {
                if (QFile::remove(destPath)) {
                    if (QFile::copy(filePath, destPath)) {
                        logSuccess("配置文件还原成功!");
                        QMessageBox::information(&dialog, "成功", "配置文件还原成功!");
                    } else {
                        logError("配置文件复制失败");
                        QMessageBox::warning(&dialog, "错误", "配置文件复制失败。");
                    }
                } else {
                    logError("无法删除现有配置文件");
                    QMessageBox::warning(&dialog, "错误", "无法删除现有配置文件。");
                }
            } else {
                if (QFile::copy(filePath, destPath)) {
                    logSuccess("配置文件还原成功!");
                    QMessageBox::information(&dialog, "成功", "配置文件还原成功!");
                } else {
                    logError("配置文件复制失败");
                    QMessageBox::warning(&dialog, "错误", "配置文件复制失败。");
                }
            }
        }
    });
    
    connect(openFolderButton, &QPushButton::clicked, [backupDir]() {
        QDesktopServices::openUrl(QUrl::fromLocalFile(backupDir));
    });
    
    connect(cancelButton, &QPushButton::clicked, &dialog, &QDialog::reject);
    
    dialog.exec();
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

bool MainWindow::modifyRegistry() {
    // 生成新的GUID
    QUuid uuid = QUuid::createUuid();
    QString newGuid = uuid.toString().remove('{').remove('}');
    
    // 创建备份目录
    QString backupDir = QDir::homePath() + "/AppData/Roaming/Cursor/User/globalStorage/backups";
    QDir().mkpath(backupDir);
    
    // 构建备份文件路径 - 使用日期时间戳命名
    QString backupFileName = "Registry_MachineGuid_" + QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss") + ".reg";
    QString backupPath = backupDir + "/" + backupFileName;
    
    logInfo("正在备份并修改 MachineGuid...");
    
    // 使用PowerShellRunner执行脚本
    PowerShellRunner runner;
    
    // 连接信号槽以处理结果
    QEventLoop loop;
    bool operationSuccess = false;
    
    connect(&runner, &PowerShellRunner::operationCompleted, [&](bool success, const QString &message) {
        if (success) {
            logSuccess("注册表操作成功: " + message);
        } else {
            logError("注册表操作失败: " + message);
        }
        operationSuccess = success;
        loop.quit();
    });
    
    connect(&runner, &PowerShellRunner::modifyCompleted, [&](bool modifySuccess, const QString &newGuid, const QString &previousGuid) {
        if (modifySuccess) {
            logSuccess("成功修改系统标识符");
        } else {
            logError("修改系统标识符失败");
            logInfo("您可以之后尝试手动重启程序并重试修改注册表");
        }
    });
    
    connect(&runner, &PowerShellRunner::backupCompleted, [&](bool backupSuccess, const QString &backupFile, const QString &currentGuid) {
        if (backupSuccess) {
            logSuccess("注册表已成功备份到: " + backupFile);
        } else {
            logInfo("注册表备份失败");
        }
    });
    
    connect(&runner, &PowerShellRunner::scriptOutput, [&](const QString &output) {
        logInfo("脚本输出: " + output);
    });
    
    connect(&runner, &PowerShellRunner::scriptError, [&](const QString &error) {
        logError("脚本错误: " + error);
    });
    
    // 执行备份和修改操作
    runner.backupAndModifyRegistry(backupPath, newGuid);
    
    // 等待操作完成
    loop.exec();
    
    return operationSuccess;
} 