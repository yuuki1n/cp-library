#!/usr/bin/env python3
"""提出用に 1 ファイルへまとめる。

`#include "..."`（引用符のもの）をファイルの中身で置き換える。
`#include <...>`（山括弧）は標準ヘッダと ACL なので触らない。

    python tools/bundle.py test/<階層>/verify/foo.cpp            # クリップボードへ
    python tools/bundle.py test/<階層>/verify/foo.cpp --stdout    # 標準出力へ
    python tools/bundle.py test/<階層>/verify/foo.cpp -o out.cpp  # ファイルへ

Library Checker も AtCoder も 1 ファイルしか受け取らないので、提出前に通す。
まとめたものはリポジトリに残さない。提出のたびに作り直してそのまま貼る。
"""

import argparse
import io
import os
import re
import subprocess
import sys
import tempfile

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


def to_clipboard(text):
    """クリップボードへ入れる。入らなければ False を返す"""
    if sys.platform == 'win32':
        # clip.exe は入力をコンソールのコードページで読むので日本語が化ける。
        # UTF-8 のファイルを経由して PowerShell に渡すと BOM も付かない
        fd, path = tempfile.mkstemp(suffix='.txt')
        os.close(fd)
        try:
            io.open(path, 'w', encoding='utf-8', newline='\n').write(text)
            cmd = "Set-Clipboard -Value (Get-Content -Raw -Encoding UTF8 -LiteralPath '%s')" % path
            subprocess.run(['powershell', '-NoProfile', '-Command', cmd], check=True)
            return True
        except (OSError, subprocess.SubprocessError):
            return False
        finally:
            os.remove(path)

    cmds = [['pbcopy']] if sys.platform == 'darwin' else [['wl-copy'], ['xclip', '-selection', 'clipboard'], ['xsel', '-ib']]
    for c in cmds:
        try:
            subprocess.run(c, input=text.encode('utf-8'), check=True)
            return True
        except (OSError, subprocess.SubprocessError):
            continue
    return False


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('src')
    ap.add_argument('-o', '--out', help='ファイルへ書き出す')
    ap.add_argument('--stdout', action='store_true', help='標準出力へ流す')
    args = ap.parse_args()

    body = '\n'.join(expand(args.src, set(), [])) + '\n'
    n = body.count('\n')
    if args.out:
        io.open(args.out, 'w', encoding='utf-8', newline='\n').write(body)
        print('%d 行 -> %s' % (n, args.out), file=sys.stderr)
    elif args.stdout:
        sys.stdout.write(body)
    elif to_clipboard(body):
        print('%d 行をクリップボードにコピーした。そのまま提出できる' % n, file=sys.stderr)
    else:
        print('クリップボードに入れられなかったので標準出力へ流す', file=sys.stderr)
        sys.stdout.write(body)


if __name__ == '__main__':
    main()
