# Simple ASCII OBJ Viewer

简单的 ASCII 3D Viewer，无需 OpenGL 等依赖库。

## 构建

```bash
mkdir build
cd build
cmake .. -G "Ninja" -DCMAKE_BUILD_TYPE=Release
cmake --build .
```

## 运行

```bash
# 指定 OBJ 文件
.\viewer.exe cube.obj

# 或运行后输入路径
.\viewer.exe
```
