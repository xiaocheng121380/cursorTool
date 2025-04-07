#include "logmanager.h"
#include <QDateTime>

// 初始化静态成员变量
LogManager* LogManager::m_instance = nullptr;

// 获取单例实例
LogManager* LogManager::instance()
{
    if (!m_instance) {
        m_instance = new LogManager();
    }
    return m_instance;
}

// 清理单例实例
void LogManager::cleanup()
{
    if (m_instance) {
        delete m_instance;
        m_instance = nullptr;
    }
}

LogManager::LogManager(QObject *parent)
    : QObject(parent)
{
}

LogManager::~LogManager()
{
    // 析构函数实现
}

QString LogManager::formatLogMessage(const QString &message, LogType type)
{
    QString color;
    switch (type) {
        case Info:
            color = "#FFFFFF";  // 白色
            break;
        case Success:
            color = "#00E676";  // 绿色
            break;
        case Warning:
            color = "#FFB74D";  // 橙色
            break;
        case Error:
            color = "#FF1744";  // 红色
            break;
        case Detail:
            color = "#00E5FF";  // 蓝色
            break;
    }

    QString timestamp = getCurrentTimestamp();
    return QString("<span style='color: #888888'>[%1]</span> <span style='color: %2'>%3</span>")
        .arg(timestamp)
        .arg(color)
        .arg(message);
}

QString LogManager::getCurrentTimestamp()
{
    return QDateTime::currentDateTime().toString("HH:mm:ss");
}

void LogManager::logInfo(const QString &message)
{
    log(message, Info);
}

void LogManager::logSuccess(const QString &message)
{
    log(message, Success);
}

void LogManager::logWarning(const QString &message)
{
    log(message, Warning);
}

void LogManager::logError(const QString &message)
{
    log(message, Error);
}

void LogManager::logDetail(const QString &message)
{
    log(message, Detail);
}

void LogManager::log(const QString &message, LogType type)
{
    QString formattedMessage = formatLogMessage(message, type);
    emit logMessage(formattedMessage);
} 