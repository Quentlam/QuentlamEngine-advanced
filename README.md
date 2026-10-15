<div align="center">

<img src="https://raw.githubusercontent.com/github/explore/80688e429a7d4ef2fca1e82350fe8e3517d3494d/topics/cpp/cpp.png" width="100" height="100" alt="C++ logo"/>

# 🌌 Quentlam Engine
**次世代 2D/3D 高性能游戏引擎架构**

[![C++20](https://img.shields.io/badge/C++-20-00599C.svg?style=for-the-badge&logo=c%2B%2B)](https://isocpp.org/)
[![OpenGL](https://img.shields.io/badge/OpenGL-4.5+-5586A4.svg?style=for-the-badge&logo=opengl)](https://www.opengl.org/)
[![Platform](https://img.shields.io/badge/Platform-Windows-lightgrey.svg?style=for-the-badge&logo=windows)]()
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg?style=for-the-badge)](https://opensource.org/licenses/MIT)

> *“不仅仅是渲染器，而是一个为你提供完整开发生态的底层架构。”*

**QuentlamEngine** 是一个基于 **C++20** 完全从零打造的现代化游戏引擎。它融合了数据驱动的 **ECS (实体组件系统)** 设计理念，集成了行业标准的 **双物理引擎 (Box2D & JoltPhysics)**，并提供了一个比肩商业级引擎（如 Unreal Engine）工作流的**原生可视化编辑器**。

[**✨ 核心特性**](#-核心特性) • [**🚀 快速开始**](#-快速开始) • [**🏗 架构解析**](#-架构与模块) • [**💻 代码示例**](#-代码示例)

---
</div>

## ✨ 核心特性 (Core Features)

### 🎮 极致性能与 ECS 架构
- **Entity-Component-System (ECS)**: 底层由 `EnTT` 驱动，带来极高的缓存命中率与数据导向设计（Data-Oriented Design），突破面向对象（OOP）的性能瓶颈。
- **数据驱动场景 (Data-Driven Scenes)**: 告别硬编码。场景树、实体和组件参数均支持一键序列化/反序列化至 JSON 文件，实现所见即所得。
- **无缝测试流**: 编辑器内一键进入 `Play` / `Pause` / `Stop` 状态，彻底分离开发（Edit）与运行时（Runtime）环境。

### 🎨 现代双轨渲染系统
- **图形 API 抽象层**: 高度抽象的底层接口（VertexArray, Shader, Framebuffer 等），目前后端由 OpenGL 强力驱动，为未来接入 Vulkan / DirectX 12 预留了完美空间。
- **Renderer 2D**: 专为 2D 游戏设计的超高性能批处理（Batch Rendering）渲染器，支持精灵（Sprite）、四边形和多边形。
- **Renderer 3D**: 支持复杂 3D 场景的网格渲染，原生接入 `Assimp` 模型加载，包含环境光、漫反射、高光（Phong 模型）等材质光照系统。
- **后期处理特效**: 基于帧缓冲（Framebuffer）的后处理管线。内置 **Sobel 边缘检测算法**，实现编辑器下炫酷的实体选中发光（Outline）特效。
- **粒子系统**: 基于对象池（Object Pool）的高效 2D 粒子发射器，支持颜色渐变、缩放与随机速度向量。

### 📐 行业级双重物理引擎
- **2D 物理系统**: 深度集成 `Box2D`。无论你是做平台跳跃还是俯视角射击，它都能提供精准的刚体（Rigidbody）、碰撞检测与重力模拟。
- **3D 物理系统**: 原生接入 `JoltPhysics`（地平线：西之绝境 同款物理引擎），为你未来的次世代 3D 物理交互保驾护航。

### 🛠 强大的可视化编辑器 (QL-Editor)
一个由 **Dear ImGui** 打造的深色极客风原生编辑器：
- **场景层级与属性面板**: 实时浏览实体树，动态添加/删除组件（Transform, Rigidbody, Colliders...）。
- **Gizmo 交互**: 整合 `ImGuizmo`，提供直观的移动、旋转、缩放操作轴。
- **智能资源浏览器 (Content Browser)**: 拖拽即可加载 `.fbx` 模型、`.png` 纹理以及 `.scene` 预设，彻底解放生产力。

---

## 🏗 架构与模块 (Architecture)

整个引擎采用高度解耦的模块化设计，并通过 `premake5` 自动化构建：

| 模块名称 | 描述说明 |
| :--- | :--- |
| ⚙️ **QuentlamEngine** | 引擎的绝对核心（静态库）。包含事件分发、时间步长、渲染管线、物理系统及基础工具链。 |
| 🛠 **QL-Editor** | 引擎的可视化操作台。提供 UI、Gizmo 交互以及场景文件的序列化管理。 |
| 🏃 **ParkourGame** | 作为技术验证的标杆项目。这是一个基于本引擎开发的 **数据驱动型 2D 跑酷游戏**，展示了引擎的实际落地能力。 |
| 🧪 **Sandbox** | 独立于编辑器的沙盒环境，供开发者在此快速编写 C++ 脚本测试新特性与渲染 API。 |

---

## 📦 核心技术栈与依赖

所有的外部依赖均以源码或静态库的形式安全地集成在 `vendor/` 目录下，无需繁琐的环境配置：
- **[GLFW](https://www.glfw.org/)**: 跨平台的窗口与输入管理。
- **[Glad](https://glad.dav1d.de/)**: 现代 OpenGL 函数加载器。
- **[GLM](https://glm.g-truc.net/)**: 专为图形编程设计的数学库（矩阵/向量计算）。
- **[Dear ImGui](https://github.com/ocornut/imgui)**: 极其高效的即时渲染（Immediate Mode）UI 框架。
- **[EnTT](https://github.com/skypjack/entt)**: C++ 业界最快的 ECS 库。
- **[Box2D](https://box2d.org/) & [JoltPhysics](https://github.com/jrouwe/JoltPhysics)**: 物理世界基石。
- **[Assimp](https://github.com/assimp/assimp)**: 强大的 3D 模型导入工具。
- **[spdlog](https://github.com/gabime/spdlog)**: 极速、轻量级的 C++ 日志系统。

---

## 🚀 快速开始 (Quick Start)

### 💻 准备工作
- **操作系统**: Windows 10 / 11
- **开发环境**: Visual Studio 2022
- **编译器要求**: 必须支持 **C++20** 标准

### 🛠️ 编译与运行
只需 3 步，即可点亮你的引擎世界：

1. **克隆代码库**:
   ```bash
   git clone https://github.com/your-username/QuentlamEngine.git
   cd QuentlamEngine
   ```

2. **一键生成工程文件**:
   运行自动化构建脚本，它会准备第三方依赖（下载 `premake5`、用 CMake 编译 `assimp`）并生成完整的工程文件：
   ```cmd
   .\script\GenerateProject.bat
   ```

   Linux 下使用同一套逻辑的 shell 入口（Windows 上签出的文件可能不带执行位，用 `bash` 调用最稳妥）：
   ```bash
   bash script/bootstrap.sh
   ```

3. **编译并起飞**:
   - 在 Visual Studio 中打开生成的 `QuentlamEngine.sln`。
   - 将 **`QL-Editor`** 或 **`ParkourGame`** 设为**启动项目 (Startup Project)**。
   - 切换构建配置为 `Debug` 或 `Release` (x64架构)。
   - 按下 `F5`，见证奇迹的时刻。

---

## 💻 代码示例 (Code Example)

体验一下使用 Quentlam 编写原生 C++ 应用是多么的优雅：

```cpp
#include <Quentlam.h>
#include <Quentlam/Base/EntryPoint.h>

class SandboxLayer : public Quentlam::Layer 
{
public:
    SandboxLayer() : Layer("Sandbox") {}

    void OnUpdate(Quentlam::Timestep ts) override 
    {
        // 1. 清屏
        Quentlam::RenderCommand::SetClearColor({0.1f, 0.1f, 0.1f, 1.0f});
        Quentlam::RenderCommand::Clear();

        // 2. 开启 2D 渲染管线
        Quentlam::Renderer2D::BeginScene(m_Camera);
        
        // 3. 提交绘制指令 (在屏幕中央绘制一个红色方块)
        Quentlam::Renderer2D::DrawQuad({0.0f, 0.0f, 0.0f}, {1.0f, 1.0f}, {0.8f, 0.2f, 0.3f, 1.0f});
        
        // 4. 结束并刷新渲染批次
        Quentlam::Renderer2D::EndScene();
    }
};

// 定义应用并推入你的游戏层
class SandboxApp : public Quentlam::Application 
{
public:
    SandboxApp() 
    {
        PushLayer(new SandboxLayer());
    }
};

// 引擎入口点绑定
Quentlam::Application* Quentlam::CreateApplication() 
{
    return new SandboxApp();
}
```

---

<div align="center">
  <strong>"Code the logic, render the world."</strong><br>
  <i>Built with ❤️ for Graphic Rendering and Engine Architecture.</i>
</div>
