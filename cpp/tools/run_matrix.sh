#!/usr/bin/env bash
# R2Ga 逐字节验收矩阵驱动器
#
# 用法: run_matrix.sh <exe路径> <输出父目录> [素材目录]
#
# 每次运行都在父目录下新建一个唯一子目录（绝不复用），并把该路径作为
# "RUN_DIR=..." 打印出来供调用方捕获。唯一目录是必须的：本机 git-bash 的
# rm -rf 在含空格/中文的路径上（样例里有 "SunFlower - 副本"）会静默失败，
# 留下半截目录会让新一轮合并进陈旧内容、产生假差异。
#
# 每个 case 一个子目录，内含：
#   stdout.log   合并的 stdout+stderr
#   exit.log     "<样例> <退出码>" 逐行
#   input/       镜像的 .reanim；未传 -of 的 case 产物也落在这里
#   outdir/      传了 -of 的 case 的产物
#
# 环境变量 MAXJOBS 控制并行度（默认 8）。

set -u

EXE_IN="${1:?用法: run_matrix.sh <exe路径> <输出父目录> [素材目录]}"
OUT_ROOT="${2:?用法: run_matrix.sh <exe路径> <输出父目录> [素材目录]}"
SRC="${3:-C:/Users/HYTomZ/Pictures/pvz素材}"
MAXJOBS="${MAXJOBS:-8}"

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

case "$EXE_IN" in
    /*|[A-Za-z]:/*) EXE="$EXE_IN" ;;
    *) EXE="$(cd "$(dirname "$EXE_IN")" && pwd)/$(basename "$EXE_IN")" ;;
esac

[ -d "$SRC" ] || { echo "找不到素材目录: $SRC" >&2; exit 1; }

mkdir -p "$OUT_ROOT"
OUT_ROOT="$(cd "$OUT_ROOT" && pwd)"
OUT_ROOT="$OUT_ROOT/run-$(date +%s)-$$"
mkdir "$OUT_ROOT"

# 每个 case: 名字 | 额外参数（OUTDIR / CONFIG 是占位符）
CASES=(
    # 只给 3 个位置参数、不给任何选项 -> argc==4 -> 解析失败 -> exit 1
    # （原实现的怪癖：3 个位置参数单独不够，必须至少再带一个选项）
    "01_no_options_rejected|"
    "02_tscn_by_anim|-om tscn_by_anim"
    "03_anim_tres|-om anim_tres"
    "04_om_tscn|-om tscn"
    "05_transform|-tm transform"
    "06_transform_cubic|-im cubic -tm transform"
    "07_keyframe|-fm keyframe"
    "08_keyframe_transform|-fm keyframe -tm transform"
    "09_blend|-bm"
    "10_noblend|-nbm"
    "11_names|-rnt Sprite2D -rnn TestNode -an MyAnim"
    "12_output_file_dir|-of OUTDIR/"
    "13_output_file_bare|-of outdir"
    "14_update_discrete|-um discrete"
    "15_update_capture|-um capture"
    "16_interp_nearest|-im nearest"
    "17_config|-cf CONFIG"
    "18_config_cli_override|-cf CONFIG -im nearest -fm inherit"
)

CONFIG_BODY='# R2Ga 测试配置
OutputMode = TscnByAnim
BlendModeEnabled = true
VisibleTrackEnabled = true
TextureTrackEnabled = false
AlphaTrackEnabled = true
RootNodeType = Node2D
RootNodeName = ConfigNode
AnimName = ConfigAnim
InterpolationMode = Cubic
UpdateMode = Discrete
FrameMode = Keyframe
TrackMode = Transform
UpdateModeDic = {
    .Head = Capture,
    Vis = Continuous,
    "Tail" = Discrete,
};
'

# 样例清单只算一次，所有 case 共用
SAMPLES_FILE="$OUT_ROOT/_samples.txt"
(cd "$SRC" && find . -iname '*.reanim' | sed 's|^\./||' | sort) > "$SAMPLES_FILE"

do_case() {
    local name="$1" extra="$2"
    local casedir="$OUT_ROOT/$name"
    mkdir -p "$casedir/input" "$casedir/outdir"

    # 程序只读输入的 .reanim（贴图路径只作为字符串写出），所以只复制这些
    while IFS= read -r f; do
        mkdir -p "$casedir/input/$(dirname "$f")"
        cp "$SRC/$f" "$casedir/input/$f"
    done < "$SAMPLES_FILE"

    printf '%s' "$CONFIG_BODY" > "$casedir/case.cfg"
    : > "$casedir/stdout.log"
    : > "$casedir/exit.log"

    while IFS= read -r sample; do
        # shellcheck disable=SC2086
        ( cd "$casedir" && "$EXE" "input/$sample" "res://anim/" "res://art/" \
            ${extra//OUTDIR/outdir/} ${extra//CONFIG/case.cfg} \
            >> stdout.log 2>&1 )
        printf '%s %d\n' "$sample" "$?" >> "$casedir/exit.log"
    done < "$SAMPLES_FILE"

    local n_out n_bad
    n_out=$(find "$casedir" -type f \( -name '*.tres' -o -name '*.tscn' \) | wc -l)
    n_bad=$(grep -vc ' 0$' "$casedir/exit.log" || true)
    printf '%-26s 产物 %4d 个, 非零退出 %d 个\n' "$name" "$n_out" "$n_bad" >> "$OUT_ROOT/_summary.txt"
}

: > "$OUT_ROOT/_summary.txt"

running=0
for entry in "${CASES[@]}"; do
    do_case "${entry%%|*}" "${entry#*|}" &
    running=$((running + 1))
    if [ "$running" -ge "$MAXJOBS" ]; then
        wait -n
        running=$((running - 1))
    fi
done
wait

echo "== exe:        $EXE"
echo "== exe sha256: $(sha256sum "$EXE" | cut -d' ' -f1)"
echo "== 素材:       $SRC"
echo "== 并行度:     $MAXJOBS"
printf '%s  %s\n' "$(sha256sum "$EXE" | cut -d' ' -f1)" "$EXE" > "$OUT_ROOT/_exe.sha256"
sort "$OUT_ROOT/_summary.txt"
rm -f "$OUT_ROOT/_samples.txt" "$OUT_ROOT/_summary.txt" 2>/dev/null || true

echo
echo "RUN_DIR=$OUT_ROOT"
echo "比对: $SCRIPT_DIR/diff_matrix.sh <基准RUN_DIR> <对比RUN_DIR>"
