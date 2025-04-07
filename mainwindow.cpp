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
#include <QInputDialog>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QHeaderView>
#include <QSettings>
#include "logmanager.h"
#include "curlhttpclient.h"
#include "databasemanager.h"
#include <QClipboard>
#include <QSqlQuery>
#include <qDebug>
#include "cursordatareader.h"
#ifdef Q_OS_WIN
#include <windows.h>
#include <shellapi.h>
#endif

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , statusLabel(new QLabel)
    , m_logManager(LogManager::instance())
    , m_powerShellRunner(new PowerShellRunner(this))
    , m_macRunner(new MacRunner(this))
    , m_cursorApi(new CursorApi(this))
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
    connect(m_powerShellRunner, &PowerShellRunner::operationCompleted, this, &MainWindow::onOperationCompleted);
    connect(m_powerShellRunner, &PowerShellRunner::backupCompleted, this, &MainWindow::onBackupCompleted);
    connect(m_powerShellRunner, &PowerShellRunner::modifyCompleted, this, &MainWindow::onModifyCompleted);
    connect(m_powerShellRunner, &PowerShellRunner::scriptOutput, this, &MainWindow::onScriptOutput);
    connect(m_powerShellRunner, &PowerShellRunner::scriptError, this, &MainWindow::onScriptError);
    #else
    connect(m_macRunner, &MacRunner::operationCompleted, this, &MainWindow::onOperationCompleted);
    connect(m_macRunner, &MacRunner::backupCompleted, this, &MainWindow::onBackupCompleted);
    connect(m_macRunner, &MacRunner::modifyCompleted, this, &MainWindow::onModifyCompleted);
    connect(m_macRunner, &MacRunner::scriptOutput, this, &MainWindow::onScriptOutput);
    connect(m_macRunner, &MacRunner::scriptError, this, &MainWindow::onScriptError);
    #endif

    QWidget *centralWidget = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setContentsMargins(20, 10, 20, 10);  // 减小上下边距
    mainLayout->setSpacing(5);  // 减小布局间距

    // 初始化状态标签
    statusLabel = new QLabel("准备就绪");
    statusLabel->setStyleSheet("QLabel { color: #888888; font-size: 12px; }");
    statusLabel->setAlignment(Qt::AlignCenter);
    statusLabel->setWordWrap(true);
    statusLabel->setFixedHeight(30);  // 减小高度
    statusLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

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

    // 创建两个并排的信息框
    QHBoxLayout *infoLayout = new QHBoxLayout();
    
    // 左侧用户信息框
    QFrame *userInfoFrame = new QFrame();
    userInfoFrame->setFrameShape(QFrame::NoFrame);
    userInfoFrame->setMinimumWidth(420);  // 增加最小宽度
    userInfoFrame->setStyleSheet("QFrame { \
        background-color: #1E1E1E; \
        border-radius: 4px; \
        padding: 10px; \
    }");
    QVBoxLayout *userInfoLayout = new QVBoxLayout(userInfoFrame);
    userInfoLayout->setSpacing(1);  // 减小行间距到1px
    userInfoLayout->setContentsMargins(10, 5, 10, 5);  // 减小上下内边距
    
    QLabel *userInfoTitle = new QLabel("用户信息");
    userInfoTitle->setStyleSheet("QLabel { color: white; font-size: 17px; font-weight: bold; margin-bottom: 5px; }");
    userInfoTitle->setAlignment(Qt::AlignCenter);  // 设置标题居中
    userInfoLayout->addWidget(userInfoTitle);

    // 用户名
    QHBoxLayout *nameLayout = new QHBoxLayout();
    QLabel *nameTitleLabel = new QLabel("用户名");
    nameTitleLabel->setStyleSheet("QLabel { color: #888888; font-size: 12px; min-width: 60px; }");
    QLabel *nameLabel = new QLabel("加载中...");
    nameLabel->setObjectName("nameLabel");
    nameLabel->setStyleSheet("QLabel { color: white; font-size: 12px; }");
    nameLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    nameLayout->addWidget(nameTitleLabel);
    nameLayout->addStretch();
    nameLayout->addWidget(nameLabel);
    userInfoLayout->addLayout(nameLayout);

    // 用户ID
    QHBoxLayout *userIdLayout = new QHBoxLayout();
    QLabel *userIdTitleLabel = new QLabel("用户ID");
    userIdTitleLabel->setStyleSheet("QLabel { color: #888888; font-size: 12px; min-width: 60px; }");
    QLabel *cpEmailLabel = new QLabel("加载中...");
    cpEmailLabel->setObjectName("cpEmailLabel");
    cpEmailLabel->setStyleSheet("QLabel { color: white; font-size: 12px; }");
    cpEmailLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    QPushButton *cpEmailButton = new QPushButton("验证中...");
    cpEmailButton->setObjectName("cpEmailButton");
    cpEmailButton->setFixedWidth(60);  // 设置固定宽度
    cpEmailButton->setStyleSheet("QPushButton { \
        background-color: transparent; \
        color: #888888; \
        border: 1px solid #888888; \
        border-radius: 2px; \
        font-size: 12px; \
        padding: 2px 8px; \
    }");
    userIdLayout->addWidget(userIdTitleLabel);
    userIdLayout->addWidget(cpEmailLabel);
    userIdLayout->addWidget(cpEmailButton);
    userInfoLayout->addLayout(userIdLayout);

    // 邮箱
    QHBoxLayout *localEmailLayout = new QHBoxLayout();
    QLabel *localEmailTitle = new QLabel("邮箱");
    localEmailTitle->setStyleSheet("QLabel { color: #888888; font-size: 12px; min-width: 60px; }");
    QLabel *localEmailLabel = new QLabel("加载中...");
    localEmailLabel->setObjectName("localEmailLabel");
    localEmailLabel->setStyleSheet("QLabel { color: white; font-size: 12px; }");
    localEmailLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    QPushButton *localEmailButton = new QPushButton("验证中...");
    localEmailButton->setObjectName("localEmailButton");
    localEmailButton->setFixedWidth(60);  // 设置固定宽度
    localEmailButton->setStyleSheet("QPushButton { \
        background-color: transparent; \
        color: #888888; \
        border: 1px solid #888888; \
        border-radius: 2px; \
        font-size: 12px; \
        padding: 2px 8px; \
    }");
    localEmailLayout->addWidget(localEmailTitle);
    localEmailLayout->addWidget(localEmailLabel);
    localEmailLayout->addWidget(localEmailButton);
    userInfoLayout->addLayout(localEmailLayout);

    // 用户状态
    QHBoxLayout *statusLayout = new QHBoxLayout();
    QLabel *statusTitleLabel = new QLabel("订阅状态");
    statusTitleLabel->setStyleSheet("QLabel { color: #888888; font-size: 12px; }");
    QPushButton *statusButton = new QPushButton("加载中...");
    statusButton->setObjectName("statusButton"); // 设置对象名
    statusButton->setStyleSheet("QPushButton { \
        background-color: transparent; \
        color: #FFC107; \
        border: 1px solid #FFC107; \
        border-radius: 2px; \
        font-size: 12px; \
        padding: 2px 8px; \
        text-align: right; \
    }");
    statusLayout->addWidget(statusTitleLabel);
    statusLayout->addStretch();
    statusLayout->addWidget(statusButton);
    userInfoLayout->addLayout(statusLayout);

    // 注册时间
    QHBoxLayout *expireLayout = new QHBoxLayout();
    QLabel *expireTitleLabel = new QLabel("注册时间");
    expireTitleLabel->setStyleSheet("QLabel { color: #888888; font-size: 12px; }");
    QLabel *expirationLabel = new QLabel("加载中...");
    expirationLabel->setObjectName("expirationLabel"); // 设置对象名
    expirationLabel->setStyleSheet("QLabel { color: white; font-size: 12px; }");
    expirationLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter); // 右对齐
    expireLayout->addWidget(expireTitleLabel);
    expireLayout->addStretch();
    expireLayout->addWidget(expirationLabel);
    userInfoLayout->addLayout(expireLayout);

    // 头像URL (隐藏，仅用于数据存储)
    QLabel *avatarUrlLabel = new QLabel("");
    avatarUrlLabel->setObjectName("avatarUrlLabel");
    avatarUrlLabel->setVisible(false);
    userInfoLayout->addWidget(avatarUrlLabel);

    userInfoLayout->addStretch();

    // 右侧使用统计框
    QFrame *usageFrame = new QFrame();
    usageFrame->setFrameShape(QFrame::NoFrame);
    usageFrame->setFixedWidth(420);  // 使用固定宽度
    usageFrame->setStyleSheet("QFrame { \
        background-color: #1E1E1E; \
        border-radius: 4px; \
        padding: 10px; \
    }");
    QVBoxLayout *usageLayout = new QVBoxLayout(usageFrame);
    usageLayout->setSpacing(10);  // 增加各项之间的间距
    usageLayout->setContentsMargins(10, 5, 10, 5);  // 减小上下内边距

    QLabel *usageTitle = new QLabel("使用统计");
    usageTitle->setStyleSheet("QLabel { color: white; font-size: 17px; font-weight: bold; margin-bottom: 5px; }");
    usageTitle->setAlignment(Qt::AlignCenter);  // 设置标题居中
    usageLayout->addWidget(usageTitle);

    // 使用最直接的方式创建统计项目
    QGridLayout *statsGrid = new QGridLayout();
    statsGrid->setContentsMargins(0, 0, 0, 0);
    statsGrid->setHorizontalSpacing(0);
    statsGrid->setVerticalSpacing(8);
    
    // 高级模型
    QLabel *highEndUsageLabel = new QLabel("高级模型使用量 (GPT-4-32k)");
    highEndUsageLabel->setStyleSheet("QLabel { color: #888888; font-size: 12px; }");
    
    QLabel *poolUsageLabel = new QLabel("0/50");
    poolUsageLabel->setObjectName("poolUsageLabel");
    poolUsageLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    poolUsageLabel->setStyleSheet("QLabel { color: white; font-size: 12px; }");
    
    QProgressBar *highEndProgressBar = new QProgressBar();
    highEndProgressBar->setObjectName("highEndProgressBar");
    highEndProgressBar->setRange(0, 100);
    highEndProgressBar->setValue(0);
    highEndProgressBar->setTextVisible(false);
    highEndProgressBar->setFixedHeight(5);
    highEndProgressBar->setFixedWidth(360);  // 设置更小的固定宽度
    highEndProgressBar->setStyleSheet("QProgressBar { background-color: #333333; border: none; border-radius: 2px; margin: 0px; }"
                                     "QProgressBar::chunk { background-color: #00B8D4; border-radius: 2px; }");
    
    // 创建一个容器，增加左边距
    QWidget *highEndProgressContainer = new QWidget();
    QHBoxLayout *highEndProgressLayout = new QHBoxLayout(highEndProgressContainer);
    highEndProgressLayout->setContentsMargins(10, 0, 0, 0);  // 左侧10px边距
    highEndProgressLayout->setSpacing(0);
    highEndProgressLayout->addWidget(highEndProgressBar);
    
    // 中级模型
    QLabel *midEndUsageLabel = new QLabel("中级模型使用量 (GPT-4)");
    midEndUsageLabel->setStyleSheet("QLabel { color: #888888; font-size: 12px; }");
    
    QLabel *advancedUsage = new QLabel("加载中...");
    advancedUsage->setObjectName("advancedUsageLabel");
    advancedUsage->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    advancedUsage->setStyleSheet("QLabel { color: white; font-size: 12px; }");
    
    QProgressBar *midEndProgressBar = new QProgressBar();
    midEndProgressBar->setObjectName("midEndProgressBar");
    midEndProgressBar->setRange(0, 100);
    midEndProgressBar->setValue(0); // 初始为0
    midEndProgressBar->setTextVisible(false);
    midEndProgressBar->setFixedHeight(5);
    midEndProgressBar->setFixedWidth(360);  // 设置更小的固定宽度
    midEndProgressBar->setStyleSheet("QProgressBar { background-color: #333333; border: none; border-radius: 2px; margin: 0px; }"
                                    "QProgressBar::chunk { background-color: #00E676; border-radius: 2px; }");
    
    // 创建一个容器，增加左边距
    QWidget *midEndProgressContainer = new QWidget();
    QHBoxLayout *midEndProgressLayout = new QHBoxLayout(midEndProgressContainer);
    midEndProgressLayout->setContentsMargins(10, 0, 0, 0);  // 左侧10px边距
    midEndProgressLayout->setSpacing(0);
    midEndProgressLayout->addWidget(midEndProgressBar);
    
    // 普通模型
    QLabel *normalUsageLabel = new QLabel("普通模型使用量 (GPT-3.5)");
    normalUsageLabel->setStyleSheet("QLabel { color: #888888; font-size: 12px; }");
    
    QLabel *normalUsage = new QLabel("加载中...");
    normalUsage->setObjectName("normalUsageLabel");
    normalUsage->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    normalUsage->setStyleSheet("QLabel { color: white; font-size: 12px; }");
    
    QProgressBar *normalProgressBar = new QProgressBar();
    normalProgressBar->setObjectName("normalProgressBar");
    normalProgressBar->setRange(0, 100);
    normalProgressBar->setValue(0); // 初始为0
    normalProgressBar->setTextVisible(false);
    normalProgressBar->setFixedHeight(5);
    normalProgressBar->setFixedWidth(360);  // 设置更小的固定宽度
    normalProgressBar->setStyleSheet("QProgressBar { background-color: #333333; border: none; border-radius: 2px; margin: 0px; }"
                                    "QProgressBar::chunk { background-color: #FFC107; border-radius: 2px; }");
                                    
    // 创建一个容器，增加左边距
    QWidget *normalProgressContainer = new QWidget();
    QHBoxLayout *normalProgressLayout = new QHBoxLayout(normalProgressContainer);
    normalProgressLayout->setContentsMargins(10, 0, 0, 0);  // 左侧10px边距
    normalProgressLayout->setSpacing(0);
    normalProgressLayout->addWidget(normalProgressBar);
    
    // 添加到网格
    QHBoxLayout *highEndHeader = new QHBoxLayout();
    highEndHeader->addWidget(highEndUsageLabel);
    highEndHeader->addStretch();
    highEndHeader->addWidget(poolUsageLabel);
    
    QHBoxLayout *midEndHeader = new QHBoxLayout();
    midEndHeader->addWidget(midEndUsageLabel);
    midEndHeader->addStretch();
    midEndHeader->addWidget(advancedUsage);
    
    QHBoxLayout *normalHeader = new QHBoxLayout();
    normalHeader->addWidget(normalUsageLabel);
    normalHeader->addStretch();
    normalHeader->addWidget(normalUsage);
    
    QVBoxLayout *statsLayout = new QVBoxLayout();
    statsLayout->setSpacing(1);
    statsLayout->setContentsMargins(0, 0, 0, 0);
    
    // 高级模型
    statsLayout->addLayout(highEndHeader);
    statsLayout->addWidget(highEndProgressContainer);  // 使用容器而不是直接添加进度条
    statsLayout->addSpacing(8); // 添加间距
    
    // 中级模型
    statsLayout->addLayout(midEndHeader);
    statsLayout->addWidget(midEndProgressContainer);  // 使用容器而不是直接添加进度条
    statsLayout->addSpacing(8); // 添加间距
    
    // 普通模型
    statsLayout->addLayout(normalHeader);
    statsLayout->addWidget(normalProgressContainer);  // 使用容器而不是直接添加进度条
    
    // 添加到主布局
    usageLayout->addLayout(statsLayout);
    
    // 计费周期信息
    QLabel *billingCycleLabel = new QLabel("计费周期: 加载中...");
    billingCycleLabel->setObjectName("billingCycleLabel");
    billingCycleLabel->setStyleSheet("QLabel { color: #1DE9B6; font-size: 12px; }");
    billingCycleLabel->setAlignment(Qt::AlignLeft);
    billingCycleLabel->setWordWrap(false);
    billingCycleLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);
    billingCycleLabel->setVisible(true);
    billingCycleLabel->setContentsMargins(0, 10, 0, 0); // 顶部增加一点间距
    usageLayout->addWidget(billingCycleLabel);

    usageLayout->addStretch();

    // 添加两个框到水平布局
    infoLayout->addWidget(userInfoFrame);
    infoLayout->addWidget(usageFrame);
    infoLayout->setSpacing(15);  // 减小两个框之间的间距
    
    // 添加信息框布局到主布局
    mainLayout->addLayout(infoLayout);

    // 添加一键更换按钮
    QPushButton *oneClickResetButton = new QPushButton("一键重置");
    oneClickResetButton->setStyleSheet("QPushButton { \
        background-color: #00BFA5; \
        color: white; \
        border: none; \
        border-radius: 4px; \
        padding: 6px 15px; \
        font-size: 14px; \
        margin: 5px 0; \
    } \
    QPushButton:hover { \
        background-color: #00897B; \
    }");
    oneClickResetButton->setCursor(Qt::PointingHandCursor);  // 鼠标悬停时显示手型光标
    oneClickResetButton->setFixedWidth(200);  // 设置按钮宽度
    oneClickResetButton->setObjectName("oneClickResetButton");
    mainLayout->addWidget(oneClickResetButton, 0, Qt::AlignCenter);

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
        font-size: 12px; \
    } \
    QPushButton:hover { \
        background-color: #00796B; \
    }");

    footerLayout->addWidget(copyrightLabel);
    footerLayout->addWidget(qqGroupButton);
    mainLayout->addLayout(footerLayout);

    setCentralWidget(centralWidget);

    initializeConnections();
    startInitialDataFetch();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::clearCursorData() {
    // 定义路径
    QString userProfile = QDir::homePath();
    QString appDataPath = userProfile + "/AppData/Roaming/Cursor";
    QString storageFile = appDataPath + "/User/globalStorage/storage.json";
    QString backupDir = appDataPath + "/User/globalStorage/backups";
    
    m_logManager->logInfo("🔍 开始Cursor重置过程");
    m_logManager->logInfo("正在创建备份目录...");
    // 创建备份目录
    QDir().mkpath(backupDir);
    
    // 备份注册表并修改MachineGuid - 改为只调用一次这个操作
    m_logManager->logInfo("正在备份并修改 MachineGuid...");
    bool regModified = modifyRegistry();
    if (regModified) {
        m_logManager->logSuccess("成功修改系统标识符");
    } else {
        m_logManager->logInfo("您可以之后尝试手动重启程序并重试修改注册表");
    }
    
    // 删除注册表项 - 使用新方法，不再直接执行删除
    m_logManager->logInfo("正在清理 Cursor 注册表项...");
    
    QProcess regDelete;
    regDelete.start("reg", QStringList() << "delete" 
                                       << "HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\cursor" 
                                       << "/f");
    regDelete.waitForFinished();
    
    if (regDelete.exitCode() != 0) {
        m_logManager->logInfo("注册表项 HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\cursor 可能不存在");
    } else {
        m_logManager->logSuccess("成功删除注册表项: HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\cursor");
    }
    
    // 删除第二个注册表项
    QProcess regDelete2;
    regDelete2.start("reg", QStringList() << "delete" 
                                        << "HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\App Paths\\cursor.exe" 
                                        << "/f");
    regDelete2.waitForFinished();
    
    if (regDelete2.exitCode() != 0) {
        m_logManager->logInfo("注册表项 HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\App Paths\\cursor.exe 可能不存在");
    } else {
        m_logManager->logSuccess("成功删除注册表项: HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\App Paths\\cursor.exe");
    }
    
    // 删除第三个注册表项
    QProcess regDelete3;
    regDelete3.start("reg", QStringList() << "delete" 
                                        << "HKCU\\Software\\Cursor" 
                                        << "/f");
    regDelete3.waitForFinished();
    
    if (regDelete3.exitCode() != 0) {
        m_logManager->logInfo("注册表项 HKCU\\Software\\Cursor 可能不存在");
    } else {
        m_logManager->logSuccess("成功删除注册表项: HKCU\\Software\\Cursor");
    }
    
    // 备份现有配置
    if(QFile::exists(storageFile)) {
        m_logManager->logInfo("正在备份配置文件...");
        QString backupName = "storage.json.backup_" + 
            QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");
        QString fullBackupPath = backupDir + "/" + backupName;
        if (QFile::copy(storageFile, fullBackupPath)) {
            m_logManager->logSuccess("配置文件备份成功: " + backupName);
        } else {
            m_logManager->logError("警告: 配置文件备份失败");
        }
    }
    
    m_logManager->logInfo("正在生成新的设备ID...");
    // 生成新的 ID
    QString machineId = generateMachineId();
    QString macMachineId = generateMacMachineId();
    
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
            
            m_logManager->logSuccess("✅ Cursor 数据清除完成！可以进行下一步操作。");
            
            // 显示完整信息
            m_logManager->logInfo("已更新的字段:");
            m_logManager->logInfo("machineId: " + machineId);
            m_logManager->logInfo("macMachineId: " + macMachineId);
            m_logManager->logInfo("devDeviceId: " + obj["telemetry.devDeviceId"].toString());
            
            // 不显示弹窗，只在日志区域显示信息
        } else {
            m_logManager->logError("错误：无法访问配置文件");
            QMessageBox::warning(this, "错误", "无法访问配置文件，请确保 Cursor 已关闭。");
        }
    } else {
        m_logManager->logError("错误：未找到配置文件");
        QMessageBox::warning(this, "错误", "未找到配置文件，请确保 Cursor 已安装并运行过。");
    }
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

QString MainWindow::generateUUID()
{
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
        m_logManager->logInfo("正在启动 Cursor: " + cursorPath);
        QProcess::startDetached(cursorPath, QStringList());
        m_logManager->logSuccess("✅ Cursor 已启动！重置过程完成。");
    } else {
        m_logManager->logError("错误：无法找到 Cursor 可执行文件。");
        QMessageBox::warning(this, "错误", "无法找到 Cursor 可执行文件，请手动启动 Cursor。");
    }
    #else
    QProcess::startDetached("cursor", QStringList());
    #endif
}

void MainWindow::closeCursor() {
    m_logManager->logInfo("正在关闭 Cursor 进程...");
    
    // 在Windows上使用wmic命令强制关闭进程
    #ifdef Q_OS_WIN
    QProcess process;
    process.setProcessChannelMode(QProcess::MergedChannels);
    process.setProcessEnvironment(QProcessEnvironment::systemEnvironment());
    
    process.start("wmic", QStringList() << "process" << "where" << "name='Cursor.exe'" << "delete");
    process.waitForFinished();
    
    if (process.exitCode() == 0) {
        m_logManager->logSuccess("✅ Cursor 进程已关闭");
    } else {
        QByteArray output = process.readAll();
        QString errorMessage = decodeProcessOutput(output);
        if (!errorMessage.isEmpty() && !errorMessage.contains("没有")) {
            m_logManager->logError("关闭进程时出错: " + errorMessage);
        } else {
            m_logManager->logInfo("没有找到运行中的 Cursor 进程");
        }
    }
    #else
    // 在其他平台上使用pkill
    QProcess process;
    process.setProcessChannelMode(QProcess::MergedChannels);
    
    process.start("pkill", QStringList() << "-f" << "Cursor");
    process.waitForFinished();
    
    if (process.exitCode() == 0) {
        m_logManager->logSuccess("✅ Cursor 进程已关闭");
    } else {
        QByteArray output = process.readAll();
        QString errorMessage = decodeProcessOutput(output);
        if (!errorMessage.isEmpty()) {
            m_logManager->logError("关闭进程时出错: " + errorMessage);
        } else {
            m_logManager->logInfo("没有找到运行中的 Cursor 进程");
        }
    }
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
            m_logManager->logError("创建注册表路径失败: " + errorMessage);
            return false;
        }
    }
    
    return true;
}

void MainWindow::onOperationCompleted(bool success, const QString &message)
{
    if (success) {
        m_logManager->logSuccess(message);
    } else {
        m_logManager->logError(message);
    }
}

void MainWindow::onBackupCompleted(bool success, const QString &backupFile, const QString &currentGuid)
{
    if (success) {
        m_logManager->logSuccess("备份完成: " + backupFile);
        m_logManager->logInfo("当前设备ID: " + currentGuid);
    } else {
        m_logManager->logError("备份失败");
    }
}

void MainWindow::onModifyCompleted(bool success, const QString &newGuid, const QString &previousGuid)
{
    if (success) {
        m_logManager->logSuccess("设备ID修改成功");
        m_logManager->logInfo("原设备ID: " + previousGuid);
        m_logManager->logInfo("新设备ID: " + newGuid);
    } else {
        m_logManager->logError("设备ID修改失败");
    }
}

void MainWindow::onScriptOutput(const QString &output)
{
    // m_logManager->logInfo(output);
}

void MainWindow::onScriptError(const QString &error)
{
    m_logManager->logError(error);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    hide();
    event->ignore();
}

void MainWindow::initializeConnections()
{
    // 连接CursorApi信号
    connect(m_cursorApi, &CursorApi::userInfoUpdated, this, &MainWindow::onUserInfoUpdated);
    connect(m_cursorApi, &CursorApi::userInfoError, this, &MainWindow::onUserInfoError);
    connect(m_cursorApi, &CursorApi::usageInfoUpdated, this, &MainWindow::onUsageInfoUpdated);
    connect(m_cursorApi, &CursorApi::usageInfoError, this, &MainWindow::onUsageInfoError);

    // 连接日志管理器信号
    connect(m_logManager, &LogManager::logMessage, this, &MainWindow::onLogMessage);

    // 连接一键重置
    QPushButton *oneClickResetButton = findChild<QPushButton*>("oneClickResetButton");
    if (oneClickResetButton) {
        connect(oneClickResetButton, &QPushButton::clicked, this, &MainWindow::onOneClickResetClicked);
    }
}

void MainWindow::startInitialDataFetch()
{
    m_logManager->logInfo("正在启动初始化...");
    CursorSessionData sessionData = CursorDataReader::readSessionData();
    
    if (!sessionData.release.isEmpty()) {
        m_logManager->logInfo("Cursor版本: " + sessionData.release);
    } else {
        m_logManager->logInfo("无法获取Cursor版本信息");
    }
    
    if (!sessionData.email.isEmpty()) {
        m_logManager->logInfo("用户邮箱: " + sessionData.email);
    }
    
    if (!sessionData.userId.isEmpty()) {
        m_logManager->logInfo("用户ID: " + sessionData.userId);
    }
    
    // 使用完整的Cookie格式设置token
    QString authCookie = loadAuthToken();
    // qDebug() << "认证Cookie: " + authCookie;
    
    m_cursorApi->setAuthToken(authCookie);
    
    // 先请求用户信息，然后在收到用户信息后再请求使用统计信息
    QTimer::singleShot(300, [this]() {
        m_logManager->logInfo("开始获取用户信息...");
        
        // 使用适当的方式处理一次性连接
        QMetaObject::Connection *conn = new QMetaObject::Connection();
        *conn = connect(m_cursorApi, &CursorApi::userInfoUpdated, 
            [this, conn](const CursorUserInfo &) {
                // 断开连接，确保只触发一次
                disconnect(*conn);
                delete conn; // 释放内存
                
                // 延迟300ms后请求使用统计信息
                QTimer::singleShot(300, [this]() {
                    m_logManager->logInfo("开始获取使用统计信息...");
                    
                    // 添加使用统计信息失败后的重试逻辑
                    QMetaObject::Connection *usageConn = new QMetaObject::Connection();
                    *usageConn = connect(m_cursorApi, &CursorApi::usageInfoError, 
                        [this, usageConn](const QString &error) {
                            static int retryCount = 0;
                            const int maxRetries = 2;
                            
                            if (error.contains("Timeout was reached") && retryCount < maxRetries) {
                                retryCount++;
                                m_logManager->logInfo(QString("获取使用统计信息超时，延迟后第 %1 次重试...").arg(retryCount));
                                
                                // 延迟1.5秒后重试
                                QTimer::singleShot(1500, [this]() {
                                    m_cursorApi->fetchUsageInfo();
                                });
                            } else {
                                // 断开连接，防止内存泄漏
                                disconnect(*usageConn);
                                delete usageConn;
                                retryCount = 0;
                            }
                        });
                    
                    m_cursorApi->fetchUsageInfo();
                });
            });
        
        // 发起用户信息请求
        m_cursorApi->fetchUserInfo();
    });
}

void MainWindow::onUserInfoUpdated(const CursorUserInfo &info)
{
    updateUserInfoDisplay(info);
}

void MainWindow::onUserInfoError(const QString &error)
{
    static int retryCount = 0;
    const int maxRetries = 2;
    
    // 检查是否是超时错误
    if (error.contains("Timeout was reached") && retryCount < maxRetries) {
        retryCount++;
        m_logManager->logInfo(QString("获取用户信息超时，正在进行第 %1 次重试...").arg(retryCount));
        
        // 延迟1秒后重试
        QTimer::singleShot(1000, [this]() {
            m_cursorApi->fetchUserInfo();
        });
    } else {
        // 重置重试计数并显示错误
        retryCount = 0;
        m_logManager->logError(QString("获取用户信息失败: %1").arg(error));
    }
}

void MainWindow::onUsageInfoUpdated(const CursorUsageInfo &info)
{
    updateUsageInfoDisplay(info);
}

void MainWindow::onUsageInfoError(const QString &error)
{
    static int retryCount = 0;
    const int maxRetries = 2;
    
    // 检查是否是超时错误
    if (error.contains("Timeout was reached") && retryCount < maxRetries) {
        retryCount++;
        m_logManager->logInfo(QString("获取使用统计超时，正在进行第 %1 次重试...").arg(retryCount));
        
        // 延迟1秒后重试
        QTimer::singleShot(1000, [this]() {
            m_cursorApi->fetchUsageInfo();
        });
    } else {
        // 重置重试计数并显示错误
        retryCount = 0;
        m_logManager->logError(QString("获取使用统计失败: %1").arg(error));
    }
}

void MainWindow::onLogMessage(const QString &formattedMessage)
{
    if (logTextArea) {
        logTextArea->append(formattedMessage);
        // 滚动到底部
        QScrollBar *scrollBar = logTextArea->verticalScrollBar();
        scrollBar->setValue(scrollBar->maximum());
    }
}

void MainWindow::updateUserInfoDisplay(const CursorUserInfo &info)
{
    m_logManager->logInfo("更新用户信息显示...");

    // 更新用户名
    QLabel *nameLabel = findChild<QLabel*>("nameLabel");
    if (nameLabel) {
        nameLabel->setText(!info.name.isEmpty() ? info.name : "未知用户");
    }

    // 更新用户ID
    QLabel *cpEmailLabel = findChild<QLabel*>("cpEmailLabel");
    QPushButton *cpEmailButton = findChild<QPushButton*>("cpEmailButton");
    if (cpEmailLabel) {
        cpEmailLabel->setText(!info.id.isEmpty() ? info.id : "未知ID");
    }
    if (cpEmailButton) {
        cpEmailButton->setText(info.emailVerified ? "已验证" : "未验证");
        cpEmailButton->setStyleSheet(info.emailVerified ? 
            "QPushButton { background-color: transparent; color: #4CAF50; border: 1px solid #4CAF50; border-radius: 2px; font-size: 12px; padding: 2px 8px; }" : 
            "QPushButton { background-color: transparent; color: #FF5722; border: 1px solid #FF5722; border-radius: 2px; font-size: 12px; padding: 2px 8px; }");
    }

    // 更新邮箱
    QLabel *localEmailLabel = findChild<QLabel*>("localEmailLabel");
    QPushButton *localEmailButton = findChild<QPushButton*>("localEmailButton");
    if (localEmailLabel) {
        localEmailLabel->setText(!info.email.isEmpty() ? info.email : "未设置邮箱");
    }
    if (localEmailButton) {
        localEmailButton->setText(info.emailVerified ? "已验证" : "未验证");
        localEmailButton->setStyleSheet(info.emailVerified ? 
            "QPushButton { background-color: transparent; color: #4CAF50; border: 1px solid #4CAF50; border-radius: 2px; font-size: 12px; padding: 2px 8px; }" : 
            "QPushButton { background-color: transparent; color: #FF5722; border: 1px solid #FF5722; border-radius: 2px; font-size: 12px; padding: 2px 8px; }");
    }

    // 更新订阅状态
    QPushButton *statusButton = findChild<QPushButton*>("statusButton");
    if (statusButton) {
        QString status = "免费用户";  // 默认状态
        statusButton->setText(status);
        statusButton->setStyleSheet("QPushButton { background-color: transparent; color: #2196F3; border: 1px solid #2196F3; border-radius: 2px; font-size: 12px; padding: 2px 8px; }");
    }

    // 注册时间
    QLabel *expirationLabel = findChild<QLabel*>("expirationLabel");
    if (expirationLabel) {
        QDateTime updateTime = QDateTime::fromString(info.updatedAt, Qt::ISODate);
        if (updateTime.isValid()) {
            QString updateTimeText = updateTime.toString("yyyy-MM-dd");
            //右对齐
            expirationLabel->setAlignment(Qt::AlignRight);
            expirationLabel->setText(updateTimeText);
            expirationLabel->setStyleSheet("QLabel { color: white; font-size: 12px; }");
                } else {
            expirationLabel->setText("无注册时间");
            expirationLabel->setStyleSheet("QLabel { color: #888888; font-size: 12px; }");
        }
    }
    
    // 更新头像URL
    QLabel *avatarUrlLabel = findChild<QLabel*>("avatarUrlLabel");
    if (avatarUrlLabel) {
        avatarUrlLabel->setText(info.picture);
    }
    
    m_logManager->logInfo("用户信息更新完成");
}

void MainWindow::updateUsageInfoDisplay(const CursorUsageInfo &info)
{
    m_logManager->logInfo("更新使用统计信息显示...");
    
    // 更新GPT-4-32k使用量
    QLabel *poolUsageLabel = findChild<QLabel*>("poolUsageLabel");
    QProgressBar *highEndProgressBar = findChild<QProgressBar*>("highEndProgressBar");
    if (poolUsageLabel) {
        if (info.gpt432kMaxRequests > 0) {
            poolUsageLabel->setText(QString("%1/%2")
                .arg(info.gpt432kRequests)
                .arg(info.gpt432kMaxRequests));
            
            // 更新进度条
            if (highEndProgressBar) {
                int percentage = qMin(100, (int)(100.0 * info.gpt432kRequests / info.gpt432kMaxRequests));
                highEndProgressBar->setValue(percentage);
            }
        } else {
            poolUsageLabel->setText(QString("%1/无限制").arg(info.gpt432kRequests));
            // 无限制情况下，进度条保持为0
            if (highEndProgressBar) {
                highEndProgressBar->setValue(0);
            }
        }
    }

    // 更新GPT-4使用量
    QLabel *advancedUsageLabel = findChild<QLabel*>("advancedUsageLabel");
    QProgressBar *midEndProgressBar = findChild<QProgressBar*>("midEndProgressBar");
    if (advancedUsageLabel) {
        if (info.gpt4MaxRequests > 0) {
            advancedUsageLabel->setText(QString("%1/%2")
                .arg(info.gpt4Requests)
                .arg(info.gpt4MaxRequests));
            
            // 更新进度条
            if (midEndProgressBar) {
                int percentage = qMin(100, (int)(100.0 * info.gpt4Requests / info.gpt4MaxRequests));
                midEndProgressBar->setValue(percentage);
            }
        } else {
            advancedUsageLabel->setText(QString("%1/无限制").arg(info.gpt4Requests));
            // 无限制情况下，进度条保持为0
            if (midEndProgressBar) {
                midEndProgressBar->setValue(0);
            }
        }
    }

    // 更新GPT-3.5使用量
    QLabel *normalUsageLabel = findChild<QLabel*>("normalUsageLabel");
    QProgressBar *normalProgressBar = findChild<QProgressBar*>("normalProgressBar");
    if (normalUsageLabel) {
        normalUsageLabel->setText(QString("%1/%2")
            .arg(info.gpt35Requests)
            .arg(info.gpt35MaxRequests));
        
        // 更新进度条
        if (normalProgressBar) {
            int percentage = info.gpt35MaxRequests > 0 
                ? qMin(100, (int)(100.0 * info.gpt35Requests / info.gpt35MaxRequests))
                : 0;
            normalProgressBar->setValue(percentage);
        }
    }
    
    // 设置计费周期信息
    QLabel *billingCycleLabel = findChild<QLabel*>("billingCycleLabel");
    if (billingCycleLabel) {
        if (info.startOfMonth.isValid()) {
            // 显示计费周期开始日期和估计结束日期
            QDateTime endOfMonth = info.startOfMonth.addMonths(1);
            QString billingCycleText = QString("计费周期: %1 至 %2")
                .arg(info.startOfMonth.date().toString("yyyy-MM-dd"))
                .arg(endOfMonth.date().toString("yyyy-MM-dd"));
            
            // 计算当前周期剩余天数
            int daysLeft = QDateTime::currentDateTime().daysTo(endOfMonth);
            if (daysLeft >= 0) {
                billingCycleText += QString(" (剩余 %1 天)").arg(daysLeft);
            }
            
            billingCycleLabel->setText(billingCycleText);
            billingCycleLabel->setVisible(true);
        } else {
            billingCycleLabel->setVisible(false);
        }
    }
    
    m_logManager->logInfo("使用统计信息更新完成");
}

void MainWindow::onOneClickResetClicked()
{
    m_logManager->logInfo("开始执行一键更换操作...");
    
    // 第一步：删除账户
    m_cursorApi->deleteAccount();
    m_logManager->logSuccess("账户删除成功，开始执行后续操作...");
    closeCursor();
    clearCursorData();
    restartCursor();
}

// 从设置中读取token，优先尝试从Cursor数据库获取
QString MainWindow::loadAuthToken()
{
    QSettings settings(QCoreApplication::organizationName(), QCoreApplication::applicationName());
    QString token = settings.value("auth/token").toString();
    
    // 尝试从数据库读取token
    DatabaseManager* dbManager = DatabaseManager::instance();
    if (dbManager->openDatabase()) {
        // 获取数据库中的token（已经格式化好的）
        QString dbToken = dbManager->getAuthToken();
        
        if (!dbToken.isEmpty()) {
            // 保存到本地设置中
            saveAuthToken(dbToken);
            dbManager->closeDatabase();
            return dbToken;
        }
        
        dbManager->closeDatabase();
    }
    return token;
}

// 保存认证token到设置中
void MainWindow::saveAuthToken(const QString &token)
{
    QSettings settings(QCoreApplication::organizationName(), QCoreApplication::applicationName());
    settings.setValue("auth/token", token);
}

void MainWindow::backupRegistry()
{
    QString backupPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/backups";
    if (m_powerShellRunner) {
        m_powerShellRunner->backupRegistry(backupPath);
    }
}

bool MainWindow::modifyRegistry()
{
    QString newGuid = generateMachineId();
    if (m_powerShellRunner) {
        m_powerShellRunner->modifyRegistry(newGuid);
        return true;
    }
    return false;
}

QString MainWindow::decodeProcessOutput(const QByteArray &output)
{
    QTextCodec *codec = QTextCodec::codecForName("System");
    if (!codec) {
        codec = QTextCodec::codecForLocale();
    }
    return codec->toUnicode(output);
}
