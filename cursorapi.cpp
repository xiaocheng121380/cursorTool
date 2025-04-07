#include "cursorapi.h"
#include "logmanager.h"
#include "curlhttpclient.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>
#include <QUrl>
#include <QSettings>
#include <QStandardPaths>
#include <QDir>
#include <QDateTime>
#include <QStringList>

CursorApi::CursorApi(QObject *parent)
    : QObject(parent)
    , m_logManager(LogManager::instance())
    , m_httpClient(new CurlHttpClient(this))
    , m_authToken("")
    , m_userInfoRequestInProgress(false)
    , m_usageInfoRequestInProgress(false)
    , m_deleteAccountRequestInProgress(false)
    , m_currentRequestType(RequestType::UserInfo)
{
    // 检查初始化状态
    if (!m_httpClient->isInitialized()) {
        m_logManager->log("CursorApi: CurlHttpClient初始化失败: " + m_httpClient->getDllStatus(), LogManager::Error);
    }
    
    m_logManager->log("CursorApi: 初始化完成");
}

CursorApi::~CursorApi()
{
    // 析构函数，不需要日志
}

void CursorApi::setAuthToken(const QString &token)
{
    m_authToken = token;
    
    // 同时设置HttpClient的认证令牌，以确保它能正确格式化Cookie
    if (m_httpClient) {
        m_httpClient->setAuthToken(token);
        m_authToken = m_httpClient->getAuthCookie(); // 使用格式化后的完整Cookie
    }
}

QString CursorApi::authToken() const
{
    return m_authToken;
}

QString CursorApi::buildApiUrl(const QString &endpoint)
{
    const QString baseUrl = "https://www.cursor.com/api";
    
    // 构建完整URL - 简单直接地连接，避免过度复杂的处理
    QString url;
    if (endpoint.isEmpty()) {
        url = baseUrl;
    } else if (endpoint.startsWith("/")) {
        url = baseUrl + endpoint;
    } else {
        url = baseUrl + "/" + endpoint;
    }
    
    // 使用QUrl进行验证
    QUrl validatedUrl(url);
    if (!validatedUrl.isValid()) {
        m_logManager->log("CursorApi: 警告 - 生成的URL不是有效格式: " + url, LogManager::Warning);
    }
    
    return url;
}

void CursorApi::fetchUserInfo()
{
    if (m_userInfoRequestInProgress) {
        return;
    }
    
    m_userInfoRequestInProgress = true;
    QString endpoint = "/auth/me";
    
    // 构建URL
    QString url = buildApiUrl(endpoint);
    
    // 设置请求头
    QMap<QString, QString> headers;
    headers["Accept"] = "application/json";
    headers["Content-Type"] = "application/json";
    
    // 添加认证Cookie
    if (!m_authToken.isEmpty()) {
        headers["Cookie"] = m_authToken;
        qDebug() << "添加认证Cookie: " << m_authToken;
    } else {
        m_logManager->log("CursorApi: 警告 - 认证令牌为空", LogManager::Warning);
    }
    
    // 发送请求并直接处理响应
    m_currentRequestType = RequestType::UserInfo;
    
    CurlResponse response = m_httpClient->get(url, headers);
    if (response.success) {
        handleResponse(response.data);
    } else {
        handleError(response.errorMessage);
    }
}

void CursorApi::fetchUsageInfo()
{
    if (m_usageInfoRequestInProgress) {
        return;
    }
    
    m_usageInfoRequestInProgress = true;
    QString endpoint = "/usage";
    
    // 构建URL
    QString url = buildApiUrl(endpoint);
    
    // 设置请求头
    QMap<QString, QString> headers;
    headers["Accept"] = "application/json";
    headers["Content-Type"] = "application/json";
    
    // 添加认证Cookie
    if (!m_authToken.isEmpty()) {
        headers["Cookie"] = m_authToken;
    } else {
        m_logManager->log("CursorApi: 警告 - 认证令牌为空", LogManager::Warning);
    }
    
    // 发送请求并直接处理响应
    m_currentRequestType = RequestType::UsageInfo;
    
    CurlResponse response = m_httpClient->get(url, headers);
    if (response.success) {
        handleResponse(response.data);
    } else {
        handleError(response.errorMessage);
    }
}

void CursorApi::deleteAccount()
{
    if (m_deleteAccountRequestInProgress) {
        return;
    }
    
    m_deleteAccountRequestInProgress = true;
    QString endpoint = "/dashboard/delete-account";
    
    // 构建URL
    QString url = buildApiUrl(endpoint);
    
    // 设置请求头
    QMap<QString, QString> headers;
    headers["Accept"] = "application/json";
    headers["Content-Type"] = "application/json";
    
    // 添加认证Cookie
    if (!m_authToken.isEmpty()) {
        headers["Cookie"] = m_authToken;
    } else {
        m_logManager->log("CursorApi: 警告 - 认证令牌为空", LogManager::Warning);
    }
    
    // 发送请求并直接处理响应
    m_currentRequestType = RequestType::DeleteAccount;
    
    // 发送空的POST请求体
    QByteArray emptyData = "{}";
    CurlResponse response = m_httpClient->post(url, emptyData, headers);
    
    // 只判断请求是否成功，不处理响应内容
    if (response.success) {
        m_deleteAccountRequestInProgress = false;
        emit accountDeleted();
    } else {
        m_deleteAccountRequestInProgress = false;
        emit accountDeleteError("删除账户失败: " + response.errorMessage);
    }
}

void CursorApi::handleResponse(const QByteArray &data)
{
    int responseSize = data.size();
    
    // 处理空响应
    if (responseSize == 0) {
        m_logManager->log("CursorApi: 警告 - 响应为空", LogManager::Warning);
        handleError("Empty response");
        return;
    }
    
    // 根据请求类型处理响应
    switch (m_currentRequestType) {
        case RequestType::UserInfo:
            m_userInfoRequestInProgress = false;
            parseUserInfoResponse(data);
            break;
        case RequestType::UsageInfo:
            m_usageInfoRequestInProgress = false;
            parseUsageInfoResponse(data);
            break;
        default:
            m_logManager->log("CursorApi: 未知的请求类型", LogManager::Warning);
            break;
    }
}

void CursorApi::handleError(const QString &error)
{
    // 根据请求类型处理错误
    switch (m_currentRequestType) {
        case RequestType::UserInfo:
            m_userInfoRequestInProgress = false;
            emit userInfoError("请求失败: " + error);
            break;
        case RequestType::UsageInfo:
            m_usageInfoRequestInProgress = false;
            emit usageInfoError("请求失败: " + error);
            break;
        default:
            // 未知请求类型的错误，不打印日志
            break;
    }
}

void CursorApi::parseUserInfoResponse(const QByteArray &data)
{
    QJsonParseError parseError;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        m_logManager->log("CursorApi: JSON解析失败 - " + parseError.errorString(), LogManager::Error);
        emit userInfoError("无效的JSON数据");
        return;
    }
    
    // 提取用户信息
    QJsonObject jsonObj = jsonDoc.object();
    CursorUserInfo userInfo;
    
    // 新的响应格式直接包含用户信息，不再包含在user字段中
    userInfo.email = jsonObj["email"].toString();
    userInfo.name = jsonObj["name"].toString();
    userInfo.id = jsonObj["sub"].toString();  // 使用sub作为用户ID
    userInfo.picture = jsonObj["picture"].toString();
    userInfo.emailVerified = jsonObj["email_verified"].toBool();
    userInfo.updatedAt = jsonObj["updated_at"].toString();
    
    emit userInfoUpdated(userInfo);
}

void CursorApi::parseUsageInfoResponse(const QByteArray &data)
{
    QJsonParseError parseError;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &parseError);
    
    if (parseError.error != QJsonParseError::NoError) {
        m_logManager->log("CursorApi: JSON解析失败 - " + parseError.errorString(), LogManager::Error);
        emit usageInfoError("无效的JSON数据");
        return;
    }
    
    QJsonObject jsonObj = jsonDoc.object();
    CursorUsageInfo usageInfo;
    
    // 解析GPT-4使用情况
    if (jsonObj.contains("gpt-4")) {
        QJsonObject gpt4 = jsonObj["gpt-4"].toObject();
        usageInfo.gpt4Requests = gpt4["numRequests"].toInt();
        usageInfo.gpt4TokensUsed = gpt4["numTokens"].toInt();
        usageInfo.gpt4MaxRequests = gpt4["maxRequestUsage"].toInt();
    }
    
    // 解析GPT-3.5使用情况
    if (jsonObj.contains("gpt-3.5-turbo")) {
        QJsonObject gpt35 = jsonObj["gpt-3.5-turbo"].toObject();
        usageInfo.gpt35Requests = gpt35["numRequests"].toInt();
        usageInfo.gpt35TokensUsed = gpt35["numTokens"].toInt();
        usageInfo.gpt35MaxRequests = gpt35["maxRequestUsage"].toInt();
    }
    
    // 解析GPT-4-32k使用情况
    if (jsonObj.contains("gpt-4-32k")) {
        QJsonObject gpt432k = jsonObj["gpt-4-32k"].toObject();
        usageInfo.gpt432kRequests = gpt432k["numRequests"].toInt();
        usageInfo.gpt432kTokensUsed = gpt432k["numTokens"].toInt();
        usageInfo.gpt432kMaxRequests = gpt432k["maxRequestUsage"].toInt();
    }
    
    // 获取计费周期开始时间
    usageInfo.startOfMonth = QDateTime::fromString(jsonObj["startOfMonth"].toString(), Qt::ISODate);
    
    emit usageInfoUpdated(usageInfo);
}

void CursorApi::switchAccount(const QString &accountId)
{
    // 待实现，不打印日志
}