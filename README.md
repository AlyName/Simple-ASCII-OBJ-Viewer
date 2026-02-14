# Simple ASCII OBJ Viewer

![example](example.gif)

简单的 ASCII 3D Viewer，无需 OpenGL 等依赖库。

目前仅支持 `.obj` 格式的 Mesh。允许带 `.mtl` 的材质描述文件。支持旋转（A / D）和缩放（W / S）。

## Build

```bash
mkdir build
cd build
cmake .. -G "Ninja" -DCMAKE_BUILD_TYPE=Release
cmake --build .
```

## Run

```bash
# 指定 OBJ 文件
./viewer.exe ../examples/rose.obj

# 或运行后输入路径
./viewer.exe
```
