#include "databasemanager.h"
#include "logmanager.h"
#include "cursordatareader.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QCoreApplication>
#include <QFileDialog>
#include <QMessageBox>
#include <QSettings>
#include <QDebug>
#include <QThread>

DatabaseManager* DatabaseManager::m_instance = nullptr;
const QString DatabaseManager::DB_CONNECTION_NAME = "cursor_db_connection";

DatabaseManager* DatabaseManager::instance()
{
    if (!m_instance) {
        m_instance = new DatabaseManager();
    }
    return m_instance;
}

DatabaseManager::DatabaseManager(QObject *parent)
    : QObject(parent)
    , m_isConnected(false)
{
    // 确保SQLITE驱动可用
    if (!QSqlDatabase::isDriverAvailable("QSQLITE")) {
        LogManager::instance()->logError("SQLite驱动不可用");
    }
}

DatabaseManager::~DatabaseManager()
{
    closeDatabase();
}

bool DatabaseManager::openDatabase(const QString &dbPath)
{
    // 如果已经连接，先关闭
    if (m_isConnected) {
        closeDatabase();
    }
    
    QString path = dbPath;
    
    // 如果没有提供路径，尝试自动查找
    if (path.isEmpty()) {
        QStringList possiblePaths = findPossibleDatabasePaths();
        
        for (const QString &testPath : possiblePaths) {
            if (testDatabaseConnection(testPath)) {
                path = testPath;
                break;
            }
        }
        
        if (path.isEmpty()) {
            LogManager::instance()->logError("未找到可用的Cursor数据库");
            return false;
        }
    } else {
        // 测试提供的路径是否可用
        if (!testDatabaseConnection(path)) {
            LogManager::instance()->logError("提供的数据库路径不可用: " + path);
            return false;
        }
    }
    
    // 保存当前路径
    m_currentDbPath = path;
    
    // 创建数据库连接
    m_db = QSqlDatabase::addDatabase("QSQLITE", DB_CONNECTION_NAME);
    m_db.setDatabaseName(path);
    
    // 打开数据库
    if (!m_db.open()) {
        LogManager::instance()->logError("无法打开数据库: " + m_db.lastError().text());
        return false;
    }
    
    m_isConnected = true;
    
    return true;
}

void DatabaseManager::closeDatabase()
{
    if (m_isConnected) {
        // 确保所有查询都已完成
        if (m_db.isOpen()) {
            // 关闭数据库连接
            m_db.close();
        }
        
        // 等待一小段时间确保所有查询都已完成
        QThread::msleep(100);
        
        // 移除数据库连接
        if (QSqlDatabase::contains(DB_CONNECTION_NAME)) {
            // 确保所有查询对象都被销毁
            QSqlDatabase::database(DB_CONNECTION_NAME).close();
            QSqlDatabase::removeDatabase(DB_CONNECTION_NAME);
        }
        
        m_isConnected = false;
        m_currentDbPath.clear();
    }
}

QString DatabaseManager::getAuthToken()
{
    if (!m_isConnected) {
        LogManager::instance()->logWarning("尝试在未连接数据库时获取认证Token");
        return QString();
    }
    
    // 直接使用cursorAuth/refreshToken键查询Token
    QSqlQuery query(m_db);
    query.prepare("SELECT value FROM ItemTable WHERE key = :key");
    query.bindValue(":key", "cursorAuth/refreshToken");
    
    QString token;
    if (query.exec() && query.next()) {
        token = query.value(0).toString();
        qDebug() << "从数据库获取到的Token:" << token;
    } else {
        // 如果未能直接获取，尝试其他可能的键名
        QStringList possibleKeys = {
            "authentication.token",          // 可能的键名1
            "cursor.authToken",              // 可能的键名2
            "cursor:auth:token",             // 可能的键名3
            "cursor.authentication.token",   // 可能的键名4
            "workspaceStorage:cursor.token", // 可能的键名5
            "cursor.session.token"           // 可能的键名6
        };
        
        for (const QString &key : possibleKeys) {
            QVariant value = getItemValue(key);
            
            if (value.isValid() && !value.isNull()) {
                token = value.toString();
                qDebug() << "从备用键" << key << "获取到的Token:" << token;
                break;
            }
        }
    }
    
    if (token.isEmpty()) {
        LogManager::instance()->logWarning("未找到认证Token");
        return QString();
    }
    // 格式化token，正确处理%符号
    QString formattedToken = QString("NEXT_LOCALE=cn; IndrX2ZuSmZramJSX0NIYUZoRzRzUGZ0cENIVHpHNXk0VE0ya2ZiUkVzQU14X2Fub255bW91c1VzZXJJZCI%3D=IjVkOGEyYjE0LTBkODItNGIyYy04NjQzLTVmYmUzYTUwNjY2ZiI=; WorkosCursorSessionToken=%1%3A%3A%2")
        .arg(getUserId())
        .arg(token);
    
    qDebug() << "格式化后的Token:" << formattedToken;
    
    return formattedToken;
}

QString DatabaseManager::getUserId()
{
    // 从CursorSessionData中获取did
    CursorSessionData sessionData = CursorDataReader::readSessionData();
    if (sessionData.userId.isEmpty()) {
        return QString();
    }
    
    // 处理did，提取user_后面的部分
    if (sessionData.userId.contains("|")) {
        return sessionData.userId.section("|", 1);
    }
    return sessionData.userId;
}

QStringList DatabaseManager::findPossibleDatabasePaths()
{
    QStringList paths;
    
#ifdef Q_OS_WIN
    // Windows路径 - 直接获取用户目录
    QString userProfile = QDir::homePath();
    
    // 标准Cursor路径
    paths << QDir::toNativeSeparators(userProfile + "/AppData/Roaming/Cursor/User/globalStorage/state.vscdb");
#else
    // macOS路径
    QString userHome = QDir::homePath();
    paths << QDir::toNativeSeparators(userHome + "/Library/Application Support/Cursor/User/globalStorage/state.vscdb");
#endif
    
    // 去除不存在的路径
    QStringList existingPaths;
    
    for (const QString &path : paths) {
        QFileInfo info(path);
        if (info.exists() && info.isFile()) {
            existingPaths << path;
        }
    }
    
    if (existingPaths.isEmpty()) {
        LogManager::instance()->logWarning("没有找到Cursor数据库文件");
    }
    
    return existingPaths;
}

bool DatabaseManager::testDatabaseConnection(const QString &dbPath)
{
    QFileInfo fileInfo(dbPath);
    if (!fileInfo.exists() || !fileInfo.isFile()) {
        return false;
    }
    
    return true;
}

QVariant DatabaseManager::getItemValue(const QString &key)
{
    if (!m_isConnected || !m_db.isOpen()) {
        return QVariant();
    }
    
    // 直接从ItemTable表中查询
    QSqlQuery query(m_db);
    query.prepare("SELECT value FROM ItemTable WHERE key = :key");
    query.bindValue(":key", key);
    
    if (query.exec() && query.next()) {
        return query.value(0);
    }
    return QVariant();
}