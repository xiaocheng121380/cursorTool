#ifndef LOGMANAGER_H
#define LOGMANAGER_H

#include <QObject>
#include <QString>

class LogManager : public QObject
{
    Q_OBJECT

public:
    // 日志类型枚举
    enum LogType {
        Info,       // 普通信息 - 白色
        Success,    // 成功信息 - 绿色
        Warning,    // 警告信息 - 橙色
        Error,      // 错误信息 - 红色
        Detail      // 详细信息 - 蓝色
    };
    Q_ENUM(LogType)

    // 获取单例实例
    static LogManager* instance();

    // 清理单例实例
    static void cleanup();
    
    ~LogManager();

    // 格式化日志消息
    static QString formatLogMessage(const QString &message, LogType type = Info);

public slots:
    // 日志记录方法
    void logInfo(const QString &message);
    void logSuccess(const QString &message);
    void logWarning(const QString &message);
    void logError(const QString &message);
    void logDetail(const QString &message);
    void log(const QString &message, LogType type = Info);

signals:
    // 日志消息信号
    void logMessage(const QString &formattedMessage);

private:
    // 私有构造函数，防止外部创建实例
    explicit LogManager(QObject *parent = nullptr);
    
    // 单例实例
    static LogManager* m_instance;
    
    // 获取当前时间戳
    static QString getCurrentTimestamp();
};

#endif // LOGMANAGER_H 