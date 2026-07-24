# ADR 0001：单仓库、双发行版、本地独立运行

- 状态：已接受
- 日期：2026-07-24

## 背景

开发者使用Ubuntu 22.04和Ubuntu 26.04两台电脑，希望共享源码并让AI快速理解另一终端的
改动。两个终端不需要组成同一个ROS运行图。

## 决策

- 所有ROS package保存在一个GitHub monorepo和一个`main`分支。
- Ubuntu 22.04使用ROS 2 Humble，作为兼容目标。
- Ubuntu 26.04使用ROS 2 Lyrical，作为长期主目标。
- 两个终端分别构建并运行完整系统，只通过GitHub同步源码。
- Git提交、PR模板、`system_manifest.yaml`和`ai_context.sh`共同构成AI交接上下文。
- 不共享`build/`、`install/`、`log/`或二进制文件。

## 结果

每次变更必须分别验证两个目标，但不承担混合发行版DDS运行兼容。Humble在2027年5月
结束支持，因此终端1未来需要迁移。
