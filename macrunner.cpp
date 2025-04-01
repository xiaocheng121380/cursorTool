#include "macrunner.h"
#include <QFile>
#include <QDateTime>
#include <QStandardPaths>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDir>
#include <QProcess>
#include <QDebug>

MacRunner::MacRunner(QObject *parent) : QObject(parent)
{
    m_process = new QProcess(this);
    connect(m_process, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(processFinished(int, QProcess::ExitStatus)));
    connect(m_process, SIGNAL(readyReadStandardOutput()), this, SLOT(readProcessOutput()));
    connect(m_process, SIGNAL(readyReadStandardError()), this, SLOT(readProcessError()));
}

MacRunner::~MacRunner()
{
    if (m_process->state() != QProcess::NotRunning) {
        m_process->kill();
        m_process->waitForFinished();
    }
}

QString MacRunner::getCursorConfigPath() const
{
    // Mac 系统下 Cursor 的配置文件路径
    QString homeDir = QDir::homePath();
    return homeDir + "/Library/Application Support/Cursor/User/settings.json";
}

QString MacRunner::getCurrentDeviceId() const
{
    QFile file(getCursorConfigPath());
    if (!file.open(QIODevice::ReadOnly)) {
        const_cast<MacRunner*>(this)->scriptError("无法读取 Cursor 配置文件");
        return QString();
    }

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();

    if (doc.isNull()) {
        const_cast<MacRunner*>(this)->scriptError("配置文件格式错误");
        return QString();
    }

    QJsonObject obj = doc.object();
    return obj["deviceId"].toString();
}

bool MacRunner::createBackupDirectory(const QString &backupPath) const
{
    QDir dir(backupPath);
    if (!dir.exists()) {
        return dir.mkpath(".");
    }
    return true;
}

void MacRunner::backupConfig(const QString &backupPath)
{
    if (!createBackupDirectory(backupPath)) {
        emit operationCompleted(false, "创建备份目录失败");
        return;
    }

    QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss");
    QString backupDir = backupPath + "/cursor_backup_" + timestamp;
    
    // 创建备份目录
    QDir dir;
    if (!dir.mkpath(backupDir)) {
        emit operationCompleted(false, "创建备份目录失败");
        return;
    }

    // 复制整个 User 目录
    QString userDir = QDir::homePath() + "/Library/Application Support/Cursor/User";
    QDir sourceDir(userDir);
    
    if (!sourceDir.exists()) {
        emit operationCompleted(false, "Cursor 用户目录不存在");
        return;
    }

    // 递归复制目录
    if (!copyDirectory(userDir, backupDir + "/User")) {
        emit operationCompleted(false, "备份用户数据失败");
        return;
    }

    // 获取当前设备ID
    QString currentDeviceId = getCurrentDeviceId();
    emit backupCompleted(true, backupDir, currentDeviceId);
}

bool MacRunner::copyDirectory(const QString &sourcePath, const QString &destPath)
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

void MacRunner::modifyConfig(const QString &newGuid)
{
    QFile file(getCursorConfigPath());
    if (!file.open(QIODevice::ReadOnly)) {
        emit operationCompleted(false, "无法读取配置文件");
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();

    if (doc.isNull()) {
        emit operationCompleted(false, "配置文件格式错误");
        return;
    }

    QString previousGuid = getCurrentDeviceId();
    QJsonObject obj = doc.object();
    obj["deviceId"] = newGuid;

    QJsonDocument newDoc(obj);
    if (!file.open(QIODevice::WriteOnly)) {
        emit operationCompleted(false, "无法写入配置文件");
        return;
    }

    file.write(newDoc.toJson());
    file.close();

    emit modifyCompleted(true, newGuid, previousGuid);
}

void MacRunner::backupAndModifyConfig(const QString &backupPath, const QString &newGuid)
{
    backupConfig(backupPath);
    modifyConfig(newGuid);
}

bool MacRunner::runCommand(const QStringList &arguments)
{
    m_process->start("sh", arguments);
    return m_process->waitForFinished();
}

void MacRunner::processFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    if (exitStatus == QProcess::NormalExit && exitCode == 0) {
        emit operationCompleted(true, "操作完成");
    } else {
        emit operationCompleted(false, "操作失败");
    }
}

void MacRunner::readProcessOutput()
{
    QString output = QString::fromUtf8(m_process->readAllStandardOutput());
    emit scriptOutput(output);
}

void MacRunner::readProcessError()
{
    QString error = QString::fromUtf8(m_process->readAllStandardError());
    emit scriptError(error);
} 