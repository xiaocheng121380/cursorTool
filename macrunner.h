#ifndef MACRUNNER_H
#define MACRUNNER_H

#include <QObject>
#include <QProcess>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>

class MacRunner : public QObject
{
    Q_OBJECT

public:
    explicit MacRunner(QObject *parent = nullptr);
    ~MacRunner();

    // 备份 Cursor 配置
    void backupConfig(const QString &backupPath);
    
    // 修改 Cursor 配置
    void modifyConfig(const QString &newGuid);
    
    // 执行备份和修改
    void backupAndModifyConfig(const QString &backupPath, const QString &newGuid);

    QString getCursorConfigPath() const;
    bool createBackupDirectory(const QString &backupPath) const;
    bool copyDirectory(const QString &sourcePath, const QString &destPath);

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
    
    // 获取当前设备 ID
    QString getCurrentDeviceId() const;
    
    // 执行 shell 命令
    bool runCommand(const QStringList &arguments);
};

#endif // MACRUNNER_H 