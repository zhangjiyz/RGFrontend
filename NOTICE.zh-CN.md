# 版权与署名声明

## RGFrontend 项目与修改

RGFrontend 是面向 H700 原厂 Linux 掌机的轻量游戏库与启动器。本仓库中的
独立产品树、通用平台架构、H700 APPS 集成、扫描与启动适配、用户状态存储、
发布脚本、文档及 RGFrontend 专有界面修改：

```text
Copyright (c) 2026 zhangjiyz
网站：zhangjiyz.com
```

任何源码或二进制形式的再发布都必须保留适用许可证、本文件及合理可见的
RGFrontend 作者署名：

```text
RGFrontend / zhangjiyz
```

署名可以出现在设置、关于、鸣谢、文档等同等可见位置，也可以根据不同屏幕
和设备重新排版、翻译或移动，但不得删除、隐藏或弱化到实际不可见。二次
修改版必须说明其为下游适配，不得暗示获得 zhangjiyz 官方背书。

RGFrontend 不是模拟器，不分发 ROM、BIOS、商业游戏媒体、模拟器核心、
系统镜像或设备厂商的模拟器二进制。本项目使用目标设备系统中已有的
模拟器栈和原厂启动链。

## 下游修改状态

RGFrontend 是独立产品树。项目在元数据兼容、H700 交互模式、SDL2 UI
行为和启动集成方面参考了 Pegasus Frontend 与 PegasusG by ROC，但不把
参考工程源码作为本仓库实现基线。

本项目是非官方下游适配，不是 Pegasus Frontend、PegasusG by ROC、
RetroArch、Libretro 或任何设备厂商的官方发布版，也不暗示获得这些项目
或作者的官方背书。

## 原始 Pegasus Frontend

Pegasus Frontend 及其贡献者保留各自版权。上游项目地址：

```text
https://github.com/mmatyas/pegasus-frontend
```

RGFrontend 可以准确描述“支持 Pegasus 元数据格式”，包括
`metadata.pegasus.txt`，但不得使用 Pegasus、Pegasus Frontend、
Pegasus Launcher 或容易混淆的标识作为本下游修改作品的产品标题或 Logo。

## PegasusG by ROC

本项目参考的 H700 SDL2 前端、输入、媒体预览、启动整合及机型适配工作中
包含 PegasusG by ROC 贡献：

```text
Copyright (c) 2026 Blood_roc
联系方式：QQ 825826146
```

任何源码或二进制形式的再发布都必须同时保留：

1. RGFrontend / zhangjiyz 的版权、许可和可见署名。
2. 原始 Pegasus Frontend 的版权、许可和适用附加条款。
3. Blood_roc 的移植署名以及本文件。
4. `LICENSE.md`、`NOTICE.md`、`NOTICE.zh-CN.md` 和
   `THIRD_PARTY_NOTICES.md`。
5. 程序内可见的“天马G ROC移植”，或在设置、关于、鸣谢、文档等同等
   可见位置显示的“PegasusG by ROC / Blood_roc”署名。
6. 对二次修改者所做改动的明确说明，且不得暗示原 Pegasus Frontend 作者
   或 Blood_roc 对下游修改版进行背书。

署名可以根据不同屏幕和设备重新排版、翻译或移动，但不得删除、隐藏或弱化
到实际不可见。

## 当前修改摘要

RGFrontend 使用独立产品名和品牌名，采用通用 `Platform`、`Game` 和
`Library` 架构，并拆分游戏库扫描、UI、结构化启动请求、H700 设备集成和
用户状态存储。本项目不执行元数据中的任意启动命令，不安装、替换或修改
系统模拟器核心、RetroArch 配置或原厂启动脚本。

## 第三方资源

第三方核心、音乐、图标、字体、滤镜、主题、金手指数据、BIOS、ROM、系统
镜像和设备厂商组件不当然继承 RGFrontend 前端源码许可证。公开再发布前
必须同时遵守 `THIRD_PARTY_NOTICES.md` 及相关上游许可说明。

## 无担保

RGFrontend 按 `LICENSE.md` 所述方式提供，不作任何担保。
