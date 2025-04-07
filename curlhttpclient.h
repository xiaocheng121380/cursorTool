#ifndef CURLHTTPCLIENT_H
#define CURLHTTPCLIENT_H

#include <QObject>
#include <QMap>
#include <QByteArray>
#include <windows.h>
#include "logmanager.h"
#include <QLibrary>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

// HTTP响应结构
struct CurlResponse {
    bool success; // 请求是否成功
    int statusCode; // HTTP状态码
    QByteArray data; // 响应数据
    QString errorMessage; // 错误信息
};

class CurlHttpClient : public QObject
{
    Q_OBJECT
public:
    explicit CurlHttpClient(QObject *parent = nullptr);
    virtual ~CurlHttpClient();

    // 检查是否成功初始化
    bool isInitialized() const;
    
    // 获取DLL状态信息
    QString getDllStatus() const;
    
    // 同步HTTP请求方法
    CurlResponse get(const QString &url, const QMap<QString, QString> &headers = QMap<QString, QString>());
    CurlResponse post(const QString &url, const QByteArray &data, const QMap<QString, QString> &headers = QMap<QString, QString>());
    
    bool loadDll();
    void unloadDll();
    bool isLoaded() const { return m_loaded; }
    
    // 设置认证Cookie
    void setAuthToken(const QString &token);
    
    // 获取当前认证Cookie
    QString getAuthCookie() const { return m_authCookie; }
    
    // 发送HTTP请求
    QJsonObject sendRequest(const QString &url, const QString &method = "GET", const QJsonObject &data = QJsonObject());

private:
    // DLL加载状态跟踪
    bool m_initialized;
    QString m_dllStatus;
    HMODULE m_hDll;
    LogManager* m_logManager;
    
    // libcurl函数指针类型定义
    typedef void* (*curl_easy_init_func)();
    typedef void (*curl_easy_cleanup_func)(void*);
    typedef int (*curl_easy_setopt_func)(void*, int, ...);
    typedef int (*curl_easy_perform_func)(void*);
    typedef const char* (*curl_easy_strerror_func)(int);
    typedef int (*curl_easy_getinfo_func)(void*, int, ...);
    typedef struct curl_slist* (*curl_slist_append_func)(struct curl_slist*, const char*);
    typedef void (*curl_slist_free_all_func)(struct curl_slist*);
    typedef const char* (*curl_version_func)();
    
    // 函数指针
    curl_easy_init_func m_curl_easy_init;
    curl_easy_cleanup_func m_curl_easy_cleanup;
    curl_easy_setopt_func m_curl_easy_setopt;
    curl_easy_perform_func m_curl_easy_perform;
    curl_easy_strerror_func m_curl_easy_strerror;
    curl_easy_getinfo_func m_curl_easy_getinfo;
    curl_slist_append_func m_curl_slist_append;
    curl_slist_free_all_func m_curl_slist_free_all;
    curl_version_func m_curl_version;
    
    // 加载libcurl动态库
    bool loadCurlLibrary();
    
    // 获取函数指针
    bool getFunctionPointers();
    
    // 辅助方法 - curl写回调函数
    static size_t writeCallback(char *ptr, size_t size, size_t nmemb, void *userdata);
    
    // 检查DLL是否存在并尝试复制到当前目录
    bool checkAndCopyDll();
    
    bool initializeCurl();
    void cleanupCurl();
    
    // 认证Cookie
    QString m_authCookie;
    bool m_loaded;
};

#endif // CURLHTTPCLIENT_H