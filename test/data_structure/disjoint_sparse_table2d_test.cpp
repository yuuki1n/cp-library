// data_structure/disjoint_sparse_table2d.hpp の検証。総当たりと突き合わせる。
//   g++ -std=gnu++20 -O2 -Wall -Wextra -I.. disjoint_sparse_table2d_test.cpp -o disjoint_sparse_table2d_test
#include "../../data_structure/disjoint_sparse_table2d.hpp"

#include <cstdio>
#include <numeric>
#include <random>
#include <vector>

using ll = long long;
using namespace std;

int failed = 0;
void check(bool ok, const char* what) {
  if (!ok) printf("  NG: %s\n", what), failed++;
}

ll add(ll a, ll b) { return a + b; }
ll mx(ll a, ll b) { return max(a, b); }
ll gc(ll a, ll b) { return gcd(a, b); }
ll xr(ll a, ll b) { return a ^ b; }
constexpr ll M = 998244353;
ll mul(ll a, ll b) { return a * b % M; }

template <ll (*op)(ll, ll)> bool all_rects(const vector<vector<ll>>& g) {
  int h = (int)g.size(), w = h ? (int)g[0].size() : 0;
  disjoint_sparse_table2d<ll, op> d(g);
  if (d.h() != h || (h && d.w() != w)) return false;
  for (int i = 0; i < h; i++)
    for (int j = 0; j < w; j++)
      if (d.get(i, j) != g[i][j]) return false;
  for (int i0 = 0; i0 < h; i0++)
    for (int j0 = 0; j0 < w; j0++) {
      // [i0, i1) x [j0, j1) を、行ごとの累積 col[j] を使って総当たりで作る
      vector<ll> col(g[i0].begin(), g[i0].end());
      for (int i1 = i0 + 1; i1 <= h; i1++) {
        if (i1 > i0 + 1)
          for (int j = j0; j < w; j++) col[j] = op(col[j], g[i1 - 1][j]);
        ll acc = col[j0];
        for (int j1 = j0 + 1; j1 <= w; j1++) {
          if (j1 > j0 + 1) acc = op(acc, col[j1 - 1]);
          if (d.prod(i0, j0, i1, j1) != acc) return false;
        }
      }
    }
  return true;
}

int main() {
  mt19937_64 rng(20261116);

  {  // ---- 0〜20 の全サイズで全長方形 ----
    int bad = 0;
    for (int h = 0; h <= 20; h++)
      for (int w = 0; w <= 20; w++) {
        if (h == 0 && w) continue;
        vector<vector<ll>> a(h, vector<ll>(w)), g = a, p = a;
        for (auto& r : a)
          for (auto& x : r) x = ll(rng() % 2001) - 1000;
        for (auto& r : g)
          for (auto& x : r) x = ll(rng() % 6 + 1) * ll(rng() % 6 + 1);
        for (auto& r : p)
          for (auto& x : r) x = ll(rng() % M);
        bad += !all_rects<add>(a) + !all_rects<mx>(a) + !all_rects<gc>(g) + !all_rects<xr>(a) + !all_rects<mul>(p);
      }
    check(bad == 0, "全長方形（和 / max / gcd / xor / mod 積）");
    printf("全長方形           : %s\n", bad ? "NG" : "OK");
  }

  {  // ---- 細長い配列（片方が 1 のときと、2 の冪の境目）----
    int bad = 0;
    for (auto [h, w] : vector<pair<int, int>>{{1, 300}, {300, 1}, {3, 130}, {130, 3}, {64, 65}, {65, 64}}) {
      vector<vector<ll>> a(h, vector<ll>(w));
      for (auto& r : a)
        for (auto& x : r) x = ll(rng() % 2001) - 1000;
      disjoint_sparse_table2d<ll, add> d(a);
      for (int q = 0; q < 3000; q++) {
        int i0 = int(rng() % h), i1 = int(rng() % h), j0 = int(rng() % w), j1 = int(rng() % w);
        if (i0 > i1) swap(i0, i1);
        if (j0 > j1) swap(j0, j1);
        ll acc = 0;
        for (int i = i0; i <= i1; i++)
          for (int j = j0; j <= j1; j++) acc += a[i][j];
        bad += d.prod(i0, j0, i1 + 1, j1 + 1) != acc;
      }
    }
    check(bad == 0, "細長い配列");
    printf("細長い配列         : %s\n", bad ? "NG" : "OK");
  }

  printf(failed ? "\nNG %d 件\n" : "\nすべて OK\n", failed);
  return failed ? 1 : 0;
}
