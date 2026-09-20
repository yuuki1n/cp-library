#!/bin/sh
# スニペットを作り直す本体。post-merge と post-commit から呼ぶ。
# 呼び出し側の処理は止めない（どちらのフックも終了値を git が見ない）。

who=${1:-hook}

root=$(git rev-parse --show-toplevel) || exit 0
cd "$root" || exit 0

if ! command -v python >/dev/null 2>&1; then
  echo "$who: python が見つからないのでスニペット生成をとばした" >&2
  exit 0
fi

python tools/gen_snippets.py || echo "$who: スニペット生成に失敗した" >&2
exit 0
