// math/matrix.hpp / math/matrix_maxplus.hpp の検証。素朴な実装と突き合わせる。
//   g++ -std=gnu++20 -O2 -Wall -Wextra -I.. -IC:/acl matrix_test.cpp -o matrix_test
#include "../../math/matrix.hpp"

#include <algorithm>
#include <atcoder/modint>
#include <chrono>
#include <cstdio>
#include <random>
#include <string>
#include <vector>

#include "../../math/maxplus_matrix.hpp"

using ll = long long;
using namespace std;
using mint = atcoder::modint998244353;

int ng = 0;        // 今のブロックの NG 件数（report のたびに 0 に戻す）
int ng_total = 0;  // 全体の NG 件数
void check(bool ok, const string& msg) {
  if (!ok && ng < 5) printf("  NG: %s\n", msg.c_str());
  if (!ok) ng++, ng_total++;
}
void report(const string& name) {
  printf("%-30s : %s\n", name.c_str(), ng ? "NG" : "OK");
  ng = 0;
}

matrix<mint> rnd(mt19937& rng, int h, int w, int mod = 5) {
  matrix<mint> A(h, w);
  for (int i = 0; i < h; i++)
    for (int j = 0; j < w; j++) A[i][j] = (int)(rng() % mod);
  return A;
}

// 余因子展開による行列式（n <= 6 用）
mint det_naive(const matrix<mint>& A) {
  int n = A.h;
  vector<int> p(n);
  for (int i = 0; i < n; i++) p[i] = i;
  mint s = 0;
  do {
    int inv = 0;
    for (int i = 0; i < n; i++)
      for (int j = i + 1; j < n; j++) inv += p[i] > p[j];
    mint t = 1;
    for (int i = 0; i < n; i++) t *= A[i][p[i]];
    s += (inv & 1) ? -t : t;
  } while (next_permutation(p.begin(), p.end()));
  return s;
}

int main() {
  mt19937 rng(20260915);

  {  // ---- 積 / 和 / スカラー倍 ----
    ng = 0;
    for (int it = 0; it < 300; it++) {
      int h = 1 + (int)(rng() % 5), k = 1 + (int)(rng() % 5), w = 1 + (int)(rng() % 5);
      auto A = rnd(rng, h, k), B = rnd(rng, k, w);
      auto C = A * B;
      check(C.h == h && C.w == w, "積の形");
      for (int i = 0; i < h; i++)
        for (int j = 0; j < w; j++) {
          mint s = 0;
          for (int t = 0; t < k; t++) s += A[i][t] * B[t][j];
          check(C[i][j] == s, "積の値");
        }
      auto D = rnd(rng, h, k);
      auto S = A + D, M = A - D;
      for (int i = 0; i < h; i++)
        for (int j = 0; j < k; j++) {
          check(S[i][j] == A[i][j] + D[i][j], "和");
          check(M[i][j] == A[i][j] - D[i][j], "差");
        }
      mint s = (int)(rng() % 100);
      auto E = A * s;
      for (int i = 0; i < h; i++)
        for (int j = 0; j < k; j++) check(E[i][j] == A[i][j] * s, "スカラー倍");

      // 行列 x ベクトル
      vector<mint> v(k);
      for (auto& x : v) x = (int)(rng() % 5);
      auto Av = A * v;
      check((int)Av.size() == h, "A * v の長さ");
      for (int i = 0; i < h; i++) {
        mint t = 0;
        for (int j = 0; j < k; j++) t += A[i][j] * v[j];
        check(Av[i] == t, "A * v の値");
      }
      // (A * B) * v == A * (B * v)
      vector<mint> ones(w, mint(1));
      auto lhs = (A * B) * ones;
      auto rhs = A * (B * ones);
      for (int i = 0; i < h; i++) check(lhs[i] == rhs[i], "結合則");

      // 行ベクトル x 行列
      vector<mint> u(h);
      for (auto& x : u) x = (int)(rng() % 5);
      auto uA = u * A;
      check((int)uA.size() == k, "v * A の長さ");
      for (int j = 0; j < k; j++) {
        mint t = 0;
        for (int i = 0; i < h; i++) t += u[i] * A[i][j];
        check(uA[j] == t, "v * A の値");
      }
      auto l2 = (u * A) * B, r2 = u * (A * B);
      for (int j = 0; j < w; j++) check(l2[j] == r2[j], "行ベクトルの結合則");
    }
    report("積 / 和 / スカラー倍");
  }

  {  // ---- 単位行列と累乗 ----
    ng = 0;
    for (int it = 0; it < 200; it++) {
      int n = 1 + (int)(rng() % 5);
      auto A = rnd(rng, n, n);
      auto I = matrix<mint>::identity(n);
      auto AI = A * I;
      for (int i = 0; i < n; i++)
        for (int j = 0; j < n; j++) check(AI[i][j] == A[i][j], "A * I == A");

      int k = (int)(rng() % 12);
      auto P = A.pow(k);
      auto Q = matrix<mint>::identity(n);
      for (int t = 0; t < k; t++) Q = Q * A;
      for (int i = 0; i < n; i++)
        for (int j = 0; j < n; j++) check(P[i][j] == Q[i][j], "pow");
      auto P0 = A.pow(0);
      for (int i = 0; i < n; i++)
        for (int j = 0; j < n; j++) check(P0[i][j] == (i == j ? mint(1) : mint(0)), "pow(0) は単位行列");
    }
    // フィボナッチ
    {
      matrix<mint> F(2, 2);
      F[0][0] = F[0][1] = F[1][0] = 1;
      mint a = 0, b = 1;
      for (int k = 0; k <= 20; k++) {
        check(F.pow(k)[0][1] == a, "フィボナッチ");
        mint c = a + b;
        a = b;
        b = c;
      }
    }
    report("単位行列 / 累乗");
  }

  {  // ---- 行列式 ----
    ng = 0;
    for (int it = 0; it < 300; it++) {
      int n = 1 + (int)(rng() % 5);
      auto A = rnd(rng, n, n);
      check(A.det() == det_naive(A), "det が余因子展開と一致");
    }
    // 特異行列
    {
      matrix<mint> A(3, 3);
      for (int j = 0; j < 3; j++) A[0][j] = A[1][j] = j + 1;
      A[2][0] = 5;
      check(A.det() == 0, "同じ行があれば det = 0");
    }
    check(matrix<mint>::identity(4).det() == 1, "単位行列の det = 1");
    report("行列式");
  }

  {  // ---- 階数 ----
    ng = 0;
    for (int it = 0; it < 300; it++) {
      int n = 1 + (int)(rng() % 5), m = 1 + (int)(rng() % 5);
      auto A = rnd(rng, n, m);
      int r = A.rank();
      check(0 <= r && r <= min(n, m), "階数の範囲");
      // 階数 r <=> r+1 次の小行列式がすべて 0（n, m <= 5 なら det で確認できる）
      if (n == m) check((A.det() == 0) == (r < n), "det = 0 と階数落ちが一致");
    }
    {
      matrix<mint> Z(3, 4);
      check(Z.rank() == 0, "零行列の階数は 0");
      auto I = matrix<mint>::identity(3);
      check(I.rank() == 3, "単位行列の階数");
      matrix<mint> D(3, 3);  // 2 行が同じ
      D[0][0] = D[1][0] = 1;
      D[0][1] = D[1][1] = 2;
      check(D.rank() == 1, "同じ行は 1 つと数える");
    }
    report("階数");
  }

  {  // ---- 逆行列 ----
    ng = 0;
    for (int it = 0; it < 300; it++) {
      int n = 1 + (int)(rng() % 5);
      auto A = rnd(rng, n, n);
      auto B = A.inv();
      if (A.det() == 0) {
        check(B.h == 0, "特異なら空を返す");
      } else {
        check(B.h == n && B.w == n, "逆行列の形");
        auto C = A * B;
        for (int i = 0; i < n; i++)
          for (int j = 0; j < n; j++) check(C[i][j] == (i == j ? mint(1) : mint(0)), "A * inv(A) = I");
        auto D = B * A;
        for (int i = 0; i < n; i++)
          for (int j = 0; j < n; j++) check(D[i][j] == (i == j ? mint(1) : mint(0)), "inv(A) * A = I");
      }
    }
    report("逆行列");
  }

  {  // ---- 連立一次方程式 ----
    ng = 0;
    for (int it = 0; it < 400; it++) {
      int n = 1 + (int)(rng() % 5), m = 1 + (int)(rng() % 5);
      auto A = rnd(rng, n, m);

      // 解があるはずのケース（Ax から b を作る）
      vector<mint> x(m);
      for (auto& v : x) v = (int)(rng() % 5);
      vector<mint> b(n, 0);
      for (int i = 0; i < n; i++)
        for (int j = 0; j < m; j++) b[i] += A[i][j] * x[j];

      auto res = A.solve(b);
      check(!res.empty(), "解があるのに空を返した");
      if (res.empty()) continue;
      check((int)res[0].size() == m, "解の次元");
      for (int i = 0; i < n; i++) {  // 解が実際に満たすか
        mint s = 0;
        for (int j = 0; j < m; j++) s += A[i][j] * res[0][j];
        check(s == b[i], "Ax = b");
      }
      for (size_t t = 1; t < res.size(); t++)  // 核の基底は Av = 0
        for (int i = 0; i < n; i++) {
          mint s = 0;
          for (int j = 0; j < m; j++) s += A[i][j] * res[t][j];
          check(s == 0, "核の基底は Av = 0");
        }
      check((int)res.size() - 1 == m - A.rank(), "自由度の個数");

      // ランダムな b（解が無いこともある）
      vector<mint> rb(n);
      for (auto& v : rb) v = (int)(rng() % 5);
      auto r2 = A.solve(rb);
      if (!r2.empty()) {
        for (int i = 0; i < n; i++) {
          mint s = 0;
          for (int j = 0; j < m; j++) s += A[i][j] * r2[0][j];
          check(s == rb[i], "ランダム b でも解が正しい");
        }
      }
    }
    {  // 解なしの例
      matrix<mint> A(2, 1);
      A[0][0] = 1;
      A[1][0] = 1;
      check(A.solve({mint(1), mint(2)}).empty(), "解なしなら空");
      check(!A.solve({mint(3), mint(3)}).empty(), "解ありなら空でない");
    }
    report("連立一次方程式");
  }

  {  // ---- maxplus ----
    ng = 0;
    constexpr ll NEG = maxplus_matrix<ll>::NEG;
    for (int it = 0; it < 300; it++) {
      int n = 1 + (int)(rng() % 5);
      maxplus_matrix<ll> M(n);
      for (int i = 0; i < n; i++)
        for (int j = 0; j < n; j++)
          if (rng() % 3) M[i][j] = (ll)(rng() % 20) - 10;

      int k = (int)(rng() % 8);
      auto P = M.pow(k);
      auto Q = maxplus_matrix<ll>::identity(n);
      for (int t = 0; t < k; t++) Q = Q * M;
      for (int i = 0; i < n; i++)
        for (int j = 0; j < n; j++) check(P[i][j] == Q[i][j], "pow");

      // k 辺での最長路を素朴な DP と比べる
      vector<vector<ll>> dp(n, vector<ll>(n, NEG));
      for (int i = 0; i < n; i++) dp[i][i] = 0;
      for (int t = 0; t < k; t++) {
        vector<vector<ll>> nx(n, vector<ll>(n, NEG));
        for (int i = 0; i < n; i++)
          for (int u = 0; u < n; u++) {
            if (dp[i][u] == NEG) continue;
            for (int v = 0; v < n; v++) {
              if (M[u][v] == NEG) continue;
              nx[i][v] = max(nx[i][v], dp[i][u] + M[u][v]);
            }
          }
        dp = nx;
      }
      for (int i = 0; i < n; i++)
        for (int j = 0; j < n; j++) check(P[i][j] == dp[i][j], "k 辺の最長路");

      // ベクトル x 行列
      vector<ll> v(n, NEG);
      int s0 = (int)(rng() % n);
      v[s0] = 0;
      auto vP = v * P;
      for (int j = 0; j < n; j++) check(vP[j] == P[s0][j], "v * P は行の取り出し");
      auto l2 = (v * P) * M, r2 = v * (P * M);
      for (int j = 0; j < n; j++) check(l2[j] == r2[j], "結合則");
    }
    {  // 符号反転で最短路になるか
      int n = 4;
      vector<vector<ll>> w(n, vector<ll>(n, -1));
      w[0][1] = 3, w[1][2] = 4, w[0][2] = 10, w[2][3] = 2, w[1][3] = 20;
      maxplus_matrix<ll> Mx(n), Mn(n);
      for (int i = 0; i < n; i++)
        for (int j = 0; j < n; j++)
          if (w[i][j] >= 0) {
            Mx[i][j] = w[i][j];
            Mn[i][j] = -w[i][j];  // 符号反転
          }
      auto Px = Mx.pow(2), Pn = Mn.pow(2);
      check(Px[0][3] == 23, "2 辺での最長路 0->1->3 = 3+20");
      check(-Pn[0][3] == 12, "2 辺での最短路 0->2->3 = 10+2");
    }
    {  // 到達不能
      maxplus_matrix<ll> M(2);
      M[0][0] = 1;
      auto P = M.pow(3);
      check(P[0][1] == NEG && P[1][0] == NEG, "到達不能は NEG のまま");
      check(P[0][0] == 3 && P[1][1] == NEG, "自己ループ");
    }
    report("maxplus_matrix");
  }

#ifdef _GLIBCXX_DEBUG
  puts("速度計測                       : _GLIBCXX_DEBUG のため省略");
#else
  {  // ---- 速度 ----
    auto ms = [](auto s, auto e) { return (ll)chrono::duration_cast<chrono::milliseconds>(e - s).count(); };
    {
      int n = 200;
      auto A = rnd(rng, n, n, 998244353);
      auto st = chrono::steady_clock::now();
      auto B = A * A;
      printf("%-30s : %lld ms\n", "速度 200x200 の積", ms(st, chrono::steady_clock::now()));
      (void)B;
    }
    {
      int n = 200;
      auto A = rnd(rng, n, n, 998244353);
      auto st = chrono::steady_clock::now();
      auto d = A.det();
      auto r = A.rank();
      printf("%-30s : %lld ms  (rank %d)\n", "速度 200x200 の det + rank", ms(st, chrono::steady_clock::now()), r);
      (void)d;
    }
    {
      int n = 60;
      auto A = rnd(rng, n, n, 998244353);
      auto st = chrono::steady_clock::now();
      auto P = A.pow(1000000000000LL);
      printf("%-30s : %lld ms\n", "速度 60x60 の 10^12 乗", ms(st, chrono::steady_clock::now()));
      (void)P;
    }
  }
#endif

  printf(ng_total ? "\nNG %d 件\n" : "\nすべて OK\n", ng_total);
  return ng_total ? 1 : 0;
}
