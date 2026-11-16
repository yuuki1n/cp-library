#include <algorithm>
#include <bit>
#include <cassert>
#include <cstddef>
#include <vector>

// https://github.com/yuuki1n/cp-library/blob/main/data_structure/disjoint_sparse_table.hpp

/*
 * disjoint_sparse_table<S, op> : 変更のない列の区間積を O(1) で答える
 *
 *   disjoint_sparse_table(a)    列 a から作る   O(n log n)
 *   prod(l, r)                  op(a[l], ..., a[r-1])   O(1)。l < r であること
 *   get(i)                      a[i]
 *   size()                      要素数
 *
 *   op は結合法則さえ満たせばよい。単位元・逆元・冪等性は要らないので、
 *   min / max / gcd に加えて、和・mod 付きの積・行列積・一次関数の合成
 *   （非可換）も乗る。単位元を持たないので空区間は聞けない。
 *
 *   段 k では長さ 2^(k+1) のブロックごとに中央から左右へ累積を持つ。
 *   [l, r) は l と r-1 が初めて別のブロックに分かれる段で、左の累積と
 *   右の累積を 1 回 op すれば答えになる。段 0 は元の列そのもの。
 *   メモリは S が n * ceil(log2 n) 個（2 の冪には切り上げない）。
 *
 * 使用例:
 *   ll op(ll a, ll b) { return gcd(a, b); }
 *   disjoint_sparse_table<ll, op> dst(A);
 *   dst.prod(l, r);
 */
template <class S, S (*op)(S, S)> struct disjoint_sparse_table {
 private:
  int n = 0, lg = 1;
  std::vector<S> dat;  // lg 段 * n。段 k の位置 i は dat[k * n + i]

 public:
  disjoint_sparse_table() = default;
  explicit disjoint_sparse_table(const std::vector<S>& a) : n((int)a.size()) {
    if (n >= 2) lg = (int)std::bit_width((unsigned)(n - 1));
    dat.resize((std::size_t)lg * n);
    for (int i = 0; i < n; i++) dat[i] = a[i];
    for (int k = 1; k < lg; k++) {
      S* d = dat.data() + (std::size_t)k * n;
      for (int s = 1 << k; s < n; s += 2 << k) {
        int lo = s - (1 << k), hi = std::min(s + (1 << k), n);
        d[s] = a[s];  // 右へ: [s, i]
        for (int i = s + 1; i < hi; i++) d[i] = op(d[i - 1], a[i]);
        d[s - 1] = a[s - 1];  // 左へ: [i, s)。非可換でも崩れないよう a[i] を左に置く
        for (int i = s - 2; i >= lo; i--) d[i] = op(a[i], d[i + 1]);
      }
    }
  }

  int size() const { return n; }
  S get(int i) const {
    assert(0 <= i && i < n);
    return dat[i];
  }
  // [l, r) の積。空区間は不可
  S prod(int l, int r) const {
    assert(0 <= l && l < r && r <= n);
    if (--r == l) return dat[l];
    int k = (int)std::bit_width((unsigned)(l ^ r)) - 1;
    const S* d = dat.data() + (std::size_t)k * n;
    return op(d[l], d[r]);
  }
};
