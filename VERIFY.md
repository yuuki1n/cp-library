# verify 状況

`test/verify/` のコードをジャッジに通した記録。移植そのものの進み具合は
[PORTING.md](PORTING.md) を参照。

**2 / 18 が verify 済み。** `test/` のテストは総当たりとの突き合わせなので、
実装の正しさはある程度見ているが、公開ジャッジを通したものはまだ少ない。

| ライブラリ | 状態 | verify 先 |
|---|---|---|
| `cumsum` | 未 | [static_range_sum](https://judge.yosupo.jp/problem/static_range_sum) |
| `cumsum2d` | 未 | 未定。Library Checker に密な 2 次元累積和そのものの問題は無い |
| `interval_set` | 未 | 未定 |
| `prime` | 未 | [factorize](https://judge.yosupo.jp/problem/factorize) / [enumerate_primes](https://judge.yosupo.jp/problem/enumerate_primes) |
| `inversion_count` | 未 | 未定。[static_range_inversions_query](https://judge.yosupo.jp/problem/static_range_inversions_query) は Mo's algorithm 前提で、配列全体の転倒数だけでは通らない |
| `rle` | 未 | 未定。Library Checker に該当する問題は無い |
| `avl_segtree` | **済** | [dynamic_sequence_range_affine_range_sum](https://judge.yosupo.jp/problem/dynamic_sequence_range_affine_range_sum) / [range_chmin_chmax_add_range_sum](https://judge.yosupo.jp/problem/range_chmin_chmax_add_range_sum)（Beats） / [range_affine_range_sum_large_array](https://judge.yosupo.jp/problem/range_affine_range_sum_large_array)（N ≤ 10^9） / [point_set_range_composite](https://judge.yosupo.jp/problem/point_set_range_composite)（非可換な op） / [range_reverse_range_sum](https://judge.yosupo.jp/problem/range_reverse_range_sum)（区間反転） / [range_affine_point_get](https://judge.yosupo.jp/problem/range_affine_point_get)（get） / [deque](https://judge.yosupo.jp/problem/deque)（両端の挿入削除） |
| `union_find` | 未 | 未定 |
| `monoid_union_find` | 未 | [dynamic_graph_vertex_add_component_sum](https://judge.yosupo.jp/problem/dynamic_graph_vertex_add_component_sum)（`rollback_union_find` と組で Offline Dynamic Connectivity） |
| `relational_union_find` | **済** | [非可換版](https://judge.yosupo.jp/problem/unionfind_with_potential_non_commutative_group)。可換版の [unionfind_with_potential](https://judge.yosupo.jp/problem/unionfind_with_potential) は未 |
| `rollback_union_find` | 未 | 同上（`monoid_union_find` と組で） |
| `dynamic_union_find` | 未 | 未定 |
| `trie` | 未 | 未定。[aho_corasick](https://judge.yosupo.jp/problem/aho_corasick) は別物（AC 自動機） |
| `binary_trie` | 未 | [set_xor_min](https://judge.yosupo.jp/problem/set_xor_min) |
| `matrix` | 未 | [matrix_product](https://judge.yosupo.jp/problem/matrix_product) / [pow_of_matrix](https://judge.yosupo.jp/problem/pow_of_matrix) / [matrix_det](https://judge.yosupo.jp/problem/matrix_det) / [matrix_rank](https://judge.yosupo.jp/problem/matrix_rank) / [inverse_matrix](https://judge.yosupo.jp/problem/inverse_matrix) / [system_of_linear_equations](https://judge.yosupo.jp/problem/system_of_linear_equations) |
| `maxplus_matrix` | 未 | 未定 |
| `graph` | 未 | `dijkstra` と一緒に検証される |
| `dijkstra` | 未 | [shortest_path](https://judge.yosupo.jp/problem/shortest_path) |

verify を通したら、この表の状態を「済」にしてリンクを残す。記録はここだけにまとめる
（ヘッダのコメントには書かない）。
手元で全ケースを回すには `python tools/verify_local.py test/verify/foo.cpp`。
