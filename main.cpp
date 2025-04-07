#include <QApplication>
#include <QTranslator>
#include <QDir>
#include <QFontDatabase>
#include <QIcon>
#include <QTextCodec>
#include <QDebug>
#include <QSettings>
#include "mainwindow.h"
#include "logmanager.h"
#include "curlhttpclient.h"
#include "cursordatareader.h"

int main(int argc, char *argv[])
{
    // 初始化应用程序
    QApplication a(argc, argv);

    // 设置应用程序信息
    QCoreApplication::setOrganizationName("Cursor Tool Team");
    QCoreApplication::setApplicationName("Cursor Tool");
    QCoreApplication::setApplicationVersion("1.0.0");

    // 初始化日志管理器
    LogManager::instance()->log("应用程序启动");

    // 检查工作目录
    LogManager::instance()->log("工作目录: " + QDir::currentPath());

    // 设置高DPI支持
    a.setAttribute(Qt::AA_UseHighDpiPixmaps);
    a.setAttribute(Qt::AA_EnableHighDpiScaling);

    // 设置应用图标
    a.setWindowIcon(QIcon(":/images/cursor-logo.png"));

    // 设置文本编码
    QTextCodec::setCodecForLocale(QTextCodec::codecForName("UTF-8"));

    // 读取session数据
    CursorSessionData sessionData = CursorDataReader::readSessionData();

    // 输出session信息
    qDebug() << "=== Cursor Session Information ===";
    qDebug() << "userId:" << sessionData.userId;
    qDebug() << "email:" << sessionData.email;
    qDebug() << "release:" << sessionData.release;
    qDebug() << "================================";

    // 创建主窗口
    MainWindow w;
    w.show();

    // 运行应用程序
    int result = a.exec();

    // 清理日志管理器
    LogManager::instance()->cleanup();

    return result;
}
