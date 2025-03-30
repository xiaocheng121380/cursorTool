#include "powershellrunner.h"
#include <QDebug>
#include <QFile>
#include <QTextStream>
#include <QCoreApplication>
#include <QTemporaryFile>
#include <QDir>

PowerShellRunner::PowerShellRunner(QObject *parent) : QObject(parent),
    m_process(new QProcess(this)),
    m_scriptContent("")
{
    // 连接信号槽
    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &PowerShellRunner::processFinished);
    connect(m_process, &QProcess::readyReadStandardOutput, this, &PowerShellRunner::readProcessOutput);
    connect(m_process, &QProcess::readyReadStandardError, this, &PowerShellRunner::readProcessError);
    
    // 从资源中提取脚本
    if (!loadScriptFromResources()) {
        qDebug() << "无法从资源中加载PowerShell脚本";
    }
}

PowerShellRunner::~PowerShellRunner()
{
    if (m_process->state() != QProcess::NotRunning) {
        m_process->kill();
        m_process->waitForFinished();
    }
    
    // 删除临时脚本文件(如果存在)
    if (!m_tempScriptPath.isEmpty() && QFile::exists(m_tempScriptPath)) {
        QFile::remove(m_tempScriptPath);
    }
}

void PowerShellRunner::backupRegistry(const QString &backupPath)
{
    // 每次操作前重新加载脚本
    if (m_scriptContent.isEmpty() && !loadScriptFromResources()) {
        emit operationCompleted(false, "无法加载脚本内容");
        return;
    }
    
    QStringList arguments;
    arguments << "-ExecutionPolicy" << "Bypass"
              << "-File" << createCommandString("backup", backupPath, "");
              
    runScript(arguments);
}

void PowerShellRunner::modifyRegistry(const QString &newGuid)
{
    // 每次操作前重新加载脚本
    if (m_scriptContent.isEmpty() && !loadScriptFromResources()) {
        emit operationCompleted(false, "无法加载脚本内容");
        return;
    }
    
    QStringList arguments;
    arguments << "-ExecutionPolicy" << "Bypass"
              << "-File" << createCommandString("modify", "", newGuid);
              
    runScript(arguments);
}

void PowerShellRunner::backupAndModifyRegistry(const QString &backupPath, const QString &newGuid)
{
    // 每次操作前重新加载脚本
    if (m_scriptContent.isEmpty() && !loadScriptFromResources()) {
        emit operationCompleted(false, "无法加载脚本内容");
        return;
    }
    
    QStringList arguments;
    arguments << "-ExecutionPolicy" << "Bypass"
              << "-File" << createCommandString("backup_and_modify", backupPath, newGuid);
              
    runScript(arguments);
}

bool PowerShellRunner::loadScriptFromResources()
{
    // 从资源中读取脚本内容
    QFile resourceFile(":/scripts/scripts/registry_modifier.ps1");
    if (!resourceFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "无法从资源中读取脚本:" << resourceFile.errorString();
        qDebug() << "当前资源路径:" << QDir::currentPath();
        qDebug() << "尝试在以下路径中搜索脚本:";
        qDebug() << "  - :/scripts/scripts/registry_modifier.ps1";
        qDebug() << "  - 资源前缀:" << QDir(":/").entryList();
        
        // 尝试列出脚本目录
        if (QDir(":/scripts").exists()) {
            qDebug() << "  - 脚本目录内容:" << QDir(":/scripts").entryList();
            
            if (QDir(":/scripts/scripts").exists()) {
                qDebug() << "  - 内部脚本目录内容:" << QDir(":/scripts/scripts").entryList();
            }
        }
        
        return false;
    }
    
    // 读取脚本内容
    QByteArray scriptData = resourceFile.readAll();
    resourceFile.close();
    
    // 保存脚本内容
    m_scriptContent = QString::fromUtf8(scriptData);
    
    // 检查并确保脚本文本格式正确
    if (m_scriptContent.contains("\r\n")) {
        // 已经是Windows换行格式
    } else {
        // 转换为Windows换行格式
        m_scriptContent.replace("\n", "\r\n");
    }
    
    qDebug() << "PowerShell脚本已加载，大小:" << m_scriptContent.length() << "字符";
    return true;
}

QString PowerShellRunner::createCommandString(const QString &action, const QString &backupPath, const QString &newGuid)
{
    // 清理之前的临时文件
    if (!m_tempScriptPath.isEmpty() && QFile::exists(m_tempScriptPath)) {
        QFile::remove(m_tempScriptPath);
    }
    
    // 创建临时文件夹（如果不存在）
    QDir tempDir(QDir::tempPath());
    QString tempFolderName = "cursorTool";
    if (!tempDir.exists(tempFolderName)) {
        tempDir.mkdir(tempFolderName);
    }
    
    // 创建临时脚本文件
    QTemporaryFile tempFile(QDir::tempPath() + "/" + tempFolderName + "/script_XXXXXX.ps1");
    tempFile.setAutoRemove(false); // 我们会手动删除
    
    if (!tempFile.open()) {
        qDebug() << "无法创建临时脚本文件:" << tempFile.errorString();
        return QString();
    }
    
    // 写入完整脚本内容，包括附加的调用代码
    QString finalScript = m_scriptContent;
    
    // 处理路径中的反斜杠
    QString escapedPath = QString(backupPath);
    escapedPath.replace("\\", "\\\\");
    
    // 添加参数定义和函数调用代码
    QString callCode = QString("\r\n\r\n"
                               "# 下面是C++添加的参数和调用代码\r\n"
                               "$Action = \"%1\"\r\n"
                               "$BackupPath = \"%2\"\r\n"
                               "$NewGuid = \"%3\"\r\n\r\n"
                               "# 根据操作类型执行对应的函数\r\n"
                               "try {\r\n"
                               "    $result = $null\r\n"
                               "    \r\n"
                               "    switch ($Action) {\r\n"
                               "        \"backup\" {\r\n"
                               "            if (-not $BackupPath) {\r\n"
                               "                throw \"Action 'backup' requires BackupPath parameter\"\r\n"
                               "            }\r\n"
                               "            $result = Backup-Registry -BackupPath $BackupPath\r\n"
                               "        }\r\n"
                               "        \"modify\" {\r\n"
                               "            $result = Modify-Registry -NewGuid $NewGuid\r\n"
                               "        }\r\n"
                               "        \"backup_and_modify\" {\r\n"
                               "            if (-not $BackupPath) {\r\n"
                               "                throw \"Action 'backup_and_modify' requires BackupPath parameter\"\r\n"
                               "            }\r\n"
                               "            $result = Backup-And-Modify-Registry -BackupPath $BackupPath -NewGuid $NewGuid\r\n"
                               "        }\r\n"
                               "        default {\r\n"
                               "            throw \"Invalid Action: $Action. Valid actions are: backup, modify, backup_and_modify\"\r\n"
                               "        }\r\n"
                               "    }\r\n"
                               "    \r\n"
                               "    # 返回JSON结果，使用UTF8-Base64编码\r\n"
                               "    $jsonResult = $result | ConvertTo-Utf8Json\r\n"
                               "    Write-Output $jsonResult\r\n"
                               "    \r\n"
                               "} catch {\r\n"
                               "    $errorResult = [OperationResult]::new()\r\n"
                               "    $errorResult.Success = $false\r\n"
                               "    $errorResult.Message = \"Error: $($_.Exception.Message)\"\r\n"
                               "    \r\n"
                               "    $jsonError = $errorResult | ConvertTo-Utf8Json\r\n"
                               "    Write-Output $jsonError\r\n"
                               "    exit 1\r\n"
                               "}")
                        .arg(action)  // 使用双引号，允许PowerShell变量替换
                        .arg(escapedPath)
                        .arg(newGuid);
    
    // 找到脚本末尾的主执行逻辑部分并替换，或者直接附加到脚本末尾
    int mainLogicStart = finalScript.indexOf("# 主执行逻辑");
    if (mainLogicStart > 0) {
        // 如果找到了主执行逻辑段落，则将其替换
        finalScript = finalScript.left(mainLogicStart) + callCode;
    } else {
        // 否则，直接附加到脚本末尾
        finalScript += callCode;
    }
    
    // 写入临时文件
    QTextStream stream(&tempFile);
    stream.setCodec("UTF-8");
    stream << finalScript;
    tempFile.close();
    
    // 保存临时文件路径
    m_tempScriptPath = QDir::toNativeSeparators(tempFile.fileName());
    
    qDebug() << "临时脚本文件已创建:" << m_tempScriptPath;
    return m_tempScriptPath;
}

bool PowerShellRunner::runScript(const QStringList &arguments)
{
    if (m_process->state() != QProcess::NotRunning) {
        qDebug() << "进程已在运行中";
        return false;
    }
    
    qDebug() << "执行PowerShell命令:" << arguments.join(" ");
    
    // 设置进程环境变量，确保正确处理UTF-8
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert("PYTHONIOENCODING", "utf-8");
    env.insert("POWERSHELL_TELEMETRY_OPTOUT", "1");
    m_process->setProcessEnvironment(env);
    
    // 设置输出编码
    m_process->setProcessChannelMode(QProcess::SeparateChannels);
    m_process->setReadChannel(QProcess::StandardOutput);
    
    m_process->start("powershell", arguments);
    
    if (!m_process->waitForStarted()) {
        qDebug() << "启动PowerShell失败:" << m_process->errorString();
        emit operationCompleted(false, "启动PowerShell失败: " + m_process->errorString());
        return false;
    }
    
    return true;
}

void PowerShellRunner::processFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    qDebug() << "PowerShell进程结束，退出代码:" << exitCode;
    
    if (exitStatus == QProcess::CrashExit) {
        emit operationCompleted(false, "PowerShell进程异常终止");
        return;
    }
    
    if (exitCode != 0) {
        emit operationCompleted(false, "PowerShell进程返回错误代码: " + QString::number(exitCode));
        return;
    }
    
    // 读取可能剩余的输出
    readProcessOutput();
    
    // 处理脚本操作的成功情况在readProcessOutput中完成
}

void PowerShellRunner::readProcessOutput()
{
    QString output = QString::fromUtf8(m_process->readAllStandardOutput());
    if (output.isEmpty()) {
        return;
    }
    
    qDebug() << "PowerShell输出:" << output;
    emit scriptOutput(output);
    
    // 尝试解析JSON结果
    int jsonStartPos = output.indexOf('{');
    int jsonEndPos = output.lastIndexOf('}');
    
    if (jsonStartPos >= 0 && jsonEndPos > jsonStartPos) {
        QString jsonStr = output.mid(jsonStartPos, jsonEndPos - jsonStartPos + 1);
        QJsonDocument wrapperDoc = QJsonDocument::fromJson(jsonStr.toUtf8());
        
        if (!wrapperDoc.isNull() && wrapperDoc.isObject()) {
            QJsonObject wrapper = wrapperDoc.object();
            
            // 尝试获取base64编码的JSON
            if (wrapper.contains("base64")) {
                QString base64Json = wrapper["base64"].toString();
                QByteArray jsonData = QByteArray::fromBase64(base64Json.toLatin1());
                QString decodedJson = QString::fromUtf8(jsonData);
                
                // 解析解码后的JSON
                QJsonDocument jsonDoc = QJsonDocument::fromJson(decodedJson.toUtf8());
                if (!jsonDoc.isNull() && jsonDoc.isObject()) {
                    QJsonObject result = jsonDoc.object();
                    
                    // 根据不同操作类型处理结果
                    if (result.contains("CurrentValue") && result.contains("Success")) {
                        // 备份操作结果
                        bool success = result["Success"].toBool();
                        QString currentGuid = result["CurrentValue"].toString();
                        QString message = result["Message"].toString();
                        
                        emit backupCompleted(success, "内存中执行", currentGuid);
                        emit operationCompleted(success, message);
                    }
                    else if (result.contains("PreviousValue") && result.contains("NewValue")) {
                        // 修改操作结果
                        bool success = result["Success"].toBool();
                        QString previousGuid = result["PreviousValue"].toString();
                        QString newGuid = result["NewValue"].toString();
                        QString message = result["Message"].toString();
                        
                        emit modifyCompleted(success, newGuid, previousGuid);
                        emit operationCompleted(success, message);
                    }
                    else if (result.contains("BackupSuccess") && result.contains("ModifySuccess")) {
                        // 备份并修改操作结果
                        bool backupSuccess = result["BackupSuccess"].toBool();
                        bool modifySuccess = result["ModifySuccess"].toBool();
                        QString previousGuid = result["PreviousValue"].toString();
                        QString newGuid = result["NewValue"].toString();
                        QString backupMessage = result["BackupMessage"].toString();
                        QString modifyMessage = result["ModifyMessage"].toString();
                        
                        emit backupCompleted(backupSuccess, "内存中执行", previousGuid);
                        emit modifyCompleted(modifySuccess, newGuid, previousGuid);
                        emit operationCompleted(modifySuccess, result["Message"].toString());
                    }
                }
            } else {
                // 兼容旧方式，直接解析原始JSON
                QJsonObject result;
                if (wrapper.contains("raw")) {
                    QJsonDocument rawDoc = QJsonDocument::fromJson(wrapper["raw"].toString().toUtf8());
                    if (!rawDoc.isNull() && rawDoc.isObject()) {
                        result = rawDoc.object();
                    }
                } else {
                    // 直接使用wrapper作为结果
                    result = wrapper;
                }
                
                // 根据不同操作类型处理结果
                if (result.contains("CurrentValue") && result.contains("Success")) {
                    // 备份操作结果
                    bool success = result["Success"].toBool();
                    QString currentGuid = result["CurrentValue"].toString();
                    QString message = result["Message"].toString();
                    
                    emit backupCompleted(success, "内存中执行", currentGuid);
                    emit operationCompleted(success, message);
                }
                else if (result.contains("PreviousValue") && result.contains("NewValue")) {
                    // 修改操作结果
                    bool success = result["Success"].toBool();
                    QString previousGuid = result["PreviousValue"].toString();
                    QString newGuid = result["NewValue"].toString();
                    QString message = result["Message"].toString();
                    
                    emit modifyCompleted(success, newGuid, previousGuid);
                    emit operationCompleted(success, message);
                }
                else if (result.contains("BackupSuccess") && result.contains("ModifySuccess")) {
                    // 备份并修改操作结果
                    bool backupSuccess = result["BackupSuccess"].toBool();
                    bool modifySuccess = result["ModifySuccess"].toBool();
                    QString previousGuid = result["PreviousValue"].toString();
                    QString newGuid = result["NewValue"].toString();
                    QString backupMessage = result["BackupMessage"].toString();
                    QString modifyMessage = result["ModifyMessage"].toString();
                    
                    emit backupCompleted(backupSuccess, "内存中执行", previousGuid);
                    emit modifyCompleted(modifySuccess, newGuid, previousGuid);
                    emit operationCompleted(modifySuccess, result["Message"].toString());
                }
            }
        }
    }
}

void PowerShellRunner::readProcessError()
{
    QString error = QString::fromUtf8(m_process->readAllStandardError());
    if (error.isEmpty()) {
        return;
    }
    
    qDebug() << "PowerShell错误:" << error;
    emit scriptError(error);
} 