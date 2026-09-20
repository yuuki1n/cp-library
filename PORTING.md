# 移植状況

Java 版の資産を C++ に移し替えていく。このファイルはコミット・プッシュのたびに更新する。

移植元は 2 か所ある。

1. ライブラリディレクトリ `C:\pleiades\workspace\library\src\main\java\library`（58 ファイル）
2. Eclipse のテンプレート（スニペット）
   `C:\pleiades\workspace\.metadata\.plugins\org.eclipse.core.runtime\.settings\org.eclipse.jdt.ui.prefs`
   の `org.eclipse.jdt.ui.text.custom_templates` キー。全 212 件のうち `java` context が 90 件で、
   その大半は 1 のファイルと同じもの。**1 に無いものが 19 件**あり、それを母数に加えている
   （Eclipse 標準のテンプレートと、デバッグ用の `log` は数えない）

移植の進み具合と verify の進み具合は別物なので、それぞれ分けて数えている。

- **移植済み** 17 / 77（うち verify 済み **0**）
- **移植不要** 29 / 77（ACL 16・標準ライブラリ 10・対象外 3）
- **残り** 31 / 77（高 7・中 16・低 8）

---

## 移植済み

| Java | C++ | verify | 備考 |
|---|---|---|---|
| `dataStructure/rangeData/PrefixSum` | `data_structure/cumsum.hpp` | 未 | |
| `dataStructure/rectangleData/Sum2D` | `data_structure/cumsum2d.hpp` | 未 | |
| `dataStructure/rangeData/RangeSet` | `data_structure/interval_set.hpp` | 未 | 名前を `IntervalSet` に変更 |
| `math/Prime` | `math/prime.hpp` | 未 | SPF 篩 + Pollard's rho |
| `graph/unionfind/UnionFind` | `graph/union_find/union_find.hpp` | 未 | `getGroup` にあたる `group(x)` を持つ。ACL の `dsu` には無い |
| `graph/unionfind/MonoidUnionFind` | `graph/union_find/monoid_union_find.hpp` | 未 | |
| `graph/unionfind/RelationalUnionFind` | `graph/union_find/relational_union_find.hpp` | 未 | |
| `graph/unionfind/RollbackUnionFind` | `graph/union_find/rollback_union_find.hpp` | 未 | |
| `graph/unionfind/DynamicUnionFind` | `graph/union_find/dynamic_union_find.hpp` | 未 | |
| `graph/Edge` `graph/Graph` | `graph/graph.hpp` | 未 | 隣接リスト。2 ファイルを 1 つにまとめた |
| `graph/Dijkstra` | `graph/dijkstra.hpp` | 未 | `graph.hpp` と組で使う |
| `dataStructure/Trie` | `string/trie.hpp` | 未 | 文字列のトライ。接頭辞の本数を数える |
| `dataStructure/BinaryTrie` | `data_structure/binary_trie.hpp` | 未 | xor 最小 / k 番目。xor_all は O(1) |
| スニペット `invCnt` | `util/inversion_count.hpp` | 未 | 転倒数。マージソート版 |
| スニペット `rle` | `util/rle.hpp` | 未 | ランレングス圧縮。`rle_decode` も持つ |
| スニペット `Matrix` | `math/matrix.hpp` | 未 | Java 版は mod 積と `pow` だけ。C++ 版は `det` / `rank` / `inv` / `solve` を足した上位互換 |

Java に無い追加分（未 verify）:

| C++ | 内容 |
|---|---|
| `math/maxplus_matrix.hpp` | (max, +) 半環の行列。k 辺での最長路など |

---

## 移植不要

### ACL にあるもの

| Java | ACL |
|---|---|
| `dataStructure/rangeData/BIT` | `atcoder::fenwick_tree` |
| `dataStructure/rangeData/DualBIT` | `atcoder::fenwick_tree` + 差分配列 |
| `.../segmentTree/SegmentTree` `Seg` `SegmentTreeLong` `SegLong` | `atcoder::segtree` |
| `.../segmentTree/LazySegmentTree` `LazySegmentTreeLong` | `atcoder::lazy_segtree` |
| `.../segmentTree/DualSegmentTree` `DualSegmentTreeLong` | `atcoder::lazy_segtree`（取得を 1 点に） |
| `graph/MaxFlow` | `atcoder::mf_graph` |
| `graph/MinCostFlow` | `atcoder::mcf_graph` |
| `graph/SCC` | `atcoder::scc_graph` |
| `math/FloorSum` | `atcoder::floor_sum` |
| `math/NTT` | `atcoder::convolution` |
| `string/SuffixArray` | `atcoder::suffix_array` / `lcp_array` |

### 標準ライブラリ・テンプレートで足りるもの

| Java | 代替 |
|---|---|
| `dataStructure/collection/BitSet` | `std::bitset` |
| `dataStructure/collection/MyList` | `std::vector` / `std::deque` |
| `util/Util` | テンプレートの `SUM` / `MAX` / `MIN` / `std::swap` など |
| `math/Permutation`（`increment` / `decrement`） | `std::next_permutation` / `prev_permutation` |
| スニペット `Deque`（`MyDeque`。リングバッファ） | `std::deque` |
| スニペット `lcm` | `std::lcm` |
| スニペット `zaatu`（座標圧縮） | テンプレートの `compress` |
| スニペット `reverse` | `std::reverse` / `views::reverse` |
| スニペット `addId`（各行に添字を付ける） | `rep` で 1 行。ライブラリにするほどではない |
| スニペット `for`（部分集合列挙） | テンプレートの `fore_subset` マクロ |

### 対象外

| Java | 理由 |
|---|---|
| `graph/unionfind/PersistentUnionFind` | 移植しない方針 |
| `dataStructure/rangeData/base/BaseV` | Java の抽象クラス基盤。C++ ではテンプレート引数で済む |
| `dataStructure/.../AVLSegmentTreeLong` | `AVLSegmentTree` の long 特殊化。C++ ではテンプレートで型を切り替えられる |

---

## 残り

優先度は、競プロでの出番の多さで付けた目安。
「元」がスニペットのものは、ライブラリディレクトリには無く Eclipse のテンプレートにしかない。

### 高

| 元 | 想定ファイル名 | 内容 |
|---|---|---|
| `math/Combin` | `math/combin.hpp` | 階乗テーブル、`nCr` / `nHr`。modint と組む |
| `dataStructure/rangeData/SparseTable` | `data_structure/sparse_table.hpp` | 区間 min/max を `O(1)` |
| `string/RollingHash` | `string/rolling_hash.hpp` | |
| `other/Grid` | `util/grid.hpp` | 2 次元グリッドの添字変換と 4/8 近傍 |
| スニペット `zAlgo` | `string/z_algorithm.hpp` | Z-algorithm。ACL に無い |
| スニペット `mo`（`Mo`） | `util/mo.hpp` | Mo's algorithm。クラス 59 行 + 使用例 78 行 |
| `dataStructure/.../AVLSegmentTree` | `data_structure/avl_segtree.hpp` | AVL 木で列を持つ。挿入・削除・区間反転・区間巡回シフトができる遅延セグ木 |

### 中

| 元 | 想定ファイル名 | 内容 |
|---|---|---|
| `graph/tree/HLD` | `graph/tree/hld.hpp` | HL 分解 |
| `graph/tree/ReRootingDp` | `graph/tree/rerooting.hpp` | 全方位木 DP |
| `graph/LowLink` | `graph/lowlink.hpp` | 橋・関節点 |
| `dataStructure/SWAG` | `data_structure/swag.hpp` | Sliding Window Aggregation |
| `dataStructure/FastSet` | `data_structure/fast_set.hpp` | 64 分木の整数集合。`floor` / `ceiling` が速い |
| `dataStructure/.../DynamicSegmentTree` | `data_structure/dynamic_segtree.hpp` | 座標が大きいときのセグ木 |
| `dataStructure/cuboidData/Sum3D` | `data_structure/cumsum3d.hpp` | 3 次元累積和 |
| `dataStructure/rectangleData/SparseTable2D` | `data_structure/sparse_table2d.hpp` | |
| `other/DigitDp` | `util/digit_dp.hpp` | 桁 DP の枠組み |
| スニペット `BIT2D` | `data_structure/bit2d.hpp` | 2 次元 BIT。点加算・矩形和 |
| スニペット `SegmentTree2D` | `data_structure/segtree2d.hpp` | 2 次元セグ木 |
| スニペット `FFT` | `math/fft.hpp` | 浮動小数の FFT。ACL の `convolution` は NTT なので任意 mod / 実数はこちら |
| スニペット `lcs` | `string/lcs.hpp` | 最長共通部分列 |
| スニペット `cross` | `geometry/geometry.hpp` | 線分交差判定 |
| スニペット `angleSort` | `geometry/geometry.hpp` | 偏角ソート |
| スニペット `kika`（`gaisin`） | `geometry/geometry.hpp` | 三角形の外心 |

### 低

| 元 | 想定ファイル名 | 内容 |
|---|---|---|
| `dataStructure/AVLTree` | `data_structure/avl_tree.hpp` | 順序統計木。`std::set` + BIT で代用できることが多い |
| `dataStructure/.../PersistentSegmentTree` | `data_structure/persistent_segtree.hpp` | |
| `dataStructure/PersistentArray` | `data_structure/persistent_array.hpp` | |
| `dataStructure/PriorityDeque` | `data_structure/priority_deque.hpp` | 両端優先度付きキュー |
| `dataStructure/collection/DeletablePQue` | `data_structure/deletable_pque.hpp` | 削除できる優先度付きキュー。ヒープ 2 本 |
| `dataStructure/rangeData/RangeTree` | `data_structure/range_tree.hpp` | |
| `math/RelaxedNTT` | `math/relaxed_convolution.hpp` | オンライン畳み込み |
| スニペット `trans` | `util/transpose.hpp` | 行列の転置。4 つの型ごとにオーバーロードされている |

---

## avl_segtree の仕様

`AVLSegmentTree.java`（306 行）の移植。決めたことを残しておく。

- **型** `avl_segtree<S, op, e, F, mapping, composition, id, rev>`。
  ACL の `lazy_segtree` と同じ並びで、末尾に反転時の値の直し `rev` を足す
- **`avl_value`** を継承した型を載せる。`int sz` と `bool fail` を持つ基底で、
  どちらもライブラリが維持する。ユーザーは `op` の中で `sz` を触らない
- **`mapping` は ACL と同じ `S mapping(F f, S x)`**。`x.sz` には呼ぶ前に
  節点の要素数が入っている
- **Beats** は `mapping` の中で `x.fail = true` を立てる。ライブラリが検知して
  子へ降りる。`sz` / `fail` の有無は concept で見るので、持たない型も載せられる
- **節点は `std::vector<node>` のプールに置き、`int` の添字で辿る**。
  ポインタや参照で持つと `push_back` の再確保で無効になる
- 葉は「同じ値 k 個」を持てる（ランレングス）。`avl_segtree(n)` が O(1)

段階を分けて進める。

| 段階 | 内容 | 状態 |
|---|---|---|
| 1 | AVL の骨格（merge / split / balance）+ build / insert / erase / get / prod / size | 済 |
| 2 | 遅延伝搬 apply(l, r, f) | 未 |
| 3 | 区間反転 reverse(l, r) と rev フック | 未 |
| 4 | 区間巡回シフト rotate(l, r, k) | 未 |
| 5 | 葉のランレングス圧縮 | 未 |
| 6 | Beats（fail） | 未 |

全段階が終わるまでは「移植済み」に数えず、ここで進み具合を見る。

段階 3 まで終われば
[dynamic_sequence_range_affine_range_sum](https://judge.yosupo.jp/problem/dynamic_sequence_range_affine_range_sum)
で verify できる。

---

## verify 状況

**0 / 17 が verify 済み。** `test/` のテストは総当たりとの突き合わせなので、
実装の正しさはある程度見ているが、公開ジャッジは 1 つも通していない。

| ライブラリ | 状態 | verify 先 |
|---|---|---|
| `cumsum` | 未 | [static_range_sum](https://judge.yosupo.jp/problem/static_range_sum) |
| `cumsum2d` | 未 | 未定。Library Checker に密な 2 次元累積和そのものの問題は無い |
| `interval_set` | 未 | 未定 |
| `prime` | 未 | [factorize](https://judge.yosupo.jp/problem/factorize) / [enumerate_primes](https://judge.yosupo.jp/problem/enumerate_primes) |
| `inversion_count` | 未 | 未定。[static_range_inversions_query](https://judge.yosupo.jp/problem/static_range_inversions_query) は Mo's algorithm 前提で、配列全体の転倒数だけでは通らない |
| `rle` | 未 | 未定。Library Checker に該当する問題は無い |
| `union_find` | 未 | 未定 |
| `monoid_union_find` | 未 | [dynamic_graph_vertex_add_component_sum](https://judge.yosupo.jp/problem/dynamic_graph_vertex_add_component_sum)（`rollback_union_find` と組で Offline Dynamic Connectivity） |
| `relational_union_find` | 未 | [unionfind_with_potential](https://judge.yosupo.jp/problem/unionfind_with_potential) / [非可換版](https://judge.yosupo.jp/problem/unionfind_with_potential_non_commutative_group) |
| `rollback_union_find` | 未 | 同上（`monoid_union_find` と組で） |
| `dynamic_union_find` | 未 | 未定 |
| `trie` | 未 | 未定。[aho_corasick](https://judge.yosupo.jp/problem/aho_corasick) は別物（AC 自動機） |
| `binary_trie` | 未 | [set_xor_min](https://judge.yosupo.jp/problem/set_xor_min) |
| `matrix` | 未 | [matrix_product](https://judge.yosupo.jp/problem/matrix_product) / [pow_of_matrix](https://judge.yosupo.jp/problem/pow_of_matrix) / [matrix_det](https://judge.yosupo.jp/problem/matrix_det) / [matrix_rank](https://judge.yosupo.jp/problem/matrix_rank) / [inverse_matrix](https://judge.yosupo.jp/problem/inverse_matrix) / [system_of_linear_equations](https://judge.yosupo.jp/problem/system_of_linear_equations) |
| `maxplus_matrix` | 未 | 未定 |
| `graph` | 未 | `dijkstra` と一緒に検証される |
| `dijkstra` | 未 | [shortest_path](https://judge.yosupo.jp/problem/shortest_path) |

`relational_union_find` の非可換版は優先度が高い。現状のテストは `plus` と `bit_xor` だけで
どちらも可換なため、**合成順を間違えていても検出できない**。

verify を通したら、この表の状態を「済」にしてリンクを残し、対応するヘッダの
`verify:` 欄も `(未 verify)` から実際の URL に書き換える。
