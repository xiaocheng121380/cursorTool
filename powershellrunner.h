#ifndef POWERSHELLRUNNER_H
#define POWERSHELLRUNNER_H

#include <QObject>
#include <QProcess>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>

class PowerShellRunner : public QObject
{
    Q_OBJECT

public:
    explicit PowerShellRunner(QObject *parent = nullptr);
    ~PowerShellRunner();

    // 执行注册表备份
    void backupRegistry(const QString &backupPath);
    
    // 执行注册表修改
    void modifyRegistry(const QString &newGuid);
    
    // 执行备份和修改
    void backupAndModifyRegistry(const QString &backupPath, const QString &newGuid);

signals:
    // 操作结果信号
    void operationCompleted(bool success, const QString &message);
    void backupCompleted(bool success, const QString &backupFile, const QString &currentGuid);
    void modifyCompleted(bool success, const QString &newGuid, const QString &previousGuid);
    void scriptOutput(const QString &output);
    void scriptError(const QString &error);
    
private slots:
    void processFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void readProcessOutput();
    void readProcessError();

private:
    QProcess *m_process;
    QString m_scriptContent;
    QString m_tempScriptPath;  // 临时脚本文件路径
    
    // 从资源中加载脚本
    bool loadScriptFromResources();
    
    // 创建PowerShell命令字符串
    QString createCommandString(const QString &action, const QString &backupPath, const QString &newGuid);
    
    // 执行脚本
    bool runScript(const QStringList &arguments);
};

#endif // POWERSHELLRUNNER_H 