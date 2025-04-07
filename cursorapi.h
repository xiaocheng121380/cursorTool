#ifndef CURSORAPI_H
#define CURSORAPI_H

#include <QObject>
#include <QDateTime>
#include <QMap>

class LogManager;
class CurlHttpClient;

// 用户信息结构体
struct CursorUserInfo {
    QString id;          // sub字段
    QString name;        // name字段
    QString email;       // email字段
    QString picture;     // picture字段
    QString updatedAt;   // updated_at字段
    bool emailVerified;  // email_verified字段
};

// 使用统计结构体
struct CursorUsageInfo {
    // GPT-4使用情况
    int gpt4Requests = 0;        // numRequests
    int gpt4TokensUsed = 0;      // numTokens
    int gpt4MaxRequests = 0;     // maxRequestUsage
    
    // GPT-3.5使用情况
    int gpt35Requests = 0;       // numRequests
    int gpt35TokensUsed = 0;     // numTokens
    int gpt35MaxRequests = 0;    // maxRequestUsage
    
    // GPT-4-32k使用情况
    int gpt432kRequests = 0;     // numRequests
    int gpt432kTokensUsed = 0;   // numTokens
    int gpt432kMaxRequests = 0;  // maxRequestUsage
    
    // 计费周期
    QDateTime startOfMonth;      // startOfMonth字段
};

class CursorApi : public QObject
{
    Q_OBJECT
public:
    explicit CursorApi(QObject *parent = nullptr);
    ~CursorApi();
    
    // 设置认证令牌
    void setAuthToken(const QString &token);
    QString authToken() const;
    
    // API请求方法
    void fetchUserInfo();
    void fetchUsageInfo();
    void switchAccount(const QString &accountId);
    void deleteAccount();
    
    // URL处理辅助方法
    QString buildApiUrl(const QString &endpoint);
    
signals:
    // 用户信息相关信号
    void userInfoUpdated(const CursorUserInfo &userInfo);
    void userInfoError(const QString &errorMessage);
    
    // 使用统计相关信号
    void usageInfoUpdated(const CursorUsageInfo &usageInfo);
    void usageInfoError(const QString &errorMessage);
    
    // 删除账户相关信号
    void accountDeleted();
    void accountDeleteError(const QString &errorMessage);
    
private:
    // 请求类型枚举
    enum class RequestType {
        UserInfo,
        UsageInfo,
        DeleteAccount
    };
    
    // 成员变量
    LogManager *m_logManager;
    CurlHttpClient *m_httpClient;
    QString m_authToken;
    bool m_userInfoRequestInProgress;
    bool m_usageInfoRequestInProgress;
    bool m_deleteAccountRequestInProgress;
    RequestType m_currentRequestType;
    
    // 响应解析方法
    void parseUserInfoResponse(const QByteArray &data);
    void parseUsageInfoResponse(const QByteArray &data);
    
    // 响应处理方法
    void handleResponse(const QByteArray &data);
    void handleError(const QString &error);
};

#endif // CURSORAPI_H 