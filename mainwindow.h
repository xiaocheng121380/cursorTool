#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLabel>
#include <QTextEdit>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void closeCursor();      // 关闭Cursor程序
    void clearCursorData();  // 清除Cursor数据
    void restartCursor();    // 重启Cursor程序
    void showBackups();      // 显示备份文件
    void openCursorAccount(); // 打开Cursor账号网站

private:
    Ui::MainWindow *ui;
    QLabel *statusLabel;     // 状态显示标签
    QTextEdit *logTextArea;  // 日志文本区域

    // 生成ID相关
    QString generateMachineId();    // 生成机器ID
    QString generateMacMachineId(); // 生成MAC机器ID
    QString generateUUID();         // 生成UUID

    // 注册表相关
    bool checkAndCreateRegistryPath(const QString &path); // 检查并创建注册表路径
    void backupRegistry();
    bool modifyRegistry();
    bool restoreRegistryBackup(const QString &backupFile); // 从备份恢复注册表
    
    // 日志相关
    void logMessage(const QString &message, const QString &color);
    void logInfo(const QString &message);
    void logSuccess(const QString &message);
    void logError(const QString &message);
    
    // 编码处理
    QString decodeProcessOutput(const QByteArray &output);
};
#endif // MAINWINDOW_H 