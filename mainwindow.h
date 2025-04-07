#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLabel>
#include <QTextEdit>
#include <QProgressBar>
#include <QProcess>
#include <QMessageBox>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDateTime>
#include <QTextCodec>
#include <QSettings>
#include <QStandardPaths>
#include <QClipboard>
#include <QSqlQuery>
#include <QDebug>
#include "powershellrunner.h"
#include "macrunner.h"
#include "cursorapi.h"
#include "logmanager.h"
#include "curlhttpclient.h"
#include "databasemanager.h"
#include "cursordatareader.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    bool isMacOS() const;
    QString getBackupPath() const;

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void closeCursor();      // 关闭Cursor程序
    void restartCursor();    // 重启Cursor程序
    void clearCursorData();  // 清除Cursor数据

    // 操作完成回调
    void onOperationCompleted(bool success, const QString &message);
    void onBackupCompleted(bool success, const QString &backupFile, const QString &currentGuid);
    void onModifyCompleted(bool success, const QString &newGuid, const QString &previousGuid);
    void onScriptOutput(const QString &output);
    void onScriptError(const QString &error);

    // 用户信息处理槽
    void onUserInfoUpdated(const CursorUserInfo &info);
    void onUserInfoError(const QString &error);

    // 使用统计处理槽
    void onUsageInfoUpdated(const CursorUsageInfo &info);
    void onUsageInfoError(const QString &error);

    // 日志处理槽
    void onLogMessage(const QString &formattedMessage);

    // 一键更换按钮点击槽
    void onOneClickResetClicked();
    void backupRegistry();
    bool modifyRegistry();
    bool checkAndCreateRegistryPath(const QString &path);

private:
    Ui::MainWindow *ui;
    QLabel *statusLabel;     // 状态显示标签
    QTextEdit *logTextArea;  // 日志文本区域
    PowerShellRunner *m_powerShellRunner;  // Windows PowerShell 运行器
    MacRunner *m_macRunner;  // Mac 系统运行器
    CursorApi *m_cursorApi;  // Cursor API 管理器
    LogManager *m_logManager; // 日志管理器

    // 生成ID相关
    QString generateMachineId();    // 生成机器ID
    QString generateMacMachineId(); // 生成MAC机器ID
    QString generateUUID();         // 生成UUID
    
    // 编码处理
    QString decodeProcessOutput(const QByteArray &output);

    // 初始化函数
    void initializeUi();
    void initializeConnections();
    void startInitialDataFetch();
    
    // 更新UI函数
    void updateUserInfoDisplay(const CursorUserInfo &info);
    void updateUsageInfoDisplay(const CursorUsageInfo &info);
    
    // 认证Token管理
    QString loadAuthToken();
    void saveAuthToken(const QString &token);
};
#endif // MAINWINDOW_H 