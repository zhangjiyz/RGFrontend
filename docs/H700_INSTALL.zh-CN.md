# RGFrontend H700 安装说明

[English](H700_INSTALL.md) | [简体中文](H700_INSTALL.zh-CN.md)

本文适用于 H700 原厂 Linux 掌机的 release zip 安装包，例如：

```text
RGFrontend-H700-1.0.3.zip
```

RGFrontend 是游戏库与启动器，不包含 ROM、BIOS、模拟器或核心。安装包只会放入 APPS 入口和 RGFrontend 运行文件，不会修改系统 RetroArch 配置、系统核心、原厂启动脚本或系统启动项。

## 安装前准备

- 一台 H700 原厂 Linux 掌机。
- 一张已能被掌机识别的 ROM 卡。
- release zip，例如 `RGFrontend-H700-1.0.3.zip`。

zip 内部目录从 `Roms/` 开始：

```text
Roms/APPS/RGFrontend.sh
Roms/APPS/RGFrontend/
Roms/APPS/Imgs/RGFrontend.png
```

因此必须把 zip 解压到 TF 卡根目录，不要解压到 `Roms/APPS` 里面。

## 全新安装

1. 关闭掌机，取出 ROM 卡并插到电脑。
2. 把 `RGFrontend-H700-1.0.3.zip` 复制到 TF 卡根目录。

   正确位置示例：

   ```text
   TF卡根目录/RGFrontend-H700-1.0.3.zip
   TF卡根目录/Roms/
   ```

3. 在 TF 卡根目录解压这个 zip。

   解压完成后应出现：

   ```text
   TF卡根目录/Roms/APPS/RGFrontend.sh
   TF卡根目录/Roms/APPS/RGFrontend/
   TF卡根目录/Roms/APPS/Imgs/RGFrontend.png
   ```

4. 安全弹出 TF 卡，插回掌机并开机。
5. 进入系统 APPS 列表，选择 `RGFrontend` 启动。

## 通过 SSH 安装

如果掌机已开启 SSH，也可以直接把 zip 拷到卡根目录并在真机上解压：

```sh
scp RGFrontend-H700-1.0.3.zip root@10.1.1.233:/mnt/mmc/
ssh root@10.1.1.233
cd /mnt/mmc
unzip -o RGFrontend-H700-1.0.3.zip
```

安装后可检查这些路径：

```sh
ls -ld \
  /mnt/mmc/Roms/APPS/RGFrontend \
  /mnt/mmc/Roms/APPS/RGFrontend.sh \
  /mnt/mmc/Roms/APPS/Imgs/RGFrontend.png
```

## 分辨率兼容

RGFrontend 已包含 720x480、640x480 和 720x720 界面布局。H700 原厂发布包使用 720x480。

640x480 和 720x720 布局已经包含，但在标记为完整验证前，还需要在对应设备上确认显示、输入、媒体播放和游戏返回。

## 许可证

RGFrontend 以 GPL-3.0-or-later 发布，并保留本项目、Pegasus Frontend 与 PegasusG by ROC / Blood_roc 的适用版权、许可证、署名和修改说明。

安装后的许可证与第三方声明位于：

```text
Roms/APPS/RGFrontend/licenses/
```

## 升级或重装

升级时可直接把新版 zip 放到 TF 卡根目录并重新解压：

```sh
cd /mnt/mmc
unzip -o RGFrontend-H700-1.0.3.zip
```

如需干净重装，只删除以下 APPS 文件后再解压 zip：

```text
Roms/APPS/RGFrontend
Roms/APPS/RGFrontend.sh
Roms/APPS/Imgs/RGFrontend.png
```

如果旧版本曾使用 `RetroFrontend` 名称，也可以删除旧入口：

```text
Roms/APPS/RetroFrontend
Roms/APPS/RetroFrontend.sh
Roms/APPS/Imgs/RetroFrontend.png
```

不要删除 ROM、BIOS、系统核心或系统配置目录。

## 卸载

删除以下文件即可卸载 APPS 入口和程序文件：

```text
Roms/APPS/RGFrontend
Roms/APPS/RGFrontend.sh
Roms/APPS/Imgs/RGFrontend.png
```

RGFrontend 的收藏、最近、扫描缓存和设置默认保存在掌机应用私有目录：

```text
/mnt/data/multiplatform-launcher
```

普通卸载可以保留这个目录，方便以后重装恢复状态。只有在想完全清空设置和缓存时再删除它。

## 常见问题

### APPS 里没有看到 RGFrontend

通常是 zip 解压位置不对。请确认不是这样的结构：

```text
TF卡根目录/RGFrontend-H700-1.0.3/Roms/APPS/...
```

正确结构应是：

```text
TF卡根目录/Roms/APPS/RGFrontend.sh
```

### 图标没有更新

确认菜单图标存在：

```text
Roms/APPS/Imgs/RGFrontend.png
```

如果系统 APPS 列表有缓存，可重启掌机后再看。

### 启动后没有游戏

RGFrontend 会扫描原厂 ROM 根目录：

```text
/mnt/mmc/Roms
/mnt/sdcard/Roms
```

只有设备系统已支持且本启动器可启动的平台会显示。没有 ROM、平台不可启动或目录不符合设备能力时，不会在普通游戏库里显示。
