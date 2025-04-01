#!/bin/bash

# 创建图标集目录
mkdir -p cursor_logo.iconset

# 使用 sips 命令生成不同尺寸的图标
sips -z 16 16     cursor_logo.png --out cursor_logo.iconset/icon_16x16.png
sips -z 32 32     cursor_logo.png --out cursor_logo.iconset/icon_16x16@2x.png
sips -z 32 32     cursor_logo.png --out cursor_logo.iconset/icon_32x32.png
sips -z 64 64     cursor_logo.png --out cursor_logo.iconset/icon_32x32@2x.png
sips -z 128 128   cursor_logo.png --out cursor_logo.iconset/icon_128x128.png
sips -z 256 256   cursor_logo.png --out cursor_logo.iconset/icon_128x128@2x.png
sips -z 256 256   cursor_logo.png --out cursor_logo.iconset/icon_256x256.png
sips -z 512 512   cursor_logo.png --out cursor_logo.iconset/icon_256x256@2x.png
sips -z 512 512   cursor_logo.png --out cursor_logo.iconset/icon_512x512.png
sips -z 1024 1024 cursor_logo.png --out cursor_logo.iconset/icon_512x512@2x.png

# 使用 iconutil 创建 icns 文件
iconutil -c icns cursor_logo.iconset

# 清理临时文件
rm -rf cursor_logo.iconset 