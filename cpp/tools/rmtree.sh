#!/usr/bin/env bash
# 可靠地删除一个目录树。
#
# 本机 git-bash 的 rm -rf 在含空格/中文的路径上（样例里有 "SunFlower - 副本"）
# 会静默失败，留下半截目录，导致后续 run_matrix 合并进陈旧内容、产生假差异。
# 所以先试 rm -rf，不行再退回 Windows 的 rmdir /s /q。
#
# 用法: rmtree.sh <目录> [<目录>...]

set -u

for target in "$@"; do
    [ -e "$target" ] || { echo "  $target: 不存在，跳过"; continue; }

    rm -rf "$target" 2>/dev/null

    if [ -e "$target" ]; then
        # 转成 Windows 路径给 rmdir 用
        win="$(cd "$(dirname "$target")" && pwd -W)/$(basename "$target")" 2>/dev/null || win=""
        if [ -n "$win" ]; then
            win="${win//\//\\}"
            cmd //c "rmdir /s /q \"$win\"" >/dev/null 2>&1
        fi
    fi

    if [ -e "$target" ]; then
        echo "!! 无法删除: $target" >&2
        exit 1
    fi
    echo "  已删除 $target"
done
