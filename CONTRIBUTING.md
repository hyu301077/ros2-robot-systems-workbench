# 参与贡献 / Contributing

本项目使用同一份源代码支持 Ubuntu 22.04 / ROS 2 Humble 与 Ubuntu 26.04 / ROS 2
Lyrical。GitHub 是同步中心；构建产物始终只属于各自终端，不能在终端间复制使用。

This project uses a single source branch for Ubuntu 22.04 / ROS 2 Humble and
Ubuntu 26.04 / ROS 2 Lyrical. GitHub is the synchronization point; generated
build products are always local to each terminal.

## 开始一个任务 / Start a task

```bash
git status --short --branch
git fetch --prune origin
tools/ai_context.sh
git pull --ff-only
git switch -c agent/<short-description>
```

不要在存在未提交改动的工作区上执行 `git pull`。若需要把未完成工作交接给另一终端，请先推送到
任务分支，并创建 Draft Pull Request。

Do not pull over an uncommitted worktree. Push unfinished work to its task
branch and use a Draft pull request when it must be handed to the other
terminal.

## 分离工作与个人 Git 身份 / Keep work and personal Git identities separate

不要为了本项目修改机器全局 Git 身份；应改为设置并检查仓库本地身份：

Do not change the machine-wide Git identity for this project. Configure and
verify a repository-local identity instead:

```bash
git config --local user.name
git config --local user.email
gh auth status --hostname github.com
```

如果 GitHub CLI 登录了多个账号，在远端写操作前选择预期账号：

When more than one account is logged in to GitHub CLI, select the intended
account before a remote write:

```bash
gh auth switch --hostname github.com --user <github-login>
```

提交作者身份与远端认证账号是两个独立配置。提交或推送前都应检查，并在公开提交中使用 GitHub
noreply 邮箱。

The commit author and the authenticated remote account are separate settings.
Check both before committing or pushing, and use a GitHub noreply address for
public commits.

## 构建环境 / Build environment

请打开一个新的 shell，并且只 source 与当前操作系统对应的 ROS 发行版：

Use a new shell and source only the ROS distribution matching the operating
system:

```bash
# 终端1 / Terminal 1
source /opt/ros/humble/setup.bash

# 终端2 / Terminal 2
source /opt/ros/lyrical/setup.bash
```

构建前运行 `tools/doctor.sh`。该脚本只诊断环境，不会安装软件或修改系统。

Run `tools/doctor.sh` before building. The script diagnoses the environment but
does not install or modify anything.

## 提交信息胶囊 / Commit capsule

保持小粒度改动，并在提交正文中写出让另一轮 AI 会话无需阅读此前对话也能理解的信息：

Keep changes small and use a commit body that another AI session can understand
without reading an earlier conversation:

```text
<short summary>

Why:
<reason>

Affected:
<packages or documentation>

Compatibility:
<Humble and Lyrical impact>

Validated:
<commands actually run and results>

Follow-up:
<remaining work, or "None">
```

不得只凭代码审阅就声称某个目标环境已经验证通过。

Never report a target as validated based only on code inspection.

## Pull Request

- 空仓库的首次初始化是唯一允许直接推送到 `main` 的情况。
  The initial empty-repository bootstrap is the only direct push to `main`.
- 初始化之后，不要直接推送到 `main`。
  After bootstrap, do not push directly to `main`.
- 一个 Pull Request 只处理一个任务。
  Keep one task per pull request.
- 只有系统事实变化时才更新紧凑系统清单。
  Update the compact system manifest only when system facts change.
- CI 可用后，合并前必须要求 Humble 与 Lyrical 两个 job 都通过。
  Require the Humble and Lyrical CI jobs before merging once CI is available.
- 优先使用 squash merge，使最终提交成为简洁、可交接的信息胶囊。
  Prefer squash merging so the final commit remains a concise handoff capsule.
