# Contributing

This project uses a single source branch for Ubuntu 22.04 / ROS 2 Humble and
Ubuntu 26.04 / ROS 2 Lyrical. GitHub is the synchronization point; generated
build products are always local to each terminal.

## Start a task

```bash
git status --short --branch
git fetch --prune origin
tools/ai_context.sh
git pull --ff-only
git switch -c agent/<short-description>
```

Do not pull over an uncommitted worktree. Push unfinished work to its task
branch and use a Draft pull request when it must be handed to the other
terminal.

## Keep work and personal Git identities separate

Do not change the machine-wide Git identity for this project. Configure and
verify a repository-local identity instead:

```bash
git config --local user.name
git config --local user.email
gh auth status --hostname github.com
```

When more than one account is logged in to GitHub CLI, select the intended
account before a remote write:

```bash
gh auth switch --hostname github.com --user <github-login>
```

The commit author and the authenticated remote account are separate settings.
Check both before committing or pushing, and use a GitHub noreply address for
public commits.

## Build environment

Use a new shell and source only the ROS distribution matching the operating
system:

```bash
# Terminal 1
source /opt/ros/humble/setup.bash

# Terminal 2
source /opt/ros/lyrical/setup.bash
```

Run `tools/doctor.sh` before building. The script diagnoses the environment but
does not install or modify anything.

## Commit capsule

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

Never report a target as validated based only on code inspection.

## Pull requests

- The initial empty-repository bootstrap is the only direct push to `main`.
- After bootstrap, do not push directly to `main`.
- Keep one task per pull request.
- Update the compact system manifest only when system facts change.
- Require the Humble and Lyrical CI jobs before merging once CI is available.
- Prefer squash merging so the final commit remains a concise handoff capsule.
