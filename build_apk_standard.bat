@echo off
chcp 65001 >nul
title MotoNav 安卓普通版一键打包
echo ======================================================================
echo   MotoNav 安卓普通版 (Standard Edition) 一键个人专属打包工具
echo ======================================================================
echo.
echo [说明] 本工具将自动检测/配置编译环境，并生成属于您个人的专属包名与签名证书。
echo [步骤] 打包完成后，您只需将生成的包名和 SHA1 填入高德开放平台免费申请 Key 即可。
echo.
cd /d "%~dp0"
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0Build-Personal.ps1"
if %ERRORLEVEL% NEQ 0 (
    echo.
    echo ======================================================================
    echo  [错误] 打包失败，请检查上方红字提示！
    echo ======================================================================
) else (
    echo.
    echo ======================================================================
    echo  [成功] 个人专属 APK 打包完成！
    echo  产物保存在 output/ 目录：
    echo  - output/MotoNav-personal.apk  (安装到手机的专属安装包)
    echo  - output/Application-info.txt  (申请高德 Key 所需的专属包名与 SHA1)
    echo ======================================================================
)
echo.
pause
