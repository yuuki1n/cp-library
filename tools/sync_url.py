#!/usr/bin/env python3
"""各 .hpp に、そのファイル自身への GitHub URL をコメントで入れる。

貼り付けたコードから元のファイルへ飛べるようにするため。提出したコードを
後から見たときに、どのライブラリのどのファイルを使ったのかを辿れる。

    python tools/sync_url.py            # 入れる / ずれていれば直す
    python tools/sync_url.py --check    # 直さずに、ずれているものを列挙して終わる

URL 行は #include 群の直後、説明コメントの前に置く。
gen_snippets.py の strip_includes が「先頭の #include と空行」で止まるので、
ファイルの先頭に置くと #include が落とされなくなってしまう。
何度かけても同じ形になる。ファイルを移したり名前を変えたら流し直すこと。
"""

import argparse
import io
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SKIP_DIRS = {'test', '.git', '.github', 'tools'}
BASE = 'https://github.com/yuuki1n/cp-library/blob/main/'
URL_RE = re.compile('^// ' + re.escape(BASE) + r'\S*\n', re.M)


def targets():
    """lib 直下の .hpp を全部返す。"""
    for d, dirs, files in os.walk(ROOT):
        dirs[:] = [x for x in dirs if x not in SKIP_DIRS and not x.startswith('.')]
        for f in sorted(files):
            if f.endswith('.hpp'):
                yield os.path.join(d, f)


def fixed(src, url):
    """URL 行を入れた中身を返す。何度かけても同じ形になる。"""
    src = URL_RE.sub('', src)  # 古い URL 行をいったん全部落とす
    lines = src.splitlines(True)
    i = 0
    while i < len(lines) and (lines[i].startswith('#include') or not lines[i].strip()):
        i += 1
    # head は #include 群だけ（末尾の空行は含めない）、残りは最初の中身の行から
    k = i
    while k > 0 and not lines[k - 1].strip():
        k -= 1
    head = ''.join(lines[:k])
    return (head + '\n' if head else '') + '// ' + url + '\n\n' + ''.join(lines[i:])


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--check', action='store_true', help='直さずに、ずれているものを列挙する')
    args = ap.parse_args()

    bad = 0
    for path in targets():
        rel = os.path.relpath(path, ROOT).replace(os.sep, '/')
        url = BASE + rel
        src = io.open(path, encoding='utf-8').read()
        new = fixed(src, url)
        if new == src:
            continue
        bad += 1
        if args.check:
            print('  ずれている: %s' % rel)
        else:
            io.open(path, 'w', encoding='utf-8', newline='\n').write(new)
            print('  %s' % url)

    if args.check:
        print('%d 件ずれている' % bad)
        sys.exit(1 if bad else 0)
    print('%d 件直した' % bad)


if __name__ == '__main__':
    main()
