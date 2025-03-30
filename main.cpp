#include "mainwindow.h"
#include <QApplication>
#include <QSharedMemory>
#include <QMessageBox>
#include <QLocale>
#include <QTranslator>
#include <QLibraryInfo>
#include <QIcon>

int main(int argc, char *argv[])
{
    // 设置应用程序属性
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication::setQuitOnLastWindowClosed(false);
    
    QApplication app(argc, argv);
    
    // 设置应用程序图标
    app.setWindowIcon(QIcon(":/images/cursor_logo.ico"));
    
    // 加载翻译
    QTranslator translator;
    const QStringList uiLanguages = QLocale::system().uiLanguages();
    for (const QString &locale : uiLanguages) {
        const QString baseName = "cursor_" + QLocale(locale).name();
        if (translator.load(":/i18n/" + baseName)) {
            app.installTranslator(&translator);
            break;
        }
    }
    
    // 创建并显示主窗口
    MainWindow w;
    w.show();
    
    return app.exec();
}
