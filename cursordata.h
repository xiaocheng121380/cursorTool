#ifndef CURSORDATA_H
#define CURSORDATA_H

#include <QString>
#include <QDateTime>
#include <QMap>

// Cursor机器信息结构体
struct CursorMachineInfo {
    QString machineId;               // 机器ID
    QString platform;                // 平台信息 (Windows, MacOS, Linux)
    QString version;                 // Cursor版本
    QDateTime lastSeen;              // 最后活跃时间
    bool isActive;                   // 是否处于活跃状态
    QString ipAddress;               // IP地址
    QString location;                // 地理位置
    QString deviceName;              // 设备名称
};

// Cursor会话数据结构体
struct CursorSessionData {
    QString email;                   // 用户邮箱 (scope.user.email)
    QString userId;                  // 用户ID (scope.user.id)
    QString release;                 // Cursor版本 (event.release)
};

#endif // CURSORDATA_H 