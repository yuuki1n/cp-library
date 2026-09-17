#!/usr/bin/env python3
"""lib/*.hpp から VS Code のスニペット（cpp.json）を作る。

ライブラリを直した後に実行し直す。既定の出力先は VS Code のユーザースニペット。

    python tools/gen_snippets.py            # 既定の場所に書く
    python tools/gen_snippets.py --out x.json
    python tools/gen_snippets.py --no-doc   # ファイル冒頭の説明コメントを落とす
    python tools/gen_snippets.py --dry-run  # 書かずに一覧だけ出す

エディタでは `cp_dijkstra` と打って候補から選ぶ。`cp` まで打てば一覧になる。
main.cpp は bits/stdc++.h と atcoder/all を読んでいるので、#include 行は落とす。
"""

import argparse
import io
import json
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SKIP_DIRS = {'test', '.git', '.github', 'tools'}

# 単体では足りないヘッダの依存。値のヘッダを先に貼る（順序も依存順）
DEPS = {
    'dijkstra': ['graph'],
}


def collect():
    """{stem: (相対パス, 本文)} を返す。"""
    out = {}
    for d, dirs, files in os.walk(ROOT):
        dirs[:] = [x for x in dirs if x not in SKIP_DIRS and not x.startswith('.')]
        for f in sorted(files):
            if not f.endswith('.hpp'):
                continue
            path = os.path.join(d, f)
            rel = os.path.relpath(path, ROOT).replace('\\', '/')
            stem = f[:-4]
            if stem in out:
                sys.exit('ファイル名が重複している: %s と %s' % (out[stem][0], rel))
            out[stem] = (rel, io.open(path, encoding='utf-8').read())
    return out


def strip_includes(src):
    """先頭の #include 群と、それに続く空行を落とす。"""
    lines = src.splitlines()
    i = 0
    while i < len(lines) and (lines[i].startswith('#include') or not lines[i].strip()):
        i += 1
    return '\n'.join(lines[i:]).rstrip('\n')


# 行頭から始まる最初のブロックコメントを説明とみなす。internal 名前空間が
# 前に来るファイルがあるので、先頭決め打ちにはしない
DOC_RE = re.compile(r'^/\*.*?\*/\n', re.S | re.M)


def strip_doc(src):
    return DOC_RE.sub('', src, count=1).lstrip('\n')


def summary(src):
    """説明コメントの 1 行目を description にする。"""
    m = DOC_RE.search(src)
    if not m:
        return ''
    for line in m.group(0).splitlines()[1:]:
        s = line.lstrip().lstrip('*').strip()
        if s:
            return s
    return ''


def escape(line):
    r"""VS Code のスニペット本文では \ と $ が特殊。} はそのままで通る。"""
    return line.replace('\\', '\\\\').replace('$', '\\$')


def default_out():
    if sys.platform == 'win32':
        base = os.environ.get('APPDATA')
        if base:
            return os.path.join(base, 'Code', 'User', 'snippets', 'cpp.json')
    elif sys.platform == 'darwin':
        return os.path.expanduser(
            '~/Library/Application Support/Code/User/snippets/cpp.json')
    return os.path.expanduser('~/.config/Code/User/snippets/cpp.json')


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--out', default=default_out())
    ap.add_argument('--no-doc', action='store_true',
                    help='ファイル冒頭の説明コメントを落とす')
    ap.add_argument('--dry-run', action='store_true')
    args = ap.parse_args()

    headers = collect()
    for stem, deps in DEPS.items():
        if stem not in headers:
            sys.exit('DEPS の %s がライブラリに無い' % stem)
        for d in deps:
            if d not in headers:
                sys.exit('DEPS[%s] の %s がライブラリに無い' % (stem, d))

    snippets = {}
    rows = []
    for stem in sorted(headers):
        rel, src = headers[stem]
        desc = summary(strip_includes(src))

        body = []
        for name in DEPS.get(stem, []) + [stem]:
            part = strip_includes(headers[name][1])
            if args.no_doc:
                part = strip_doc(part)
            if body:
                body.append('')
            body.extend(escape(x) for x in part.splitlines())
        body.append('$0')

        snippets[stem] = {
            'prefix': 'cp_' + stem,
            'body': body,
            'description': '%s  [%s]' % (desc or stem, rel),
        }
        rows.append((stem, len(body), rel, DEPS.get(stem, [])))

    text = json.dumps(snippets, ensure_ascii=False, indent=2) + '\n'

    w = max(len(r[0]) for r in rows)
    for stem, n, rel, deps in rows:
        tail = '  +%s' % ','.join(deps) if deps else ''
        print('  %-*s %4d行  %s%s' % (w, stem, n, rel, tail))
    print()
    print('%d 件 / %.1f KB' % (len(rows), len(text.encode('utf-8')) / 1024))

    if args.dry_run:
        print('--dry-run のため書き込みはしない')
        return
    os.makedirs(os.path.dirname(args.out), exist_ok=True)
    io.open(args.out, 'w', encoding='utf-8', newline='\n').write(text)
    print('-> %s' % args.out)


if __name__ == '__main__':
    main()
