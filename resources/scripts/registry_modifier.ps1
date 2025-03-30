# Registry Modifier for Cursor Reset Tool
# Set output encoding to UTF-8
$OutputEncoding = [System.Text.Encoding]::UTF8
[Console]::OutputEncoding = [System.Text.Encoding]::UTF8

# Variables will be dynamically set by C++
# $Action - Action type: backup, modify, backup_and_modify
# $BackupPath - Backup file path
# $NewGuid - New GUID value

# Result class
class OperationResult {
    [bool]$Success
    [string]$Message
    [string]$CurrentValue
    [string]$PreviousValue
    [string]$NewValue
    [bool]$BackupSuccess
    [bool]$ModifySuccess
    [string]$BackupMessage
    [string]$ModifyMessage
}

# Special JSON conversion using BASE64 for Unicode
function ConvertTo-Utf8Json {
    param (
        [Parameter(Mandatory=$true, ValueFromPipeline=$true)]
        [object]$InputObject
    )
    
    # Convert to regular JSON first
    $jsonString = $InputObject | ConvertTo-Json -Depth 10 -Compress
    
    # Create a special wrapper object
    $wrapper = @{
        "raw" = $jsonString
        "base64" = [Convert]::ToBase64String([System.Text.Encoding]::UTF8.GetBytes($jsonString))
    }
    
    # Output the wrapper's JSON
    return ($wrapper | ConvertTo-Json -Compress)
}

# Registry backup function
function Backup-Registry {
    param (
        [string]$BackupPath
    )
    
    $result = [OperationResult]::new()
    
    # Check registry path
    $registryPath = "HKLM:\SOFTWARE\Microsoft\Cryptography"
    if (-not (Test-Path $registryPath)) {
        $result.Success = $false
        $result.Message = "Registry path does not exist: $registryPath"
        return $result
    }
    
    try {
        # Get current value
        $currentValue = (Get-ItemProperty -Path $registryPath -Name MachineGuid -ErrorAction SilentlyContinue).MachineGuid
        $result.CurrentValue = $currentValue
        
        # Create backup file
        $backupResult = Start-Process "reg.exe" -ArgumentList "export", "`"HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Cryptography`"", "`"$BackupPath`"" -NoNewWindow -Wait -PassThru
        
        if ($backupResult.ExitCode -eq 0) {
            $result.Success = $true
            $result.Message = "Registry key successfully backed up to: $BackupPath"
        } else {
            $result.Success = $false
            $result.Message = "Backup creation failed with exit code: $($backupResult.ExitCode)"
        }
    } catch {
        $result.Success = $false
        $result.Message = "Error during backup: $($_.Exception.Message)"
    }
    
    return $result
}

# Registry modification function
function Modify-Registry {
    param (
        [string]$NewGuid
    )
    
    $result = [OperationResult]::new()
    
    # Check registry path
    $registryPath = "HKLM:\SOFTWARE\Microsoft\Cryptography"
    if (-not (Test-Path $registryPath)) {
        try {
            New-Item -Path $registryPath -Force | Out-Null
        } catch {
            $result.Success = $false
            $result.Message = "Failed to create registry path: $($_.Exception.Message)"
            return $result
        }
    }
    
    try {
        # Get current value
        $previousValue = (Get-ItemProperty -Path $registryPath -Name MachineGuid -ErrorAction SilentlyContinue).MachineGuid
        $result.PreviousValue = $previousValue
        
        # Generate new GUID if not provided
        if (-not $NewGuid) {
            $NewGuid = [System.Guid]::NewGuid().ToString()
        }
        
        # Update registry value
        Set-ItemProperty -Path $registryPath -Name MachineGuid -Value $NewGuid -Force -ErrorAction Stop
        
        # Verify update
        $updatedValue = (Get-ItemProperty -Path $registryPath -Name MachineGuid).MachineGuid
        if ($updatedValue -eq $NewGuid) {
            $result.Success = $true
            $result.Message = "MachineGuid successfully updated to: $updatedValue"
        } else {
            $result.Success = $false
            $result.Message = "MachineGuid update verification failed"
        }
        
        $result.NewValue = $updatedValue
    } catch {
        $result.Success = $false
        $result.Message = "Error during modification: $($_.Exception.Message)"
    }
    
    return $result
}

# Backup and modify registry
function Backup-And-Modify-Registry {
    param (
        [string]$BackupPath,
        [string]$NewGuid
    )
    
    $result = [OperationResult]::new()
    
    # Execute backup
    $backupResult = Backup-Registry -BackupPath $BackupPath
    $result.BackupSuccess = $backupResult.Success
    $result.BackupMessage = $backupResult.Message
    $result.PreviousValue = $backupResult.CurrentValue
    
    # Execute modification
    $modifyResult = Modify-Registry -NewGuid $NewGuid
    $result.ModifySuccess = $modifyResult.Success
    $result.ModifyMessage = $modifyResult.Message
    $result.NewValue = $modifyResult.NewValue
    
    # Set overall result
    $result.Success = $modifyResult.Success
    $result.Message = "Backup: $($backupResult.Message) | Modify: $($modifyResult.Message)"
    
    return $result
}

# Note: Main execution logic will be dynamically generated and appended to the end of the script 