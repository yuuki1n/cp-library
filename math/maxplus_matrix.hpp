#include <cassert>
#include <limits>
#include <type_traits>
#include <vector>

/*
 * maxplus_matrix<T> : (max, +) 半環の正方行列
 *
 *   普通の行列積の * を + に、+ を max に置き換えたもの。
 *   「ちょうど k 辺での最長路」「1 歩ごとに選択肢がある DP の k 歩後」に使う。
 *
 *   maxplus_matrix(n)  n x n。すべて NEG（到達不能）
 *   identity(n)        対角 0、他は NEG
 *   A[i][j]
 *   A * B              r[i][j] = max_k (A[i][k] + B[k][j])   O(n^3)
 *   v * A              r[j] = max_i (v[i] + A[i][j])         O(n^2)
 *   pow(k)             k 乗                                  O(n^3 log k)
 *   NEG                到達不能を表す番兵。これ未満の値は入れないこと
 *
 *   添字の向きは A[i][j] = 「i から j」で統一する。状態を進めるのは v * A。
 *   min を求めたいときは重みを全部符号反転して使い、結果も符号反転する。
 *   NEG は反転しない（min 側の +INF に対応する）。
 *   T は符号付きであること。経路の重みの総和が |NEG| を超えないこと。
 *
 * 使用例:
 *   maxplus_matrix<ll> M(n);
 *   fore(e, edges) chmax(M[e.from][e.to], e.w);
 *   auto P = M.pow(K);             // ちょうど K 辺での最長路
 *   print(P[s][t] == P.NEG ? -1 : P[s][t]);
 *
 *   vll v(n, M.NEG);               // 始点だけ 0 にして K 歩進める
 *   v[s] = 0;
 *   v = v * M.pow(K);
 *
 * verify:
 *   (未 verify)
 */
template <class T = long long> struct maxplus_matrix {
  static_assert(std::is_signed_v<T>,
                "T は符号付きであること（NEG に使うため）");
  static constexpr T NEG = std::numeric_limits<T>::lowest() / 4;

  int n;
  std::vector<std::vector<T>> a;

  explicit maxplus_matrix(int n_ = 0) : n(n_), a(n_, std::vector<T>(n_, NEG)) {}

  static maxplus_matrix identity(int n) {
    maxplus_matrix r(n);
    for (int i = 0; i < n; i++) r[i][i] = T(0);
    return r;
  }

  std::vector<T>& operator[](int i) { return a[i]; }
  const std::vector<T>& operator[](int i) const { return a[i]; }

  maxplus_matrix operator*(const maxplus_matrix& o) const {
    assert(n == o.n);
    maxplus_matrix r(n);
    for (int i = 0; i < n; i++)
      for (int k = 0; k < n; k++) {
        if (a[i][k] == NEG) continue;  // 到達不能を足さない（溢れ防止）
        for (int j = 0; j < n; j++) {
          if (o[k][j] == NEG) continue;
          T v = a[i][k] + o[k][j];
          if (r[i][j] < v) r[i][j] = v;
        }
      }
    return r;
  }

  maxplus_matrix pow(long long k) const {
    assert(k >= 0);
    maxplus_matrix r = identity(n), x = *this;
    while (k) {
      if (k & 1) r = r * x;
      x = x * x;
      k >>= 1;
    }
    return r;
  }
};

// 行ベクトル x 行列
template <class T>
std::vector<T> operator*(const std::vector<T>& v, const maxplus_matrix<T>& A) {
  assert((int)v.size() == A.n);
  std::vector<T> r(A.n, maxplus_matrix<T>::NEG);
  for (int i = 0; i < A.n; i++) {
    if (v[i] == maxplus_matrix<T>::NEG) continue;
    for (int j = 0; j < A.n; j++) {
      if (A[i][j] == maxplus_matrix<T>::NEG) continue;
      T x = v[i] + A[i][j];
      if (r[j] < x) r[j] = x;
    }
  }
  return r;
}
