#include <cassert>
#include <vector>

/*
 * matrix<T> : 行列
 *
 *   matrix(h, w)   h x w のゼロ行列
 *   identity(n)    n x n の単位行列
 *   A[i][j]        要素
 *   A + B / A - B / A * B / A * s
 *   A * v   r[i] = sum_j A[i][j] * v[j]   列ベクトル。A[i][j] が「j から i」のとき
 *   v * A   r[j] = sum_i v[i] * A[i][j]   行ベクトル。A[i][j] が「i から j」のとき
 *   pow(k)         k 乗。正方行列のみ            O(n^3 log k)
 *   det()          行列式                        O(n^3)
 *   rank()         階数                          O(n^3)
 *   inv()          逆行列。無ければ空の行列      O(n^3)
 *   solve(b)       Ax = b。空なら解なし。        O(n^3)
 *                  [0] が解の 1 つ、[1] 以降が自由度（核）の基底
 *
 *   vector<vector<T>> から作るときは各行の長さを揃えること。
 *   T に要る演算: + - * == と T(0) / T(1)。
 *   det / rank / inv / solve は除算も使うので modint や double 向け。
 *   ll でも積と累乗は使えるが、溢れは呼ぶ側の責任。
 *
 * 使用例:
 *   matrix<mint> A(2, 2);
 *   A[0][0] = A[0][1] = A[1][0] = 1;
 *   print(A.pow(N)[0][1]);          // フィボナッチ
 *
 *   auto x = A.solve(b);
 *   if (x.empty()) print(-1); else print(x[0]);
 */
template <class T> struct matrix {
  int h, w;
  std::vector<std::vector<T>> a;

  explicit matrix(int h_ = 0, int w_ = 0) : h(h_), w(w_), a(h_, std::vector<T>(w_, T(0))) {}
  matrix(const std::vector<std::vector<T>>& v) : h((int)v.size()), w(v.empty() ? 0 : (int)v[0].size()), a(v) {
    for (auto& row : v) assert((int)row.size() == w);  // 行の長さを揃えること
  }

  static matrix identity(int n) {
    matrix r(n, n);
    for (int i = 0; i < n; i++) r[i][i] = T(1);
    return r;
  }

  std::vector<T>& operator[](int i) { return a[i]; }
  const std::vector<T>& operator[](int i) const { return a[i]; }

  matrix operator+(const matrix& o) const {
    assert(h == o.h && w == o.w);
    matrix r(h, w);
    for (int i = 0; i < h; i++)
      for (int j = 0; j < w; j++) r[i][j] = a[i][j] + o[i][j];
    return r;
  }
  matrix operator-(const matrix& o) const {
    assert(h == o.h && w == o.w);
    matrix r(h, w);
    for (int i = 0; i < h; i++)
      for (int j = 0; j < w; j++) r[i][j] = a[i][j] - o[i][j];
    return r;
  }
  matrix operator*(const matrix& o) const {
    assert(w == o.h);
    matrix r(h, o.w);
    for (int i = 0; i < h; i++)
      for (int k = 0; k < w; k++) {
        if (a[i][k] == T(0)) continue;
        for (int j = 0; j < o.w; j++) r[i][j] = r[i][j] + a[i][k] * o[k][j];
      }
    return r;
  }
  std::vector<T> operator*(const std::vector<T>& v) const {
    assert((int)v.size() == w);
    std::vector<T> r(h, T(0));
    for (int i = 0; i < h; i++)
      for (int j = 0; j < w; j++) r[i] = r[i] + a[i][j] * v[j];
    return r;
  }
  matrix operator*(const T& s) const {
    matrix r(h, w);
    for (int i = 0; i < h; i++)
      for (int j = 0; j < w; j++) r[i][j] = a[i][j] * s;
    return r;
  }

  matrix pow(long long k) const {
    assert(h == w && k >= 0);
    matrix r = identity(h), x = *this;
    while (k) {
      if (k & 1) r = r * x;
      x = x * x;
      k >>= 1;
    }
    return r;
  }

  T det() const {
    assert(h == w);
    auto A = a;
    T d(1);
    for (int c = 0; c < w; c++) {
      int p = -1;
      for (int i = c; i < h; i++)
        if (!(A[i][c] == T(0))) {
          p = i;
          break;
        }
      if (p < 0) return T(0);
      if (p != c) {
        std::swap(A[p], A[c]);
        d = T(0) - d;
      }
      d = d * A[c][c];
      T f = T(1) / A[c][c];
      for (int j = c; j < w; j++) A[c][j] = A[c][j] * f;
      for (int i = c + 1; i < h; i++) {
        if (A[i][c] == T(0)) continue;
        T g = A[i][c];
        for (int j = c; j < w; j++) A[i][j] = A[i][j] - g * A[c][j];
      }
    }
    return d;
  }

  int rank() const {
    auto A = a;
    std::vector<int> piv;
    return gauss_(A, w, piv);
  }

  matrix inv() const {
    assert(h == w);
    std::vector<std::vector<T>> A(h, std::vector<T>(2 * h, T(0)));
    for (int i = 0; i < h; i++) {
      for (int j = 0; j < h; j++) A[i][j] = a[i][j];
      A[i][h + i] = T(1);
    }
    std::vector<int> piv;
    if (gauss_(A, h, piv) < h) return matrix();
    matrix r(h, h);
    for (int i = 0; i < h; i++)
      for (int j = 0; j < h; j++) r[i][j] = A[i][h + j];
    return r;
  }

  std::vector<std::vector<T>> solve(const std::vector<T>& b) const {
    assert((int)b.size() == h);
    std::vector<std::vector<T>> A(h, std::vector<T>(w + 1));
    for (int i = 0; i < h; i++) {
      for (int j = 0; j < w; j++) A[i][j] = a[i][j];
      A[i][w] = b[i];
    }
    std::vector<int> piv;
    int r = gauss_(A, w, piv);
    for (int i = r; i < h; i++)
      if (!(A[i][w] == T(0))) return {};  // 解なし

    std::vector<std::vector<T>> res(1, std::vector<T>(w, T(0)));
    for (int i = 0; i < r; i++) res[0][piv[i]] = A[i][w];

    std::vector<bool> isp(w, false);
    for (int c : piv) isp[c] = true;
    for (int c = 0; c < w; c++) {  // 自由変数ごとに核の基底を 1 本
      if (isp[c]) continue;
      std::vector<T> v(w, T(0));
      v[c] = T(1);
      for (int i = 0; i < r; i++) v[piv[i]] = T(0) - A[i][c];
      res.push_back(v);
    }
    return res;
  }

 private:
  // Gauss-Jordan。[0, wlim) 列だけをピボット候補にする。
  // 戻り値は階数、piv[i] = i 行目のピボット列
  static int gauss_(std::vector<std::vector<T>>& A, int wlim, std::vector<int>& piv) {
    int H = (int)A.size();
    int r = 0;
    piv.clear();
    for (int c = 0; c < wlim && r < H; c++) {
      int p = -1;
      for (int i = r; i < H; i++)
        if (!(A[i][c] == T(0))) {
          p = i;
          break;
        }
      if (p < 0) continue;
      std::swap(A[p], A[r]);
      T f = T(1) / A[r][c];
      for (auto& x : A[r]) x = x * f;
      for (int i = 0; i < H; i++) {
        if (i == r || A[i][c] == T(0)) continue;
        T g = A[i][c];
        for (std::size_t j = 0; j < A[i].size(); j++) A[i][j] = A[i][j] - g * A[r][j];
      }
      piv.push_back(c);
      r++;
    }
    return r;
  }
};

// 行ベクトル x 行列
template <class T> std::vector<T> operator*(const std::vector<T>& v, const matrix<T>& A) {
  assert((int)v.size() == A.h);
  std::vector<T> r(A.w, T(0));
  for (int i = 0; i < A.h; i++)
    for (int j = 0; j < A.w; j++) r[j] = r[j] + v[i] * A[i][j];
  return r;
}
