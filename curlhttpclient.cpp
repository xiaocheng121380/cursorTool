#include "curlhttpclient.h"
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QCoreApplication>
#include <sstream>
#include <iomanip>
#include "logmanager.h"

// curl常量定义
#define CURLOPT_URL               10002
#define CURLOPT_WRITEFUNCTION     20011
#define CURLOPT_WRITEDATA         10001
#define CURLOPT_HTTPHEADER        10023
#define CURLOPT_POST              47
#define CURLOPT_POSTFIELDS        10015
#define CURLOPT_POSTFIELDSIZE     60
#define CURLOPT_FOLLOWLOCATION    52
#define CURLOPT_TIMEOUT           13
#define CURLOPT_SSL_VERIFYPEER    64
#define CURLOPT_SSL_VERIFYHOST    81
#define CURLOPT_VERBOSE           41
#define CURLOPT_ERRORBUFFER       10010
#define CURLINFO_RESPONSE_CODE    2097154
#define CURL_ERROR_SIZE           256

// curl回调函数用于接收响应数据
size_t CurlHttpClient::writeCallback(char *ptr, size_t size, size_t nmemb, void *userdata) {
    std::string *response = static_cast<std::string*>(userdata);
    size_t realSize = size * nmemb;
    response->append(ptr, realSize);
    return realSize;
}

// 静态回调函数用于接收响应头
static size_t HeaderCallback(char* buffer, size_t size, size_t nitems, void* userdata)
{
    QStringList* headers = (QStringList*)userdata;
    QString headerLine = QString::fromUtf8(buffer, int(nitems * size)).trimmed();
    if (!headerLine.isEmpty()) {
        headers->append(headerLine);
    }
    return nitems * size;
}

CurlHttpClient::CurlHttpClient(QObject *parent)
    : QObject(parent)
    , m_initialized(false)
    , m_dllStatus("未初始化")
    , m_hDll(NULL)
    , m_curl_easy_init(nullptr)
    , m_curl_easy_cleanup(nullptr)
    , m_curl_easy_setopt(nullptr)
    , m_curl_easy_perform(nullptr)
    , m_curl_easy_strerror(nullptr)
    , m_curl_easy_getinfo(nullptr)
    , m_curl_slist_append(nullptr)
    , m_curl_slist_free_all(nullptr)
    , m_curl_version(nullptr)
    , m_authCookie("")
{
    m_logManager = LogManager::instance();
    
    // 加载DLL
    if (!loadCurlLibrary()) {
        return;
    }
    
    // 获取函数指针
    if (!getFunctionPointers()) {
        return;
    }
    
    // 初始化curl
    if (!initializeCurl()) {
        return;
    }
    
    m_initialized = true;
    m_dllStatus = "初始化成功";
}

CurlHttpClient::~CurlHttpClient() {
    if (m_hDll) {
        FreeLibrary(m_hDll);
        m_hDll = NULL;
    }
}

bool CurlHttpClient::isInitialized() const {
    return m_initialized;
}

QString CurlHttpClient::getDllStatus() const {
    return m_dllStatus;
}

bool CurlHttpClient::checkAndCopyDll() {
    // 直接返回true，不再检查和复制DLL
    return true;
}

bool CurlHttpClient::loadCurlLibrary() {
    QString dllPath = QCoreApplication::applicationDirPath() + "/libcurl-x64.dll";
    
    m_hDll = LoadLibraryW((const wchar_t*)dllPath.utf16());
    if (!m_hDll) {
        DWORD error = GetLastError();
        std::stringstream ss;
        ss << "无法加载libcurl-x64.dll (错误代码: " << error << ")";
        m_dllStatus = QString::fromStdString(ss.str());
        return false;
    }
    
    return true;
}

bool CurlHttpClient::getFunctionPointers() {
    // 获取所有必需的函数指针
    m_curl_easy_init = (curl_easy_init_func)GetProcAddress(m_hDll, "curl_easy_init");
    m_curl_easy_cleanup = (curl_easy_cleanup_func)GetProcAddress(m_hDll, "curl_easy_cleanup");
    m_curl_easy_setopt = (curl_easy_setopt_func)GetProcAddress(m_hDll, "curl_easy_setopt");
    m_curl_easy_perform = (curl_easy_perform_func)GetProcAddress(m_hDll, "curl_easy_perform");
    m_curl_easy_strerror = (curl_easy_strerror_func)GetProcAddress(m_hDll, "curl_easy_strerror");
    m_curl_easy_getinfo = (curl_easy_getinfo_func)GetProcAddress(m_hDll, "curl_easy_getinfo");
    m_curl_slist_append = (curl_slist_append_func)GetProcAddress(m_hDll, "curl_slist_append");
    m_curl_slist_free_all = (curl_slist_free_all_func)GetProcAddress(m_hDll, "curl_slist_free_all");
    m_curl_version = (curl_version_func)GetProcAddress(m_hDll, "curl_version");
    
    // 检查所有函数指针是否获取成功
    if (!m_curl_easy_init || !m_curl_easy_cleanup || !m_curl_easy_setopt || 
        !m_curl_easy_perform || !m_curl_easy_strerror || !m_curl_easy_getinfo || 
        !m_curl_slist_append || !m_curl_slist_free_all || !m_curl_version) {
        
        DWORD error = GetLastError();
        std::stringstream ss;
        ss << "获取函数指针失败 (错误代码: " << error << ")";
        m_dllStatus = QString::fromStdString(ss.str());
        return false;
    }
    
    return true;
}

CurlResponse CurlHttpClient::get(const QString &url, const QMap<QString, QString> &headers) {
    CurlResponse response;
    response.success = false;
    response.statusCode = 0;
    
    if (!m_initialized) {
        response.errorMessage = "libcurl库未初始化";
        return response;
    }
    
    // 初始化curl
    void *curl = m_curl_easy_init();
    if (!curl) {
        response.errorMessage = "curl_easy_init失败";
        return response;
    }
    
    std::string responseData;
    char errorBuffer[CURL_ERROR_SIZE] = {0};
    
    // 设置URL
    QByteArray urlBytes = url.toUtf8();
    const char* urlStr = urlBytes.constData();
    
    m_curl_easy_setopt(curl, CURLOPT_URL, urlStr);
    m_curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
    m_curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseData);
    m_curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    m_curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
    m_curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    m_curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
    m_curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, errorBuffer);
    
    // 设置HTTP头
    struct curl_slist *headersList = NULL;
    
    QMapIterator<QString, QString> i(headers);
    while (i.hasNext()) {
        i.next();
        QByteArray headerLine = (i.key() + ": " + i.value()).toUtf8();
        headersList = m_curl_slist_append(headersList, headerLine.constData());
    }
    
    if (headersList) {
        m_curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headersList);
    }

    // 执行请求
    int res = m_curl_easy_perform(curl);
    
    // 处理结果
    if (res == 0) { // CURLE_OK = 0
        response.success = true;
        
        // 将std::string转换为QByteArray
        response.data = QByteArray(responseData.data(), responseData.size());
        
        // 获取HTTP状态码
        long httpCode = 0;
        m_curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
        response.statusCode = httpCode;
    } else {
        response.success = false;
        response.errorMessage = m_curl_easy_strerror ? 
            QString::fromUtf8(m_curl_easy_strerror(res)) : 
            QString("错误代码: %1").arg(res);
        
        if (errorBuffer[0]) {
            response.errorMessage += QString(" - ") + QString::fromUtf8(errorBuffer);
        }
    }
    
    // 清理
    if (headersList) {
        m_curl_slist_free_all(headersList);
    }
    m_curl_easy_cleanup(curl);
    
    return response;
}

CurlResponse CurlHttpClient::post(const QString &url, const QByteArray &data, const QMap<QString, QString> &headers) {
    CurlResponse response;
    response.success = false;
    response.statusCode = 0;
    
    if (!m_initialized) {
        response.errorMessage = "libcurl库未初始化";
        return response;
    }
    
    // 输出调试信息
    qDebug() << "发送POST请求到: " << url;
    
    // 初始化curl
    void *curl = m_curl_easy_init();
    if (!curl) {
        response.errorMessage = "curl_easy_init失败";
        return response;
    }
    
    std::string responseData;
    char errorBuffer[CURL_ERROR_SIZE] = {0};
    
    // 设置URL
    QByteArray urlBytes = url.toUtf8();
    const char* urlStr = urlBytes.constData();
    
    m_curl_easy_setopt(curl, CURLOPT_URL, urlStr);
    m_curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
    m_curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseData);
    m_curl_easy_setopt(curl, CURLOPT_POST, 1L);
    m_curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data.constData());
    m_curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)data.size());
    m_curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    m_curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
    m_curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    m_curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
    m_curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, errorBuffer);
    
    // 设置HTTP头
    struct curl_slist *headersList = NULL;
    
    // 设置默认内容类型，如果未在headers中指定
    bool hasContentType = false;
    
    QMapIterator<QString, QString> i(headers);
    while (i.hasNext()) {
        i.next();
        if (i.key().toLower() == "content-type") {
            hasContentType = true;
        }
        QByteArray headerLine = (i.key() + ": " + i.value()).toUtf8();
        headersList = m_curl_slist_append(headersList, headerLine.constData());
    }
    
    if (!hasContentType) {
        QByteArray contentTypeHeader = "Content-Type: application/json";
        headersList = m_curl_slist_append(headersList, contentTypeHeader.constData());
    }
    
    if (headersList) {
        m_curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headersList);
    }
    
    // 执行请求
    int res = m_curl_easy_perform(curl);
    
    // 处理结果
    if (res == 0) { // CURLE_OK = 0
        response.success = true;
        response.data = QByteArray(responseData.c_str(), responseData.size());
        
        // 获取HTTP状态码
        long httpCode = 0;
        m_curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
        response.statusCode = httpCode;
    } else {
        response.success = false;
        response.errorMessage = m_curl_easy_strerror ? 
            QString::fromUtf8(m_curl_easy_strerror(res)) : 
            QString("错误代码: %1").arg(res);
        
        if (errorBuffer[0]) {
            response.errorMessage += QString(" - ") + QString::fromUtf8(errorBuffer);
        }
    }
    
    // 清理
    if (headersList) {
        m_curl_slist_free_all(headersList);
    }
    m_curl_easy_cleanup(curl);
    
    return response;
}

bool CurlHttpClient::initializeCurl() {
    // 获取curl版本
    if (m_curl_version) {
        const char* version = m_curl_version();
        if (!version) {
            return false;
        }
    } else {
        return false;
    }
    
    // 检查主要函数指针
    if (!m_curl_easy_init || !m_curl_easy_setopt || !m_curl_easy_perform || !m_curl_easy_cleanup) {
        return false;
    }
    
    return true;
}

void CurlHttpClient::cleanupCurl() {
    // 目前没有需要清理的curl全局资源
    // 如果将来有需要清理的资源，可以在这里添加
}

void CurlHttpClient::setAuthToken(const QString &token)
{
    // 检查是否已经是格式化过的完整cookie字符串
    if (token.contains("NEXT_LOCALE=") || token.contains("; ")) {
        m_authCookie = token;
        return;
    }
    
    // 从简单token构建完整cookie格式
    if (token.startsWith("WorkosCursorSessionToken=")) {
        // 提取JWT部分
        QString jwtPart = token.mid(QString("WorkosCursorSessionToken=").length());
        
        // 构建完整cookie格式，使用固定的前缀和格式
        m_authCookie = "NEXT_LOCALE=cn; IndrX2ZuSmZramJSX0NIYUZoRzRzUGZ0cENIVHpHNXk0VE0ya2ZiUkVzQU14X2Fub255bW91c1VzZXJJZCI%3D=IjVkOGEyYjE0LTBkODItNGIyYy04NjQzLTVmYmUzYTUwNjY2ZiI=; WorkosCursorSessionToken=user_01JR52W174EFYYJTCZNXZD9MA1%3A%3A" + jwtPart;
    } else {
        // 如果不是以WorkosCursorSessionToken开始的，就当作是JWT部分
        m_authCookie = "NEXT_LOCALE=cn; IndrX2ZuSmZramJSX0NIYUZoRzRzUGZ0cENIVHpHNXk0VE0ya2ZiUkVzQU14X2Fub255bW91c1VzZXJJZCI%3D=IjVkOGEyYjE0LTBkODItNGIyYy04NjQzLTVmYmUzYTUwNjY2ZiI=; WorkosCursorSessionToken=user_01JR52W174EFYYJTCZNXZD9MA1%3A%3A" + token;
    }
}

QJsonObject CurlHttpClient::sendRequest(const QString &url, const QString &method, const QJsonObject &data)
{
    QJsonObject response;
    // 简单返回空对象，因为不需要网络模块相关代码
    return response;
}