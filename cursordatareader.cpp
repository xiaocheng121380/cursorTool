#include "cursordatareader.h"
#include <QFile>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>
#include <QStandardPaths>
#include <QThread>

CursorSessionData CursorDataReader::readSessionData() {
    CursorSessionData sessionData;
    
    // 获取AppData/Roaming路径
    QString userProfile = QDir::homePath();
    QString cursorDataPath = QDir::toNativeSeparators(userProfile + "/AppData/Roaming/Cursor/sentry/scope_v3.json");
    
    qDebug() << "Looking for session file at:" << cursorDataPath;
    
    QFile file(cursorDataPath);
    if (!file.exists()) {
        qDebug() << "Session file does not exist:" << cursorDataPath;
        return sessionData;
    }
    
    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "Could not open session file:" << cursorDataPath;
        return sessionData;
    }
    
    QByteArray jsonData = file.readAll();
    file.close();
    
    QJsonDocument document = QJsonDocument::fromJson(jsonData);
    if (document.isNull() || !document.isObject()) {
        qDebug() << "Invalid JSON in session file";
        return sessionData;
    }
    
    QJsonObject jsonObj = document.object();
    
    // 检查JSON格式
    if (jsonObj.contains("scope") && jsonObj["scope"].isObject()) {
        QJsonObject scopeObj = jsonObj["scope"].toObject();
        
        // 获取用户信息
        if (scopeObj.contains("user") && scopeObj["user"].isObject()) {
            QJsonObject userObj = scopeObj["user"].toObject();
            sessionData.email = userObj["email"].toString();
            sessionData.userId = userObj["id"].toString();
        }
    }
    
    // 获取版本信息
    if (jsonObj.contains("event") && jsonObj["event"].isObject()) {
        QJsonObject eventObj = jsonObj["event"].toObject();
        sessionData.release = eventObj["release"].toString();
    }
    
    qDebug() << "Extracted email:" << sessionData.email;
    qDebug() << "Extracted user ID:" << sessionData.userId;
    qDebug() << "Extracted release:" << sessionData.release;
    
    return sessionData;
} 