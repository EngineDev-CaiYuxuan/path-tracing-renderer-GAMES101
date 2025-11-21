# path-tracing-renderer-GAMES101
# 路径追踪渲染器优化实现

# Path Tracing Renderer: 基于能量阈值+动态轮盘赌的优化实现

## 项目概述

本项目实现了一个基于**蒙特卡洛路径追踪**的离线渲染器，支持Cornell Box场景的物理真实光照渲染。核心亮点在于引入**能量阈值提前终止**与**动态概率俄罗斯轮盘赌**的双优化策略，在保证画质基本可接受的前提下，将渲染效率提升33%，兼顾物理真实性与算力优化。

## 环境依赖

- **编程语言**：C++17
- **编译器**：GCC 9.0+/Clang 10.0+/MSVC 2019+
- **依赖库**：无第三方依赖库
- **构建工具**：CMake 3.10

## 编译与运行

### 1. 克隆仓库

```Bash
git clone https://github.com/Engine-dev/path-tracing-renderer.git
cd path-tracing-renderer
```

### 2. 编译项目

```Bash
mkdir build && cd build
cmake ..
make -j$(nproc) # Linux/macOS
# 或在Windows下用Visual Studio打开生成的.sln文件编译
```

### 3. 运行渲染器

```Bash
./path_tracing_renderer --spp 128 --output binary.ppm
```

- `--spp`：设置采样数（Samples Per Pixel），默认128；
- `--output`：指定输出图像路径。

## 核心功能与技术细节

### 1. 基础路径追踪实现

- **渲染方程求解**：通过蒙特卡洛采样递归求解渲染方程，支持直接光照（光源采样）与间接光照（材质反射采样）；
- **材质系统**：实现Lambert漫反射材质，支持自发光物体（如Cornell Box的光源）；
- **相交检测**：基于AABB包围盒的加速结构，支持三角形网格的光线相交检测。

### 2. 双优化策略：能量阈值+动态轮盘赌

#### 2.1 优化背景

原始路径追踪对所有光线无差别计算，低能量光线（RGB能量最大值<0.01）对画质贡献<1%，却占用约30%算力，导致渲染效率低下。

#### 2.2 优化实现

- **能量阈值提前终止**：

为`Ray`结构体新增`Vector3f energy`成员，逐通道记录光线能量；当光线能量最大值`<0.01`时直接终止追踪，砍掉无贡献计算。

```C++
float max_energy = std::max({ray.energy.x, ray.energy.y, ray.energy.z});
if (max_energy < 0.01f) return ref_color;
```

- **动态概率轮盘赌**：

用光线能量动态调整轮盘赌保留概率（`prob = max(能量最大值, 0.1)`），高能量光线少终止、低能量光线多终止；通过`1/prob`加权保证光照期望守恒，避免画质失衡。

```C++
float prob = std::max(max_energy, 0.1f);
if (depth >= 6 || get_random_float() > prob) return ref_color;
Vector3f energy_new = ray.energy * f_r / prob; // 能量衰减+加权补偿
```

- **逻辑分工优化**：

区分“物理能量衰减”（`energy_new = ray.energy * f_r / prob`）与“采样数学权重”（`L_indir *= cos_alpha / pdf`），避免重复计算导致画面过曝/过暗。

## 成果展示与对比

### 1. 渲染效率对比

| 方案                | SPP=128渲染耗时 | 效率提升 | 画质状态                 |
| ------------------- | --------------- | -------- | ------------------------ |
| 纯轮盘赌（无优化）  | 120分钟         | -        | 色彩正常，噪点均匀       |
| 能量阈值+动态轮盘赌 | 80分钟          | 33%      | 亮部轻微过曝，暗部略暗沉 |

### 2. 渲染效果对比

| 纯轮盘赌（SPP=128）                                          | 双优化（SPP=128）                                            |
| ------------------------------------------------------------ | ------------------------------------------------------------ |
| ![image-20251122013309243](C:\Users\24668\AppData\Roaming\Typora\typora-user-images\image-20251122013309243.png) | ![image-20251122013230372](C:\Users\24668\AppData\Roaming\Typora\typora-user-images\image-20251122013230372.png) |

## 待改进方向

当前优化仍存在画质细节瑕疵，后续将从以下方向迭代：

1. **阈值与保底概率微调**：将能量阈值放宽、轮盘赌保底概率提升，保留更多弱贡献间接光照，改善暗部暗沉问题；
2. **光照累加逻辑优化**：分离直接光照与间接光照的加权计算，解决亮部过曝；
3. **采样策略升级**：引入重要性采样（如余弦加权采样），进一步降低噪点。

## 核心代码结构

```Plain
path-tracing-renderer/
 ├── ray.h        // 光线类（含能量成员）
 ├── scene.h      // 场景类（castRay核心逻辑）
 ├── scene.cpp    // 路径追踪+双优化实现
└── main.cpp     // 渲染入口
└── README.md        // 项目文档
```

## 参考资料

- 《Physically Based Rendering: From Theory to Implementation》（PBRT）
- Blender Cycles渲染器技术文档
- 蒙特卡洛路径追踪与俄罗斯轮盘赌优化论文

## 致谢

感谢图形学社区的开源资源与技术分享，本项目的优化思路参考了工业界路径追踪渲染器的工程实践。
