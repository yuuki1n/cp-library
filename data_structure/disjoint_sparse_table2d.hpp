#include <algorithm>
#include <bit>
#include <cassert>
#include <cstddef>
#include <vector>

// https://github.com/yuuki1n/cp-library/blob/main/data_structure/disjoint_sparse_table2d.hpp

/*
 * disjoint_sparse_table2d<S, op> : 変更のない 2 次元配列の長方形の積を O(1) で答える
 *
 *   disjoint_sparse_table2d(g)    H x W の g から作る   O(HW log H log W)
 *   prod(i0, j0, i1, j1)          [i0, i1) x [j0, j1) の積   O(1)。空でないこと
 *   get(i, j)                     g[i][j]
 *   h() / w()                     大きさ
 *
 *   op は結合法則と交換法則を満たすこと（和・積・min・max・gcd・xor など）。
 *   長方形を最大 4 つに割って合わせるので、並び順を決められない。
 *   冪等である必要は無い。単位元を持たないので空の長方形は聞けない。
 *
 *   行方向の段 a ごとに「行ブロックの中央から上下へ、列ごとに累積した表」を持ち、
 *   その表の各行に列方向の DST（段 b）をかける。質問は行・列それぞれで分かれる段を
 *   求め、表を最大 4 回引いて 3 回 op する。
 *   メモリは S が H * W * ceil(log2 H) * ceil(log2 W) 個（2 の冪には切り上げない）。
 *   H = W = 500 で約 2 * 10^7 個、1000 で約 10^8 個なので、long long なら 1000 は厳しい。
 *
 * 使用例:
 *   ll op(ll a, ll b) { return gcd(a, b); }
 *   disjoint_sparse_table2d<ll, op> dst(G);
 *   dst.prod(i0, j0, i1, j1);
 */
template <class S, S (*op)(S, S)> struct disjoint_sparse_table2d {
 private:
  int H = 0, W = 0;
  int lh = 1, lw = 1;  // 段の数
  std::vector<S> dat;  // (a, b, i, j) は dat[((a * lw + b) * H + i) * W + j]

  static int levels(int n) { return n >= 2 ? (int)std::bit_width((unsigned)(n - 1)) : 1; }
  S* row(int a, int b, int i) { return dat.data() + (((std::size_t)a * lw + b) * H + i) * W; }
  const S* row(int a, int b, int i) const { return dat.data() + (((std::size_t)a * lw + b) * H + i) * W; }

  // 行方向の段 a、行 i の中で [j0, j1] の積（閉区間）
  S prod_row(int a, int i, int j0, int j1) const {
    if (j0 == j1) return row(a, 0, i)[j0];
    int b = (int)std::bit_width((unsigned)(j0 ^ j1)) - 1;
    const S* r = row(a, b, i);
    return op(r[j0], r[j1]);
  }

 public:
  disjoint_sparse_table2d() = default;
  explicit disjoint_sparse_table2d(const std::vector<std::vector<S>>& g) {
    H = (int)g.size(), W = H ? (int)g[0].size() : 0;
    if (H == 0 || W == 0) return;
    lh = levels(H), lw = levels(W);
    dat.resize((std::size_t)lh * lw * H * W);

    // 行方向の段 a の表を列ごとの累積で作る（列方向は段 0 の位置に置く）
    for (int i = 0; i < H; i++)
      for (int j = 0; j < W; j++) row(0, 0, i)[j] = g[i][j];
    for (int a = 1; a < lh; a++)
      for (int s = 1 << a; s < H; s += 2 << a) {
        int lo = s - (1 << a), hi = std::min(s + (1 << a), H);
        std::copy(row(0, 0, s), row(0, 0, s) + W, row(a, 0, s));
        for (int i = s + 1; i < hi; i++) {
          S *d = row(a, 0, i), *p = row(a, 0, i - 1), *x = row(0, 0, i);
          for (int j = 0; j < W; j++) d[j] = op(p[j], x[j]);
        }
        std::copy(row(0, 0, s - 1), row(0, 0, s - 1) + W, row(a, 0, s - 1));
        for (int i = s - 2; i >= lo; i--) {
          S *d = row(a, 0, i), *p = row(a, 0, i + 1), *x = row(0, 0, i);
          for (int j = 0; j < W; j++) d[j] = op(x[j], p[j]);
        }
      }

    // 各表の各行に列方向の DST をかける
    for (int a = 0; a < lh; a++)
      for (int i = 0; i < H; i++) {
        if (a && ((i >> (a + 1) << (a + 1)) | (1 << a)) >= H) continue;  // 中央が無いブロックの行は使われない
        const S* x = row(a, 0, i);
        for (int b = 1; b < lw; b++) {
          S* d = row(a, b, i);
          for (int s = 1 << b; s < W; s += 2 << b) {
            int lo = s - (1 << b), hi = std::min(s + (1 << b), W);
            d[s] = x[s];
            for (int j = s + 1; j < hi; j++) d[j] = op(d[j - 1], x[j]);
            d[s - 1] = x[s - 1];
            for (int j = s - 2; j >= lo; j--) d[j] = op(x[j], d[j + 1]);
          }
        }
      }
  }

  int h() const { return H; }
  int w() const { return W; }
  S get(int i, int j) const {
    assert(0 <= i && i < H && 0 <= j && j < W);
    return row(0, 0, i)[j];
  }
  // [i0, i1) x [j0, j1) の積。空の長方形は不可
  S prod(int i0, int j0, int i1, int j1) const {
    assert(0 <= i0 && i0 < i1 && i1 <= H && 0 <= j0 && j0 < j1 && j1 <= W);
    --i1, --j1;
    if (i0 == i1) return prod_row(0, i0, j0, j1);
    int a = (int)std::bit_width((unsigned)(i0 ^ i1)) - 1;
    return op(prod_row(a, i0, j0, j1), prod_row(a, i1, j0, j1));
  }
};
