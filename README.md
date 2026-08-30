# cp-library

競技プログラミング用のデータ構造・アルゴリズム集。ヘッダオンリー。

## ディレクトリ構成

| ディレクトリ | 内容 |
|---|---|
| `ds/` | データ構造（セグメント木、BIT、Union-Find、遅延セグ木 など） |
| `graph/` | グラフ（BFS/DFS、ダイクストラ、SCC、最小全域木、最大流 など） |
| `math/` | 数学（modint、組合せ、素数、行列 など） |
| `string/` | 文字列（Z-algorithm、ローリングハッシュ、Suffix Array など） |
| `geometry/` | 幾何（点・線分・円、凸包 など） |
| `util/` | 汎用ユーティリティ（座標圧縮、二分探索 など） |
| `test/` | 検証用コード |

## 使い方

問題を解くディレクトリ（`atcoder/algo`）からは相対パス、またはインクルードパス指定で参照する。

```cpp
#include "../lib/ds/segtree.hpp"
```

```
g++ -std=gnu++20 -O2 -I D:/OneDrive/workspace/atcoder/lib main.cpp
```
```cpp
#include <ds/segtree.hpp>
```

AtCoder へ提出するときはヘッダの中身を貼り付ける（1 ファイルにまとめる必要があるため）。

## 実装の約束ごと

- **ヘッダオンリー**。`.hpp` 1 ファイルで完結させ、`#pragma once` を先頭に置く
- `bits/stdc++.h` と `using namespace std;` は各ヘッダでは書かない。呼び出し側（テンプレート）で済ませる前提とし、必要な標準ヘッダだけを個別に include する
- 1 ファイル 1 データ構造。依存する場合は相対パスで include する
- ファイル冒頭にコメントで **概要・計算量・使用例・verify した問題の URL** を書く
- 命名は `snake_case`。テンプレート本体のマクロ（`rep` / `all` など）には依存しない

## verify

`test/` 配下に、AtCoder や Library Checker の問題を解くコードを置く。ヘッダ冒頭のコメントに verify 済みの問題 URL を必ず残す。
