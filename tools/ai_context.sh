#!/usr/bin/env bash

set -euo pipefail

fetch_remote=false
mark_seen=false
base_ref=""
max_commits=30
max_lines=80

usage() {
  echo "Usage: tools/ai_context.sh [--fetch] [--mark-seen] [--base <git-ref>]"
}

while (($# > 0)); do
  case "$1" in
    --fetch)
      fetch_remote=true
      shift
      ;;
    --mark-seen)
      mark_seen=true
      shift
      ;;
    --base)
      if (($# < 2)); then
        usage
        exit 2
      fi
      base_ref="$2"
      shift 2
      ;;
    --help|-h)
      usage
      exit 0
      ;;
    *)
      usage
      exit 2
      ;;
  esac
done

if ! git rev-parse --is-inside-work-tree >/dev/null 2>&1; then
  echo "ERROR: run this command inside the project Git repository."
  exit 1
fi

if "${fetch_remote}"; then
  if git remote get-url origin >/dev/null 2>&1; then
    git fetch --prune origin
  else
    echo "NOTICE: no origin remote exists; skipping fetch."
  fi
fi

if ! git rev-parse --verify --quiet "HEAD^{commit}" >/dev/null; then
  repository_name="$(basename "$(git rev-parse --show-toplevel)")"
  branch_name="$(git branch --show-current)"

  echo "repository=${repository_name}"
  echo "branch=${branch_name:-unborn}"
  echo "head=unborn"
  echo "base=none"
  echo "context_start=none"
  echo
  echo "[worktree]"
  git status --short --branch | sed -n "1,${max_lines}p"
  echo
  echo "[history]"
  echo "none (create the initial commit before using incremental context)"

  if "${mark_seen}"; then
    echo "ERROR: cannot mark an unborn branch as seen."
    exit 1
  fi

  exit 0
fi

if [[ -z "${base_ref}" ]]; then
  if git rev-parse --verify --quiet origin/main >/dev/null; then
    base_ref="origin/main"
  elif git symbolic-ref --quiet refs/remotes/origin/HEAD >/dev/null; then
    base_ref="$(git symbolic-ref --quiet --short refs/remotes/origin/HEAD)"
  elif git rev-parse --verify --quiet main >/dev/null; then
    base_ref="main"
  elif git rev-parse --verify --quiet master >/dev/null; then
    base_ref="master"
  else
    base_ref="HEAD"
  fi
fi

if ! git rev-parse --verify --quiet "${base_ref}^{commit}" >/dev/null; then
  echo "ERROR: base reference does not resolve to a commit: ${base_ref}"
  exit 1
fi

current_head="$(git rev-parse --short HEAD)"
base_commit="$(git rev-parse "${base_ref}^{commit}")"
base_short="$(git rev-parse --short "${base_commit}")"
branch_name="$(git branch --show-current)"
last_seen="$(git config --local --get ai.lastSeen || true)"

if [[ -n "${last_seen}" ]] &&
   git rev-parse --verify --quiet "${last_seen}^{commit}" >/dev/null; then
  start_ref="${last_seen}"
  start_source="ai.lastSeen"
else
  start_ref="HEAD"
  start_source="HEAD"
fi

start_short="$(git rev-parse --short "${start_ref}")"

echo "repository=$(basename "$(git rev-parse --show-toplevel)")"
echo "branch=${branch_name:-detached}"
echo "head=${current_head}"
echo "base=${base_ref}:${base_short}"
echo "context_start=${start_source}:${start_short}"

echo
echo "[worktree]"
git status --short --branch | sed -n "1,${max_lines}p"

echo
echo "[head_vs_base]"
git rev-list --left-right --count "HEAD...${base_ref}" |
  awk '{print "ahead=" $1 " behind=" $2}'

echo
echo "[new_commits]"
if [[ "$(git rev-list --count "${start_ref}..${base_ref}")" -eq 0 ]]; then
  echo "none"
else
  git log --no-merges --format='%h %s' \
    --max-count="${max_commits}" "${start_ref}..${base_ref}"
fi

echo
echo "[changed_files]"
if git diff --quiet "${start_ref}..${base_ref}"; then
  echo "none"
else
  git diff --name-status "${start_ref}..${base_ref}" |
    sed -n "1,${max_lines}p"
fi

echo
echo "[diff_stat]"
if git diff --quiet "${start_ref}..${base_ref}"; then
  echo "none"
else
  git diff --stat "${start_ref}..${base_ref}" |
    sed -n "1,${max_lines}p"
fi

if "${mark_seen}"; then
  git config --local ai.lastSeen "${base_commit}"
  echo
  echo "marked_seen=${base_short}"
else
  echo
  echo "next=review changes, update safely, then rerun with --mark-seen"
fi
