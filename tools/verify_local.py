#!/usr/bin/env python3
"""test/<階層>/verify/ のコードを Library Checker のテストケースで手元で検証する。

問題のテストケースは library-checker-problems から生成する。初回だけ用意する。

    cd ..            # cp-library の 1 つ上（atcoder/）
    git clone --depth 1 https://github.com/yosupo06/library-checker-problems.git
    pip install colorlog

使い方:

    python tools/verify_local.py test/<階層>/verify/foo.cpp
    python tools/verify_local.py test/<階層>/verify/foo.cpp --regen   # ケースを作り直す

問題名はソース冒頭の judge.yosupo.jp の URL から拾う。ケースが無ければ
generate.py を呼んで作る。ビルドは bundle 後の 1 ファイルで行い、提出するものと
同じ状態を確かめる。
"""

import argparse
import io
import os
import re
import shutil
import subprocess
import sys
import time

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
# 既定は cp-library の 1 つ上（atcoder/）の隣に置いてある前提。
# 別の場所なら環境変数で指す
_SIBLING = os.path.dirname(ROOT)
DEFAULT_PROBLEMS = os.environ.get(
    'LIBRARY_CHECKER_PROBLEMS', os.path.join(_SIBLING, 'library-checker-problems'))
DEFAULT_ACL = os.environ.get('ACL_PATH', os.path.join(_SIBLING, 'acl'))

URL = re.compile(r'judge\.yosupo\.jp/problem/([A-Za-z0-9_]+)')


def find_problem_dir(problems, name):
    for d in sorted(os.listdir(problems)):
        p = os.path.join(problems, d, name)
        if os.path.isfile(os.path.join(p, 'info.toml')):
            return p
    sys.exit('問題が見つからない: %s（%s の下）' % (name, problems))


def run(cmd, **kw):
    return subprocess.run(cmd, **kw)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('src', help='test/<階層>/verify/ のソース')
    ap.add_argument('--problems', default=DEFAULT_PROBLEMS)
    ap.add_argument('--acl', default=DEFAULT_ACL)
    ap.add_argument('--regen', action='store_true', help='ケースを作り直す')
    ap.add_argument('--timeout', type=float, default=60.0,
                    help='1 ケースの打ち切り秒数')
    args = ap.parse_args()

    src = os.path.abspath(args.src)
    text = io.open(src, encoding='utf-8').read()
    m = URL.search(text)
    if not m:
        sys.exit('ソースに judge.yosupo.jp の URL が無い: %s' % src)
    name = m.group(1)
    print('問題: %s' % name)

    pdir = find_problem_dir(args.problems, name)
    indir = os.path.join(pdir, 'in')
    if args.regen or not os.path.isdir(indir) or not os.listdir(indir):
        print('テストケースを生成する（数分かかることがある）')
        r = run([sys.executable, 'generate.py', '-p', name], cwd=args.problems)
        if r.returncode:
            sys.exit('generate.py が失敗した')

    # 提出するのと同じ 1 ファイルにしてからビルドする
    work = os.path.join(ROOT, '.verify_tmp')
    os.makedirs(work, exist_ok=True)
    bundled = os.path.join(work, name + '.cpp')
    r = run([sys.executable, os.path.join(ROOT, 'tools', 'bundle.py'), src,
             '-o', bundled])
    if r.returncode:
        sys.exit('bundle.py が失敗した')

    exe = os.path.join(work, name + ('.exe' if os.name == 'nt' else ''))
    r = run(['g++', '-std=gnu++20', '-O2', '-Wall', '-Wextra',
             '-I' + args.acl, bundled, '-o', exe])
    if r.returncode:
        sys.exit('ビルドが失敗した')

    checker = os.path.join(pdir, 'checker' + ('.exe' if os.name == 'nt' else ''))
    cases = sorted(f[:-3] for f in os.listdir(indir) if f.endswith('.in'))
    print('%d ケース' % len(cases))

    ng, worst, worst_name = 0, 0.0, ''
    for c in cases:
        fin = os.path.join(indir, c + '.in')
        fexp = os.path.join(pdir, 'out', c + '.out')
        fgot = os.path.join(work, c + '.out')
        st = time.time()
        try:
            with io.open(fin, 'rb') as i, io.open(fgot, 'wb') as o:
                r = run([exe], stdin=i, stdout=o, timeout=args.timeout)
        except subprocess.TimeoutExpired:
            print('  TLE  %-28s %.0f 秒で打ち切り' % (c, args.timeout))
            ng += 1
            continue
        el = time.time() - st
        if el > worst:
            worst, worst_name = el, c
        if r.returncode:
            print('  RE   %-28s exit=%d' % (c, r.returncode))
            ng += 1
            continue
        # testlib は input / 自分の出力 / 正解 の順
        v = run([checker, fin, fgot, fexp], capture_output=True, text=True)
        if v.returncode:
            print('  WA   %-28s %s' % (c, (v.stderr or v.stdout).strip()[:60]))
            ng += 1
        else:
            print('  AC   %-28s %6.0f ms' % (c, el * 1000))

    print()
    print('最遅: %s %.0f ms' % (worst_name, worst * 1000))
    if ng:
        print('NG %d 件' % ng)
    else:
        print('全 %d ケース AC' % len(cases))
    if ng:
        print('出力は %s に残してある' % work)
        return 1
    # 直前に動かした exe のハンドルが残っていることがあるので少し待って試す
    for _ in range(5):
        shutil.rmtree(work, ignore_errors=True)
        if not os.path.isdir(work):
            break
        time.sleep(0.2)
    else:
        print('片付けられなかった: %s' % work)
    return 0


if __name__ == '__main__':
    sys.exit(main())
