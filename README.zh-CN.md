# RGFrontend

[English](README.md) | [简体中文](README.zh-CN.md)

RGFrontend 是一个面向 H700 原厂 Linux 掌机的轻量游戏库与启动器。它负责扫描、整理、展示和启动设备中已有的游戏。本项目不是模拟器，也不分发 ROM、BIOS、模拟器核心、固件文件或商业媒体。

## 功能

- H700 原厂 Linux APPS 启动包。
- 掌机界面已包含 720x480、640x480 和 720x720 布局，并提供平台导航、封面网格、详情面板、收藏、最近游戏和设置。
- 支持 `/mnt/mmc/Roms` 与 `/mnt/sdcard/Roms` 双卡扫描。
- 支持普通 ROM 目录扫描。
- 支持 Pegasus `metadata.pegasus.txt`，包括单游戏启动/核心提示。
- 支持 EmulationStation `gamelist.xml`。
- 支持安伯尼克常见 `Imgs/` 媒体目录。
- 支持封面、Logo 和视频预览；媒体缺失时安全回退。
- 通过设备已有的 RetroArch 和独立模拟器启动链运行游戏。
- 支持单游戏核心选择，优先级为：
  用户覆盖 > Pegasus元数据提示 > 系统默认。
- 支持一键清空缓存并重新扫描。
- 左侧详情长游戏名横向滚动。

## 系统安全

RGFrontend 不会改动系统文件：

- 使用目标固件已有的 RetroArch、独立模拟器、核心和启动链。
- 不安装、替换、重命名或删除系统核心。
- 不修改 RetroArch 主配置、平台配置、核心配置、remap、shader、作弊库、原厂启动脚本或系统启动项。
- Pegasus `launch` 字段只用于可信的平台和核心提示。
- 收藏、最近、缓存、UI状态、日志和诊断只保存在应用自己的数据目录。

## 安装

请查看 H700 安装说明：

- [中文安装说明](docs/H700_INSTALL.zh-CN.md)
- [English install guide](docs/H700_INSTALL.md)

release zip 需要解压到 TF 卡根目录。zip 内部目录从 `Roms/` 开始：

```text
Roms/APPS/RGFrontend.sh
Roms/APPS/RGFrontend/
Roms/APPS/Imgs/RGFrontend.png
```

## 分辨率兼容

RGFrontend 已包含以下界面布局：

| 分辨率 | 状态 |
| --- | --- |
| 720x480 | 已包含，并用于 H700 原厂发布包 |
| 640x480 | 已包含，作为紧凑 4:3 布局 |
| 720x720 | 已包含，作为方屏布局 |

H700 原厂发布包使用 720x480。640x480 和 720x720 布局已经包含，但在标记为完整验证前，还需要在对应设备上确认显示、输入、媒体播放和游戏返回。

## H700默认操作

- `方向键`：移动选择。
- `A`：确认、启动游戏或保存选择。
- `B`：返回或取消。
- `L1 / R1`：切换平台或一级分类。
- `L2 / R2`：滚动游戏介绍。
- `X`：显示或隐藏封面标题。
- `Y`：打开当前游戏核心选择。
- `Select`：收藏或取消收藏。
- `Start / Menu`：打开设置。
- `音量 - / +`：调整系统音量。
- `Power`：休眠。

## H700原厂平台映射与 RGFrontend测试状态

下表共54个平台，均直接对应已检查的 H700原厂固件 `dmenu.bin`中的平台定义。

“原厂 dmenu有映射”表示菜单中存在平台名称、图标、扩展名或启动器等识别逻辑，
不代表所有 H700固件版本都带有每一个可选程序或核心，也不代表该平台的所有游戏
都已经验证。“是”表示 RGFrontend至少实际走通过该平台的一条启动路径；“待填”
表示仍待完整验证。

| 平台 | RGFrontend状态 |
| --- | --- |
| A2600 | 是 |
| A5200 | 是 |
| A7800 | 是 |
| A800 | 待填 |
| AMIGA | 待填 |
| ATARIST | 待填 |
| ATOMISWAVE | 是 |
| C64 | 待填 |
| CPS1 | 是 |
| CPS2 | 是 |
| CPS3 | 是 |
| DOS | 是 |
| DREAMCAST | 是 |
| EASYRPG | 是 |
| FBNEO | 是 |
| FC | 是 |
| FDS | 是 |
| GB | 是 |
| GBC | 是 |
| GBA | 是 |
| GG | 是 |
| GW | 是 |
| HBMAME | 是 |
| JAVA | 是 |
| LYNX | 是 |
| MAME | 是 |
| MD | 是 |
| MDCD | 是 |
| MSX | 是 |
| N64 | 是 |
| NAOMI | 是 |
| NDS | 是 |
| NEOCD | 是 |
| NEOGEO | 是 |
| NGP | 是 |
| ONS | 是 |
| OPENBOR | 是 |
| PCE | 是 |
| PCECD | 是 |
| PGM2 | 待填 |
| PICO | 是 |
| POKE | 是 |
| PORTS | 是 |
| PS | 是 |
| PSP | 是 |
| SATURN | 是 |
| SCUMMVM | 待填 |
| SEGA32X | 是 |
| SFC | 是 |
| SMS | 是 |
| VARCADE | 是 |
| VB | 是 |
| VIC20 | 待填 |
| WS | 是 |

## 许可证与署名

RGFrontend 以 GPL-3.0-or-later 发布，并保留本项目、Pegasus Frontend 与 PegasusG by ROC / Blood_roc 的适用许可证、版权、署名和修改说明。

详见：

- [LICENSE.md](LICENSE.md)
- [NOTICE.md](NOTICE.md)
- [NOTICE.zh-CN.md](NOTICE.zh-CN.md)
- [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md)

发布包不包含 ROM、BIOS、商业媒体、模拟器核心、固件镜像或设备转储。
