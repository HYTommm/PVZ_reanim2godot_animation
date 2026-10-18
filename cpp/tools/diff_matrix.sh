#!/usr/bin/env bash
# 逐字节比对两份 run_matrix.sh 输出（含 CRLF 行尾，不做任何归一化）。
#
# 用法: diff_matrix.sh <基准RUN_DIR> <对比RUN_DIR>
#
# 用单进程的 `diff -rq` 扫整棵树（比逐文件起 cmp 快得多），
# 只在确有差异时再对具体文件跑 cmp -l 给出首个不同的字节偏移。

set -u

A="${1:?用法: diff_matrix.sh <基准RUN_DIR> <对比RUN_DIR>}"
B="${2:?用法: diff_matrix.sh <对比RUN_DIR>}"

tmp="$(mktemp)"

total_cases=0
bad_cases=0
ok_cases=0

for casedir in "$A"/*/; do
    name="$(basename "$casedir")"
    other="$B/$name"
    total_cases=$((total_cases + 1))

    if [ ! -d "$other" ]; then
        echo "[$name] 对比侧缺失"
        bad_cases=$((bad_cases + 1))
        continue
    fi

    # _exe.sha256 记录的是被跑的那个 exe 的哈希，两侧本就不同，排除
    if diff -rq --exclude=_exe.sha256 "$casedir" "$other" > "$tmp" 2>&1; then
        echo "[$name] OK"
        ok_cases=$((ok_cases + 1))
        continue
    fi

    bad_cases=$((bad_cases + 1))
    echo "[$name] 差异:"
    while IFS= read -r line; do
        echo "    $line"
        # "Files A and B differ" -> 打印首个不同字节
        case "$line" in
            Files*differ)
                fa="${line#Files }"; fa="${fa%% and *}"
                fb="${line##* and }"; fb="${fb% differ}"
                pos=$(cmp -l "$fa" "$fb" 2>/dev/null | head -1 | awk '{print $1}')
                if [ -n "${pos:-}" ]; then
                    echo "        首个不同字节（1-based 偏移）: $pos"
                    echo "        基准: $(dd if="$fa" bs=1 skip=$((pos - 40)) count=80 2>/dev/null | tr -d '\n\r' | head -c 80)"
                    echo "        对比: $(dd if="$fb" bs=1 skip=$((pos - 40)) count=80 2>/dev/null | tr -d '\n\r' | head -c 80)"
                fi
                ;;
        esac
    done < "$tmp"
    echo "    （以上为该项全部差异，最多 $(wc -l < "$tmp") 处）"
done

rm -f "$tmp"

echo
echo "case 数: $total_cases, 一致: $ok_cases, 有差异: $bad_cases"

# 反向检查对比侧是否多出 case
while IFS= read -r d; do
    n="$(basename "$d")"
    [ -d "$A/$n" ] || { echo "[$n] 只在对比侧存在"; bad_cases=$((bad_cases + 1)); }
done < <(find "$B" -maxdepth 1 -mindepth 1 -type d | sort)

[ "$bad_cases" -eq 0 ]
