# Windows Screen Capture -> H.264 (x264) -> RTMP Example

这是一个示例项目，演示如何在 Windows 下使用 Desktop Duplication 捕获屏幕，使用 `libx264` 编码为 H.264，并通过 `librtmp` 推送到 RTMP 服务器。主程序使用 C，屏幕捕获使用 C++（Desktop Duplication API），并暴露 C 接口以便与主逻辑结合。

重要说明
- 该示例为教学/骨架代码，某些细节（如完整 FLV/AVC sequence header 的发送、错误处理、性能优化、线程与同步）需要在生产环境中完善。
- Desktop Duplication 要求 Windows 8 及以上。
- 建议在 MSYS2/MinGW64 下编译，或者用 Visual Studio 对 capture.cpp 做适配。

依赖（MSYS2 / mingw-w64）
- mingw-w64 toolchain
- x264 (libx264)
- librtmp (librtmp)
- Windows SDK (包含 d3d11, dxgi 等)

示例构建（在 MSYS2 MinGW64 Shell 下）
```bash
pacman -Syu
pacman -S mingw-w64-x86_64-toolchain mingw-w64-x86_64-x264 mingw-w64-x86_64-librtmp
make
```

运行
```bash
./capture_streamer.exe rtmp://your.rtmp.server/app/streamkey
```

注意事项与改进方向
- 需要在推流前发送 AVC sequence header（SPS/PPS）。当前示例把编码器输出的 nal 直接作为 FLV 视频体发送，这是不完整的。
- 建议将捕获、编码、推流分离到不同线程并使用队列以降低丢帧和延迟。
- 可选使用 Media Foundation 或硬件编码器（NVENC、Intel QSV）获得更好性能。
# Hello