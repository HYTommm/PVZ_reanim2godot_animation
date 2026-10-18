#!/usr/bin/env bash
# 错误路径 / 边界参数验收：比对 stdout 与退出码
#
# 用法: run_errors.sh <exe路径> <输出根目录>

set -u

EXE_IN="${1:?用法: run_errors.sh <exe路径> <输出根目录>}"
OUT_ROOT="${2:?用法: run_errors.sh <exe路径> <输出根目录>}"

case "$EXE_IN" in
    /*|[A-Za-z]:/*) EXE="$EXE_IN" ;;
    *) EXE="$(cd "$(dirname "$EXE_IN")" && pwd)/$(basename "$EXE_IN")" ;;
esac

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
SRC="C:/Users/HYTomZ/Pictures/pvz素材"
[ -d "$SRC" ] || { echo "找不到素材目录: $SRC" >&2; exit 1; }

OUT_ROOT="$(mkdir -p "$OUT_ROOT" && cd "$OUT_ROOT" && pwd)"
SAMPLE="MainGame/Zombie/Zombie_walk.reanim"

# 每条: 名字 | 参数（不含 exe）
CASES=(
    "e01_no_args|"
    "e02_one_arg|a.reanim"
    "e03_two_args|a.reanim res://anim/"
    "e04_bad_output_mode|SAMPLE res://anim/ res://art/ -om bogus"
    "e05_bad_interp|SAMPLE res://anim/ res://art/ -im bogus"
    "e06_bad_update|SAMPLE res://anim/ res://art/ -um bogus"
    "e07_bad_frame|SAMPLE res://anim/ res://art/ -fm bogus"
    "e08_bad_track|SAMPLE res://anim/ res://art/ -tm bogus"
    "e09_unknown_opt|SAMPLE res://anim/ res://art/ --totally-unknown"
    "e10_missing_value|SAMPLE res://anim/ res://art/ -im"
    "e11_missing_input|nope_does_not_exist.reanim res://anim/ res://art/"
    "e12_missing_config|SAMPLE res://anim/ res://art/ -cf nope.cfg"
    "e13_help_short|-h"
    "e14_help_long|--help"
    "e15_help_after|SAMPLE res://anim/ res://art/ -h -im cubic"
)

# 与 run_matrix.sh 一致：每次用唯一子目录，避免陈旧目录与删除失败的问题
OUT_ROOT="$OUT_ROOT/run-$(date +%s)-$$"
mkdir -p "$OUT_ROOT/run/input"
while IFS= read -r f; do
    mkdir -p "$OUT_ROOT/run/input/$(dirname "$f")"
    cp "$SRC/$f" "$OUT_ROOT/run/input/$f"
done < <(cd "$SRC" && find . -iname '*.reanim' | sed 's|^\./||')

for entry in "${CASES[@]}"; do
    name="${entry%%|*}"
    args="${entry#*|}"
    args="${args//SAMPLE/input\/$SAMPLE}"
    ( cd "$OUT_ROOT/run" && "$EXE" $args > "$OUT_ROOT/$name.out" 2>&1 )
    printf '%d\n' "$?" > "$OUT_ROOT/$name.exit"
done

echo "RUN_DIR=$OUT_ROOT"
