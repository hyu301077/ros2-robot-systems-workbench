# Repository instructions for AI-assisted work

This repository is a ROS 2 learning system maintained from two independent
development machines. Git history is the shared memory between AI sessions.
Keep context compact and prefer evidence from the repository over chat history.

## Supported targets

- Terminal 1: Ubuntu 22.04, ROS 2 Humble, compatibility target.
- Terminal 2: Ubuntu 26.04, ROS 2 Lyrical, long-term primary target.
- Use C++17 and APIs available in both ROS 2 distributions.
- Each terminal runs the complete node system locally. Cross-device and
  cross-distribution ROS communication is out of scope.

## Sources of truth

Read in this order:

1. Current `git status`, branch, and diff.
2. `docs/system_manifest.yaml` for the compact system inventory.
3. `package.xml` and `CMakeLists.txt` for real package dependencies.
4. Focused documentation and source files affected by the current change.

Do not scan every file when the Git diff and dependency direction identify a
smaller relevant set. A change to the interface package is the exception: all
message producers and consumers must then be inspected and tested.

## Session start

1. Run `git status --short --branch`; never overwrite unrelated local changes.
2. Run `tools/doctor.sh` to identify the current OS/ROS target.
3. Run `tools/ai_context.sh --fetch` to inspect upstream changes before pulling.
4. If the tree is clean and the branch is only behind, update with
   `git pull --ff-only`.
5. Read only the new commit capsules and affected files.

Never source Humble and Lyrical in the same shell. Build in a fresh shell after
sourcing exactly one `/opt/ros/<distro>/setup.bash`.

## Change rules

- Keep one `main` branch for both distributions; use short-lived task branches.
- V0 intentionally maps one executable to one process and one ROS node.
- Do not add Services, Actions, Gazebo, Docker, databases, cloud integrations,
  OTA, hardware protocols, or complex threading unless the user expands scope.
- Do not add a dependency until its package name and API are verified for both
  Humble and Lyrical.
- Do not use `sudo`, install packages, build large dependencies from source, or
  change GitHub state without explicit user authorization.
- Keep `docs/system_manifest.yaml` factual: planned work is not implemented
  work, and unexecuted validation is not a passing result.
- Do not commit build output, credentials, recordings, full AI conversations,
  or hidden reasoning.

## Validation and publishing

Run the most focused relevant test during development, then run the complete
workspace build, tests, test-result check, and bounded launch smoke test before
publishing. Record the exact environment and distinguish local results from CI.

Commits and pull requests must state:

- why the change exists;
- what changed and which packages are affected;
- Humble/Lyrical compatibility impact;
- commands actually executed and their results;
- remaining risks or follow-up work.

Never claim both targets are supported until both have completed real builds and
tests for the relevant revision.
