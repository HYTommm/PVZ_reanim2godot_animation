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

# 专项测试会切换工作目录，先把 exe 路径固定成绝对路径
case "$REF_EXE" in
    /*|[A-Za-z]:/*) ;;
    *) REF_EXE="$(cd "$(dirname "$REF_EXE")" && pwd)/$(basename "$REF_EXE")" ;;
esac
case "$CPP_EXE" in
    /*|[A-Za-z]:/*) ;;
    *) CPP_EXE="$(cd "$(dirname "$CPP_EXE")" && pwd)/$(basename "$CPP_EXE")" ;;
esac

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
echo "===== 3) 配置文件专项 ====="
CFG_ROOT="$WORK/config-special"
rm -rf "$CFG_ROOT"
mkdir -p "$CFG_ROOT"

to_win() { cygpath -w "$1" 2>/dev/null || printf '%s' "$1"; }

sample_bash=$(find "$REF_M/02_tscn_by_anim/input" -name '*.reanim' | sort | head -1)
if [ -z "$sample_bash" ]; then
    echo "  [跳过] 找不到样例"
else
    sample_win=$(to_win "$sample_bash")
    cfg_fail=0

    run_conf() {
        local name="$1"; shift
        local r="$CFG_ROOT/$name/ref" c="$CFG_ROOT/$name/cpp"
        mkdir -p "$r" "$c"
        ( cd "$r" && "$REF_EXE" "$@" > stdout.log 2>&1 )
        local rcode=$?
        ( cd "$c" && "$CPP_EXE" "$@" > stdout.log 2>&1 )
        local ccode=$?
        grep -v '^debug: OutputFiles:name--' "$r/stdout.log" > "$r/stdout.filtered" || true
        grep -v '^debug: OutputFiles:name--' "$c/stdout.log" > "$c/stdout.filtered" || true
        if [ "$rcode" != "$ccode" ] || ! cmp -s "$r/stdout.filtered" "$c/stdout.filtered"; then
            echo "  [差异] $name (退出码 $rcode/$ccode)"
            diff -u "$r/stdout.filtered" "$c/stdout.filtered" | head -10
            cfg_fail=1
            return
        fi
        if ! diff -r --exclude='stdout.log' --exclude='stdout.filtered' "$r" "$c" >/dev/null; then
            echo "  [差异] $name 产物"
            cfg_fail=1
            return
        fi
        echo "  [$name] OK"
    }

    # 3.1 新语法：无分号、值含空格与 #、引号、未知键、字典块
    cat > "$CFG_ROOT/new-syntax.cfg" <<EOF
OutputMode = TscnByAnim
RootNodeName = "Config # Node"
InterpolationMode = cubic
UnknownKey = 1
UpdateModeDic = {
    head = Discrete,
    body = Capture,
}
EOF
    run_conf new-syntax "$sample_win" res://anim/ res://art/ \
        -cf "$(to_win "$CFG_ROOT/new-syntax.cfg")" -of out
    if ! grep -rq 'name="Config # Node"' "$CFG_ROOT/new-syntax/ref"; then
        echo "  [失败] new-syntax 的配置没有生效"
        cfg_fail=1
    fi

    # 3.2 路径项进配置，位置参数全部省略
    cat > "$CFG_ROOT/paths.cfg" <<EOF
InputFile = $sample_win
AnimGodotPath = res://anim/
ResGodotPath = res://art/
RootNodeName = Paths
EOF
    run_conf paths -cf "$(to_win "$CFG_ROOT/paths.cfg")" --print-config
    run_conf paths-convert -cf "$(to_win "$CFG_ROOT/paths.cfg")" -of out

    # 3.3 旧的分号语法保持兼容
    cat > "$CFG_ROOT/old-syntax.cfg" <<'EOF'
OutputMode = TscnByAnim;
RootNodeName = OldSemi;
EOF
    run_conf old-syntax "$sample_win" res://anim/ res://art/ \
        -cf "$(to_win "$CFG_ROOT/old-syntax.cfg")" -of out

    # 3.4 配置模板
    run_conf init-config --init-config

    # 3.5 ExcludeTracks：全局属性排除
    cat > "$CFG_ROOT/exclude.cfg" <<'EOF'
ExcludeTracks = LawnMower_body:visible
EOF
    run_conf exclude "$sample_win" res://anim/ res://art/ \
        -om tscn_by_anim -cf "$(to_win "$CFG_ROOT/exclude.cfg")" -of out
    if grep -rq 'LawnMower_body:visible' "$CFG_ROOT/exclude/ref"; then
        echo "  [失败] ExcludeTracks 没有排除 visible"
        cfg_fail=1
    fi
    if ! grep -rq 'LawnMower_body:position' "$CFG_ROOT/exclude/ref"; then
        echo "  [失败] ExcludeTracks 误删了 position"
        cfg_fail=1
    fi

    # 3.6 ExcludeTracks：按动画整条排除
    cat > "$CFG_ROOT/exclude-anim.cfg" <<'EOF'
ExcludeTracks.normal = *body*
EOF
    run_conf exclude-anim "$sample_win" res://anim/ res://art/ \
        -om anim_tres -cf "$(to_win "$CFG_ROOT/exclude-anim.cfg")" -of out
    normal_tres=$(find "$CFG_ROOT/exclude-anim/ref" -name '*LawnMower_normal.tres' | head -1)
    tricked_tres=$(find "$CFG_ROOT/exclude-anim/ref" -name '*LawnMower_tricked.tres' | head -1)
    if [ -z "$normal_tres" ] || grep -q 'body:' "$normal_tres"; then
        echo "  [失败] normal 动画的 body 轨道没有被排除"
        cfg_fail=1
    fi
    if [ -z "$tricked_tres" ] || ! grep -q 'body:' "$tricked_tres"; then
        echo "  [失败] tricked 动画的 body 轨道被误删"
        cfg_fail=1
    fi

    if [ "$cfg_fail" -eq 0 ]; then
        echo ">>> 配置文件专项全部一致 <<<"
    else
        echo ">>> 配置文件专项存在差异 <<<"
        fail=1
    fi
fi

echo
if [ "$fail" -eq 0 ]; then
    echo "################ 验收通过：逐字节一致 ################"
else
    echo "################ 验收未通过 ################"
fi
exit "$fail"
