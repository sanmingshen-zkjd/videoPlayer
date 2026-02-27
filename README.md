# Qt/C++ 播放器界面示例

本项目是一个基于 **Qt6 + C++** 的播放器界面示例，覆盖了以下功能：

1. 导入图片序列和常见视频文件（mp4/mov/avi/mkv 等）
2. 播放/暂停/停止
3. 播放窗口控制（放大、缩小、鼠标拖拽平移、滚轮缩放）
4. 播放控制（快进 5s、快退 5s、1x 正常速度）
5. 在窗口上绘制点和直线

## 构建

```bash
cmake -S . -B build
cmake --build build -j
```

> 需要安装 Qt6（Widgets、Multimedia、MultimediaWidgets）。

## 使用

- 点击工具栏 `Import` 导入媒体文件。
- 多选且全部为图片时，会按图片序列加载。
- 使用 `Playback` 工具栏进行播放控制。
- 使用 `View` 工具栏进行窗口缩放与复位。
- 使用 `Draw` 工具栏切换绘制模式：
  - `Draw Point`：左键点击添加点
  - `Draw Line`：左键两次点击确定一条线段
  - `Draw None`：退出绘制模式
