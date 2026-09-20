# cp-library

競技プログラミング用のデータ構造・アルゴリズム集。ヘッダオンリー。

Java 版ライブラリからの移植状況は [PORTING.md](PORTING.md)、
ジャッジで検証した記録は [VERIFY.md](VERIFY.md) を参照。

## ディレクトリ構成

| ディレクトリ | 内容 |
|---|---|
| `data_structure/` | データ構造（セグメント木、BIT、累積和、区間集合 など） |
| `graph/` | グラフ（BFS/DFS、ダイクストラ、SCC、最小全域木、最大流 など） |
| `graph/union_find/` | Union-Find（素集合データ構造）の各種 |
| `math/` | 数学（modint、組合せ、素数、行列 など） |
| `string/` | 文字列（Z-algorithm、ローリングハッシュ、Suffix Array など） |
| `geometry/` | 幾何（点・線分・円、凸包 など） |
| `util/` | 汎用ユーティリティ（座標圧縮、二分探索 など） |
| `test/` | 検証用コード |
| `tools/` | 補助スクリプト |

## 使い方

**include はしない。** 使うときは `.hpp` からクラス本体を `atcoder/algo/main.cpp` の
「ライブラリ貼り付け欄」へコピーする。AtCoder は 1 ファイルでの提出なので、
ローカルでも提出時と同じ形で動かすため。

貼るときに落とすもの:

- `#include <...>`（テンプレート冒頭の `bits/stdc++.h` で足りる）

`std::` 修飾は付いたままで動くので、消さなくてよい。
`#pragma once` は置いていないので落とす必要はない。

## VS Code のスニペット

`tools/gen_snippets.py` で、各ヘッダをそのまま挿入するスニペットを作る。

```
python tools/gen_snippets.py            # VS Code のユーザースニペットに書く
python tools/gen_snippets.py --dry-run  # 書かずに一覧だけ出す
python tools/gen_snippets.py --no-doc   # 説明コメントを落として貼る
```

エディタでは `cp_dijkstra` と打って候補から選ぶ（`cp` まで打てば一覧になる）。
`#include` 行は落として挿入する。単体では足りないもの（`dijkstra` に対する
`graph`）はスクリプト内の `DEPS` に書いてあり、依存ぶんも一緒に挿入される。

**ライブラリを直したら実行し直す。** 生成物は VS Code 側に置かれるので
このリポジトリには入らない。

## 実装の約束ごと

- **ヘッダオンリー**。`.hpp` 1 ファイルで完結させる
- **`#pragma once` は書かない**。貼り付け運用なので不要で、main.cpp に貼ると
  `-Wpragma-once-outside-header` の警告が出るため
- `bits/stdc++.h` と `using namespace std;` は各ヘッダでは書かない。呼び出し側（テンプレート）で済ませる前提とし、必要な標準ヘッダだけを個別に include する
- **ヘッダ同士を include しない**。`#pragma once` が無いため二重定義になる。
  依存する処理は各ファイルに書くか、貼る側で両方貼る
- 1 ファイル 1 データ構造
- 内部実装の補助関数は `<機能名>_internal` 名前空間に入れる。
  複数のライブラリを同じ main.cpp に貼ったときの再定義を避けるため
- ファイル冒頭にコメントで **概要・API 一覧・計算量・使用例・verify** を書く。
  形は「概要 1 行 → API 一覧（1 行 1 個） → 注意点 → 使用例 → verify」。
  **設計の理由や背景はコメントに書かない**（コミットメッセージか会話に残す）。
  使用例は最小限。全メソッドを並べ直さない
- **命名は ACL に合わせて `snake_case`**。型名も関数名もファイル名も。
  `union_find` と `UnionFind` が同じコードに並ぶのを避けるため。1 ファイル 1 データ構造なので、
  **型名とファイル名は一致させる**（`segtree.hpp` の `segtree` と同じ形）
- 名前が ACL とぶつかるときは別の語を使う。ACL と同名の型を貼ると
  `using namespace atcoder;` のせいで参照が曖昧になりコンパイルが通らない
  （`atcoder::dsu` に対して `union_find`）
- ACL に同じ役割のメソッドがあれば名前を借りる（`prod` / `apply` / `leader` / `merge` など）
- **演算は ACL 流に関数ポインタのテンプレート引数で受ける**
  （`segtree<S, op, e>` と同じ形）。関数オブジェクトの型引数にすると、
  演算が 3 個以上のときに型引数とコンストラクタ引数が並んで書きにくい。
  よく使う演算は `<機能名>_internal` に置いて既定テンプレート引数にし、
  和で使うだけなら型引数を省けるようにする
- テンプレート本体のマクロ（`rep` / `all` など）には依存しない

## 異常な入力の扱い

場当たりにならないよう、次の 4 つで揃える。

- **「空」を意味する引数は無害に処理して自然な値を返す**。`l >= r` の区間、要素 0 個や
  1 個の列など。呼び出し側で `if` を書かなくて済むようにするため
  （`interval_set::insert(l, r)` は `0` を返す、`inversion_count` は `0` を返す）
- **範囲外の添字は守らない**。丸めて隠すと答えだけが静かに間違い、原因が見えなくなる。
  素直に落ちて `-D_GLIBCXX_DEBUG` で場所が分かるほうが早い
  （`cumsum(l, r)` は範囲を検査しない）
- **引数からは判断できない「状態への誤操作」はガードする**。呼び出し側が状態を
  目で追うしかないものは無害化する
  （`rollback_union_find::undo()` は履歴が空なら何もしない）
- **事前条件は `assert` で書く**。AtCoder は `NDEBUG` を定義しないので本番でも効く
  （`relational_union_find::diff(u, v)` は `same(u, v)` を要求する）

`clear()` は**構築直後の状態に戻す**。可変長のものは空にし、大きさが固定のものは
大きさを保ったまま初期状態に戻す。

## CI

`push` のたびに GitHub Actions で `test/` 配下を全部ビルドして実行する
（`.github/workflows/test.yml`）。

- 通常ビルド（`-O2 -Wall -Wextra`）
- `_GLIBCXX_DEBUG` ビルド（範囲外アクセスとイテレータの誤用を拾う）
- 厳しめの警告は参考表示のみ。手元とジャッジで出方が違うので失敗にはしない

テストは失敗したら 0 以外で終了すること。`test/` の各ファイルは
`*_test.cpp` という名前にする（CI が glob で拾う）。

速度計測は `#ifdef _GLIBCXX_DEBUG` で囲んで省略する。デバッグビルドで
測った時間に意味がなく、CI が無駄に長くなるため。

## verify

`test/verify/` に、Library Checker や AtCoder に提出するコードを置く。
ジャッジに投げて AC を取ったら、ヘッダ冒頭のコメントの `verify:` 欄に問題 URL を
書き、`PORTING.md` の verify 表も「済」にする。

提出用コードは実行に入力が要るので CI では走らせず、ビルドだけ確かめている
（ライブラリを直したときに壊れていないかを見るため）。

**提出する前に 1 ファイルにまとめる。** ジャッジは 1 ファイルしか受け取らないので、
`#include "../../..."` を残したまま出すとコンパイルエラーになる。

```
python tools/bundle.py test/verify/foo.cpp -o test/verify/submit/foo.cpp
```

`test/verify/submit/` は生成物なので `.gitignore` に入れてある。古いものを提出
しないよう、提出のたびに作り直す。

**提出する前に手元で回す。** `tools/verify_local.py` が、本物のテストケースで
全ケースを流して判定まで出す。bundle してからビルドするので、相対 include が
残っていれば（＝提出したら CE になる状態なら）ここで分かる。

```
python tools/verify_local.py test/verify/foo.cpp
```

初回だけテストケースの生成元を用意する。cp-library の 1 つ上に置けば引数は要らない。

```
cd ..            # atcoder/
git clone --depth 1 https://github.com/yosupo06/library-checker-problems.git
pip install colorlog
```

生成したケースは問題ごとに数百 MB になる。要らなくなったら `in/` と `out/` を
消してよい（`--regen` で作り直せる）。
