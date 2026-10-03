#include <algorithm>
#include <cassert>
#include <vector>

/*
 * combin<T> : 階乗と逆階乗を持って二項係数などを返す
 *
 *   combin()        空で作る。必要になった時点で伸びる
 *   combin(n)       n まで先に作っておく
 *   C(n, r)         二項係数。n < 0 や r < 0、n < r なら 0
 *   P(n, r)         順列。n! / (n-r)!
 *   H(n, r)         重複組合せ。n 種類から重複を許して r 個 = C(n+r-1, r)
 *   catalan(n)      カタラン数 C(2n, n) / (n+1)
 *   fact(n)         n!
 *   finv(n)         1 / n!
 *   inv(n)          1 / n。n >= 1 であること
 *
 *   表は 2 倍ずつ伸ばすので、大きさを決めずに使ってよい。
 *   伸ばすたびに逆元を 1 回だけ求め、そこから掛け算で埋める。
 *   T は modint（ACL の static_modint / dynamic_modint など）を想定する。
 *   T に要る演算: * と inv() と T(整数)。
 *
 *   法より大きい引数は扱えない（n! が 0 になるため）。
 *   mod が小さいときに n >= mod を渡さないこと。
 *
 * 使用例:
 *   combin<mint> C;
 *   print(C.C(N, K));           // 二項係数
 *   print(C.H(N, K));           // 重複組合せ
 *
 *   // 隣接しないように N 個から K 個選ぶ
 *   print(C.C(N - K + 1, K));
 */
template <class T> struct combin {
 private:
  std::vector<T> f{T(1), T(1)}, fi{T(1), T(1)};  // 階乗 / 逆階乗。0! と 1! を入れておく

  // n 番目まで引けるように伸ばす
  void grow(long long n) {
    long long m = (long long)f.size();
    if (n < m) return;
    n = std::max(n + 1, m * 2);
    f.resize(n), fi.resize(n);
    for (long long i = m; i < n; i++) f[i] = f[i - 1] * T(i);
    fi[n - 1] = f[n - 1].inv();
    // 逆元は 1 回だけ求め、残りは 1/(i-1)! = 1/i! * i で埋める
    for (long long i = n - 1; i >= m; i--) fi[i - 1] = fi[i] * T(i);
  }

 public:
  combin() = default;
  explicit combin(long long n) { grow(n); }

  T fact(long long n) {
    assert(n >= 0);
    grow(n);
    return f[n];
  }
  T finv(long long n) {
    assert(n >= 0);
    grow(n);
    return fi[n];
  }
  // 1 / n。n >= 1 であること
  T inv(long long n) { return assert(n >= 1), finv(n) * fact(n - 1); }

  // 二項係数。範囲外は 0
  T C(long long n, long long r) {
    if (r < 0 || n < 0 || n < r) return T(0);
    grow(n);
    return f[n] * fi[r] * fi[n - r];
  }
  // 順列
  T P(long long n, long long r) {
    if (r < 0 || n < 0 || n < r) return T(0);
    grow(n);
    return f[n] * fi[n - r];
  }
  // 重複組合せ。n 種類から重複を許して r 個
  T H(long long n, long long r) {
    if (r < 0 || n < 0) return T(0);
    if (r == 0) return T(1);
    if (n == 0) return T(0);
    return C(n + r - 1, r);
  }
  // カタラン数
  T catalan(long long n) {
    if (n < 0) return T(0);
    return C(2 * n, n) * inv(n + 1);
  }
};
