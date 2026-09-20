#!/usr/bin/env python3
"""main.cpp の「ライブラリ貼り付け欄」を空にする。

前の問題で貼ったライブラリを、区切り線を残したまま消す。

    python tools/clear_paste.py                 # カレントの main.cpp
    python tools/clear_paste.py path/to/main.cpp
    python tools/clear_paste.py --dry-run       # 消さずに行数だけ出す

「ライブラリ貼り付け欄」の区切り行から「解答」の区切り行までを落とす。
区切り行そのものは残すので、続けてスニペット（cp_...）を貼れる。
"""

import argparse
import io
import re
import sys

BEGIN = re.compile(r'^/\* =+ ライブラリ貼り付け欄 =+ \*/\s*$')
END = re.compile(r'^/\* =+ 解答 =+ \*/\s*$')


def clear(lines):
    """(新しい行たち, 消した行数) を返す。"""
    b = e = -1
    for i, s in enumerate(lines):
        if b < 0 and BEGIN.match(s):
            b = i
        elif b >= 0 and END.match(s):
            e = i
            break
    if b < 0:
        sys.exit('「ライブラリ貼り付け欄」の区切り行が見つからない')
    if e < 0:
        sys.exit('「解答」の区切り行が見つからない')
    return lines[:b + 1] + [''] + lines[e:], e - b - 1


def main():
    p = argparse.ArgumentParser()
    p.add_argument('path', nargs='?', default='main.cpp')
    p.add_argument('--dry-run', action='store_true')
    a = p.parse_args()

    lines = io.open(a.path, encoding='utf-8').read().split('\n')
    out, n = clear(lines)
    if out == lines:  # 既に「区切り・空行・区切り」の形
        print('%s: 貼り付け欄は既に空' % a.path)
        return
    if a.dry_run:
        print('%s: %d 行消せる' % (a.path, n))
        return
    io.open(a.path, 'w', encoding='utf-8', newline='\n').write('\n'.join(out))
    print('%s: %d 行消した' % (a.path, n))


main()
