#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H

#include <QObject>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>
#include <QMap>
#include <QDebug>
#include <QDir>
#include <QStandardPaths>
#include <QWidget>

/**
 * @brief 用于管理和查询Cursor的state.vscdb数据库
 * 
 * 这个类提供了打开、关闭和查询Cursor数据库的功能，可以读取认证token和使用统计等信息
 */
class DatabaseManager : public QObject
{
    Q_OBJECT
public:
    /**
     * @brief 单例模式获取实例
     * @return DatabaseManager实例
     */
    static DatabaseManager* instance();
    
    /**
     * @brief 打开数据库连接
     * @param dbPath 数据库文件路径，如果为空则自动寻找
     * @return 是否成功打开数据库
     */
    bool openDatabase(const QString &dbPath = QString());
    
    /**
     * @brief 关闭数据库连接
     */
    void closeDatabase();
    
    /**
     * @brief 查询Cursor认证Token
     * @return 认证Token，格式为"WorkosCursorSessionToken=xxx"
     */
    QString getAuthToken();
    
    /**
     * @brief 获取用户ID
     * @return 用户ID
     */
    QString getUserId();
    
    /**
     * @brief 获取所有可能的Cursor数据库路径
     * @return 路径列表
     */
    QStringList findPossibleDatabasePaths();
    
    /**
     * @brief 测试数据库连接
     * @param dbPath 数据库文件路径
     * @return 是否可连接
     */
    bool testDatabaseConnection(const QString &dbPath);
    
    
    /**
     * @brief 获取当前数据库路径
     * @return 数据库路径
     */
    QString getCurrentDatabasePath() const { return m_currentDbPath; }
    
    /**
     * @brief 获取数据库对象，用于直接执行SQL
     * @return 数据库对象
     */
    QSqlDatabase getDatabase() const { return m_db; }
    
    /**
     * @brief 检查数据库是否已连接
     * @return 是否已连接
     */
    bool isConnected() const { return m_isConnected; }
    
private:
    static DatabaseManager* m_instance;
    QSqlDatabase m_db;
    bool m_isConnected;
    QString m_currentDbPath;
    
    // 数据库连接名称
    static const QString DB_CONNECTION_NAME;
    
    DatabaseManager(QObject *parent = nullptr);
    ~DatabaseManager();
    
    /**
     * @brief 查询Item表中的值
     * @param key 要查询的键
     * @return 查询结果
     */
    QVariant getItemValue(const QString &key);
};

#endif // DATABASEMANAGER_H 