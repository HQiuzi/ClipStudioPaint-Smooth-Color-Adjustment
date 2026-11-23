# CSP 流畅调色：Windows x64 安装包

构建来源：`origin/master` 的 `b88edd74d5067eef5cf4636d39e69bd9bafb95f6`，包含“应用后期层”稳定时序恢复与图层组收起状态的安全性能优化。

修订 r1：已修复安装包中嵌入式界面资源的目录层级；优动漫 PAINT 3.0.7 已验证“帮助 → 遇到问题”可以正常打开插件菜单。

修订 r2：恢复优动漫对“应用后期层”所需的逐层合并调用顺序，避免“操作未能完整执行”；对于已经收起的图层组不再重复写入收起状态，减少无意义的宿主刷新。

## 安装（CLIP STUDIO PAINT / 优动漫 PAINT）

1. 完全退出绘图软件；可在任务管理器确认没有 `CLIPStudioPaint.exe`、`UDMPaintPRO.exe` 或 `UDMPaintEX.exe` 残留进程。
2. 将此压缩包**全部解压到 PAINT 程序所在目录**，保留压缩包内的目录结构。例如优动漫默认目录为 `C:\Program Files (x86)\Unicorn\UDMPaint\`，其中应有 `UDMPaintPRO.exe` 或 `UDMPaintEX.exe`。
3. 在该目录运行 `CSPPatchTool.exe` 一次。成功后会生成 `CSPAddrTable.json`；若提示无写入权限，请右键选择“以管理员身份运行”。
4. 启动 PAINT，在“帮助 → 遇到问题”（或命令栏入口）打开本插件菜单。

压缩包内的 `PlugIn\PAINT\` 已包含 `AAload.dll` 及全部 `SmoothFilter_*.dll`，因此只需整体解压，无需分别移动滤镜。

## 卸载

退出软件后删除程序根目录中的 `CSPMOD.dll`、`CSPPatchTool.exe`、`CSPAddrTable.json`，以及 `PlugIn\PAINT\` 中的 `AAload.dll` 和 `SmoothFilter_*.dll`。不要删除官方原有的 `PlugIn\PAINT\` 文件夹。

此包仅适用于 64 位 Windows。首次用于不同版本的 CSP / 优动漫时，请先在副本文件上验证各功能。
