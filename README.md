# Bodycam Visual Toolkit V3

一个基于 **DirectX 11** 与 **Dear ImGui** 构建的高性能 iOS/macOS 亚克力液态玻璃风格桌面工具箱。项目采用了模块化架构设计，将渲染管线、Win32 窗口底座、自绘 UI 控件与视图渲染彻底解耦。

---

## 🏗️ 项目架构与模块说明

为了方便后续维护与扩展，项目按功能职责进行了高度剥离。各模块的具体定位如下：

| 模块名称 | 文件名 (`.h` / `.cpp`) | 职责定位与作用 | 状态 |
| :--- | :--- | :--- | :--- |
| **Win32 API** | `Win32_API` | 封装 Windows 原生 DWM 接口，实现亚克力 (Acrylic) / 毛玻璃无边框高阶效果。 | **已独立** |
| **Direct3D Resource** | `Direct3D_Resource` | 负责 Direct3D 11 设备初始化、SwapChain 创建与 RenderTarget 资源生命周期管理。 | **已独立** |
| **Rain Effect Pipeline** | `RainEffectPipeline` | 动态雨滴背景渲染管线，基于 Shader 实现流体视觉与着色器特效。 | **已独立** |
| **UI Theme** | `UI_Theme` | 苹果风玻璃主题样式配置、调色板以及液态流体胶囊（Liquid Capsule）平滑物理算法。 | **已独立** |
| **UI Controls** | `UI_Controls` | 自绘底层原子控件（如 Mac 风格红黄绿三色控制按钮、侧边栏 Option 交互项等）。 | **已独立** |
| **UI Header** | `UI_Header` | 渲染顶部标题胶囊区、全局搜索输入框、无边框窗口拖拽响应区及窗口控制组件。 | **已独立** |
| **UI Sidebar** | `UI_Sidebar` | 侧边栏整体容器布局计算、选项卡状态管理以及水痕流体胶囊的动画渲染。 | **已独立** |
| **Main Views** | `MainViews` | 各功能页面的具体内容视图渲染（如：首页、分辨率修复、设置等 Tab 内容）。 | **已独立** |
| **Main Entrance** | `main.cpp` | 程序主入口、Win32 消息循环 (`WndProc`) 调度以及各子 UI 模块的联合绘制中枢。 | **重构完成** |

---

## 🛠️ 技术栈与依赖

* **C++17** / **MSVC**
* **DirectX 11** (`d3d11.lib`, `dwmapi.lib`)
* **Dear ImGui** (Win32 + DX11 Impl)
* **Windows DWM API** (Acrylic Blur & Corner Radius)

---

## 🚀 构建与运行

1. 使用 **Visual Studio 2022** 打开解决方案项目。
2. 确保已连接 Direct3D 11 及 Windows SDK 依赖库。
3. 将上述表格中的所有包含 `.h` 和 `.cpp` 源代码添加到工程项目中。
4. 编译选项选择 `Release | x64` 或 `Debug | x64` 并运行即可。# Bodycam Visual Toolkit V3

一个基于 **DirectX 11** 与 **Dear ImGui** 构建的高性能 iOS/macOS 亚克力液态玻璃风格桌面工具箱。项目采用了模块化架构设计，将渲染管线、Win32 窗口底座、自绘 UI 控件与视图渲染彻底解耦。

---

## 🏗️ 项目架构与模块说明

为了方便后续维护与扩展，项目按功能职责进行了高度剥离。各模块的具体定位如下：

| 模块名称 | 文件名 (`.h` / `.cpp`) | 职责定位与作用 | 状态 |
| :--- | :--- | :--- | :--- |
| **Win32 API** | `Win32_API` | 封装 Windows 原生 DWM 接口，实现亚克力 (Acrylic) / 毛玻璃无边框高阶效果。 | **已独立** |
| **Direct3D Resource** | `Direct3D_Resource` | 负责 Direct3D 11 设备初始化、SwapChain 创建与 RenderTarget 资源生命周期管理。 | **已独立** |
| **Rain Effect Pipeline** | `RainEffectPipeline` | 动态雨滴背景渲染管线，基于 Shader 实现流体视觉与着色器特效。 | **已独立** |
| **UI Theme** | `UI_Theme` | 苹果风玻璃主题样式配置、调色板以及液态流体胶囊（Liquid Capsule）平滑物理算法。 | **已独立** |
| **UI Controls** | `UI_Controls` | 自绘底层原子控件（如 Mac 风格红黄绿三色控制按钮、侧边栏 Option 交互项等）。 | **已独立** |
| **UI Header** | `UI_Header` | 渲染顶部标题胶囊区、全局搜索输入框、无边框窗口拖拽响应区及窗口控制组件。 | **已独立** |
| **UI Sidebar** | `UI_Sidebar` | 侧边栏整体容器布局计算、选项卡状态管理以及水痕流体胶囊的动画渲染。 | **已独立** |
| **Main Views** | `MainViews` | 各功能页面的具体内容视图渲染（如：首页、分辨率修复、设置等 Tab 内容）。 | **已独立** |
| **Main Entrance** | `main.cpp` | 程序主入口、Win32 消息循环 (`WndProc`) 调度以及各子 UI 模块的联合绘制中枢。 | **重构完成** |

---

## 🛠️ 技术栈与依赖

* **C++17** / **MSVC**
* **DirectX 11** (`d3d11.lib`, `dwmapi.lib`)
* **Dear ImGui** (Win32 + DX11 Impl)
* **Windows DWM API** (Acrylic Blur & Corner Radius)

---

## 🚀 构建与运行

1. 使用 **Visual Studio 2022** 打开解决方案项目。
2. 确保已连接 Direct3D 11 及 Windows SDK 依赖库。
3. 将上述表格中的所有包含 `.h` 和 `.cpp` 源代码添加到工程项目中。
4. 编译选项选择 `Release | x64` 或 `Debug | x64` 并运行即可。