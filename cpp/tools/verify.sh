#!/usr/bin/env bash
# 端到端验收：把参考版与 C++ 版用**同一个 exe 路径**各跑一遍，逐字节比对。
#
# 用法: verify.sh [参考exe] [C++ exe]
#
# 关键点：两个版本必须落在同一个文件路径上运行，因为 `-h` 会把 argv[0]
# 打印出来，路径不同会导致 help 输出必然不一致。

set -u

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"

# 参考版：从 HEAD 干净构建出来的 exe
REF_EXE="${1:-/tmp/r2ga_head/x64/Release/PVZ_reanim2godot_animation.exe}"
# C++ 版：与 C 工程并排落在 $(SolutionDir)x64\Release\
CPP_EXE="${2:-$REPO_ROOT/x64/Release/PVZ_reanim2godot_animation_cpp.exe}"

[ -f "$REF_EXE" ] || { echo "找不到参考 exe: $REF_EXE" >&2; exit 1; }
[ -f "$CPP_EXE" ] || { echo "找不到 C++ exe: $CPP_EXE（先构建 cpp 工程）" >&2; exit 1; }

RUN_BIN="$REPO_ROOT/cpp/.verify/bin/PVZ_reanim2godot_animation.exe"
WORK="$REPO_ROOT/cpp/.verify/work"
mkdir -p "$(dirname "$RUN_BIN")" "$WORK"

echo "== 参考 exe: $REF_EXE"
echo "== C++ exe:  $CPP_EXE"
echo

# 把被测 exe 装到固定路径。Windows 不允许覆盖正在运行的 exe，
# 而上一轮可能有残留进程短暂持有它，所以这里必须重试 + 用哈希确认装成功。
# 两侧必须用**同一个路径**运行：`-h` 会把 argv[0] 打印出来。
install_exe() {
    local src="$1" dst="$2" want got i
    want=$(sha256sum "$src" | cut -d' ' -f1)
    for i in $(seq 1 15); do
        cp -f "$src" "$dst" 2>/dev/null
        got=$(sha256sum "$dst" 2>/dev/null | cut -d' ' -f1)
        if [ "$got" = "$want" ]; then
            printf '%s' "$got"
            return 0
        fi
        sleep 1
    done
    echo "!! 无法把 $src 安装到 $dst（文件被占用？）" >&2
    return 1
}

run_side() {
    local label="$1" exe="$2"
    local h
    h=$(install_exe "$exe" "$RUN_BIN") || exit 1
    echo "[$label] 已装入 exe, sha256=$h"
    local m e
    m=$(bash "$SCRIPT_DIR/run_matrix.sh" "$RUN_BIN" "$WORK/matrix-$label" | sed -n 's/^RUN_DIR=//p')
    e=$(bash "$SCRIPT_DIR/run_errors.sh" "$RUN_BIN" "$WORK/errors-$label" | sed -n 's/^RUN_DIR=//p')
    printf '%s\n' "$m" > "$WORK/matrix-$label.path"
    printf '%s\n' "$e" > "$WORK/errors-$label.path"
    printf '%s\n' "$h" > "$WORK/exe-$label.sha256"
    echo "[$label] 矩阵目录 $m"
    echo "[$label] 错误路径目录 $e"
}

echo "---- 跑参考版 ----"
run_side ref "$REF_EXE"
echo
echo "---- 跑 C++ 版 ----"
run_side cpp "$CPP_EXE"
echo

REF_M=$(cat "$WORK/matrix-ref.path")
CPP_M=$(cat "$WORK/matrix-cpp.path")
REF_E=$(cat "$WORK/errors-ref.path")
CPP_E=$(cat "$WORK/errors-cpp.path")

REF_H=$(cat "$WORK/exe-ref.sha256")
CPP_H=$(cat "$WORK/exe-cpp.sha256")

echo "===== 0) 前置检查：两侧跑的确实是不同的二进制 ====="
echo "  参考版 sha256: $REF_H"
echo "  C++ 版 sha256: $CPP_H"
if [ "$REF_H" = "$CPP_H" ]; then
    echo ">>> 两侧哈希相同，说明 exe 没换成功，比对结果无意义 —— 中止 <<<"
    exit 1
fi
echo "  OK：两个二进制不同"
echo

fail=0

echo "===== 1) 选项矩阵逐字节比对 ====="
if bash "$SCRIPT_DIR/diff_matrix.sh" "$REF_M" "$CPP_M"; then
    echo ">>> 矩阵全部一致 <<<"
else
    echo ">>> 矩阵存在差异 <<<"
    fail=1
fi

echo
echo "===== 2) 错误路径 stdout 与退出码比对 ====="
err_fail=0
while IFS= read -r f; do
    base="$(basename "$f")"
    if ! cmp -s "$REF_E/$base" "$CPP_E/$base"; then
        echo "  [差异] $base"
        echo "         参考: $(head -c 200 "$REF_E/$base" | tr -d '\0' | tr '\n' '|')"
        echo "         对比: $(head -c 200 "$CPP_E/$base" | tr -d '\0' | tr '\n' '|')"
        err_fail=$((err_fail + 1))
    fi
done < <(find "$REF_E" -maxdepth 1 -type f \( -name '*.out' -o -name '*.exit' \) | sort)

# 反向检查
while IFS= read -r f; do
    base="$(basename "$f")"
    [ -f "$REF_E/$base" ] || { echo "  [多余] $base"; err_fail=$((err_fail + 1)); }
done < <(find "$CPP_E" -maxdepth 1 -type f \( -name '*.out' -o -name '*.exit' \) | sort)

if [ "$err_fail" -eq 0 ]; then
    echo ">>> 错误路径全部一致 <<<"
else
    echo ">>> 错误路径有 $err_fail 处差异 <<<"
    fail=1
fi

echo
if [ "$fail" -eq 0 ]; then
    echo "################ 验收通过：逐字节一致 ################"
else
    echo "################ 验收未通过 ################"
fi
exit "$fail"
