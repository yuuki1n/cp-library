#!/usr/bin/env python3
"""提出用に 1 ファイルへまとめる。

`#include "..."`（引用符のもの）をファイルの中身で置き換える。
`#include <...>`（山括弧）は標準ヘッダと ACL なので触らない。

    python tools/bundle.py test/verify/foo.cpp            # 標準出力へ
    python tools/bundle.py test/verify/foo.cpp -o out.cpp

Library Checker も AtCoder も 1 ファイルしか受け取らないので、提出前に通す。
"""

import argparse
import io
import os
import re
import sys

INC = re.compile(r'^\s*#\s*include\s*"([^"]+)"\s*$')


def expand(path, seen, stack):
    path = os.path.normpath(path)
    if path in stack:
        sys.exit('include が循環している: %s' % ' -> '.join(stack + [path]))
    if path in seen:
        return ['// (すでに展開済み: %s)' % os.path.basename(path)]
    seen.add(path)

    if not os.path.exists(path):
        sys.exit('見つからない: %s' % path)
    out = []
    for line in io.open(path, encoding='utf-8').read().splitlines():
        m = INC.match(line)
        if not m:
            out.append(line)
            continue
        child = os.path.join(os.path.dirname(path), m.group(1))
        rel = os.path.normpath(child).replace('\\', '/')
        out.append('/* ---- ここから %s ---- */' % os.path.basename(rel))
        out.extend(expand(child, seen, stack + [path]))
        out.append('/* ---- ここまで %s ---- */' % os.path.basename(rel))
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('src')
    ap.add_argument('-o', '--out')
    args = ap.parse_args()

    body = '\n'.join(expand(args.src, set(), [])) + '\n'
    if args.out:
        io.open(args.out, 'w', encoding='utf-8', newline='\n').write(body)
        print('%d 行 -> %s' % (body.count('\n'), args.out), file=sys.stderr)
    else:
        sys.stdout.write(body)


if __name__ == '__main__':
    main()
