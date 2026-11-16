// data_structure/disjoint_sparse_table.hpp の検証。総当たりと突き合わせる。
//   g++ -std=gnu++20 -O2 -Wall -Wextra -I.. disjoint_sparse_table_test.cpp -o disjoint_sparse_table_test
#include "../../data_structure/disjoint_sparse_table.hpp"

#include <cstdio>
#include <numeric>
#include <random>
#include <string>
#include <vector>

using ll = long long;
using namespace std;

int failed = 0;
void check(bool ok, const char* what) {
  if (!ok) printf("  NG: %s\n", what), failed++;
}

ll add(ll a, ll b) { return a + b; }
ll mn(ll a, ll b) { return min(a, b); }
ll gc(ll a, ll b) { return gcd(a, b); }
// 非可換：一次関数の合成。左を先に通す
constexpr ll M = 998244353;
struct F {
  ll a, b;
  bool operator==(const F&) const = default;
};
F comp(F l, F r) { return F{r.a * l.a % M, (r.a * l.b + r.b) % M}; }
// 非可換で冪等でもない：文字列の連結
string cat(string a, string b) { return a + b; }

template <class S, S (*op)(S, S)> bool all_ranges(const vector<S>& a) {
  disjoint_sparse_table<S, op> d(a);
  if (d.size() != (int)a.size()) return false;
  for (int l = 0; l < (int)a.size(); l++) {
    if (d.get(l) != a[l]) return false;
    S acc = a[l];
    for (int r = l + 1; r <= (int)a.size(); r++) {
      if (r > l + 1) acc = op(acc, a[r - 1]);
      if (d.prod(l, r) != acc) return false;
    }
  }
  return true;
}

int main() {
  mt19937_64 rng(20261116);

  {  // ---- 長さ 0〜70 の全区間（2 の冪の前後を含む）----
    int bad = 0;
    for (int n = 0; n <= 70; n++)
      for (int t = 0; t < 5; t++) {
        vector<ll> a(n);
        for (auto& x : a) x = ll(rng() % 2001) - 1000;
        vector<ll> g(n);
        for (auto& x : g) x = ll(rng() % 6 + 1) * ll(rng() % 6 + 1);
        vector<F> f(n);
        for (auto& x : f) x = F{ll(rng() % M), ll(rng() % M)};
        vector<string> s(n);
        for (auto& x : s) x = string(1, char('a' + rng() % 26));
        bad += !all_ranges<ll, add>(a) + !all_ranges<ll, mn>(a) + !all_ranges<ll, gc>(g);
        bad += !all_ranges<F, comp>(f) + !all_ranges<string, cat>(s);
      }
    check(bad == 0, "全区間（和 / min / gcd / 一次関数の合成 / 文字列連結）");
    printf("全区間             : %s\n", bad ? "NG" : "OK");
  }

  {  // ---- 長い列でランダムな区間 ----
    int bad = 0, n = 100000;
    vector<F> f(n);
    for (auto& x : f) x = F{ll(rng() % M), ll(rng() % M)};
    disjoint_sparse_table<F, comp> d(f);
    for (int q = 0; q < 300; q++) {
      int l = int(rng() % n), r = int(rng() % n);
      if (l > r) swap(l, r);
      r++;
      F acc = f[l];
      for (int i = l + 1; i < r; i++) acc = comp(acc, f[i]);
      bad += !(d.prod(l, r) == acc);
    }
    check(bad == 0, "n = 10^5 の非可換");
    printf("長い列             : %s\n", bad ? "NG" : "OK");
  }

  {  // ---- 既定構築 ----
    disjoint_sparse_table<ll, add> d;
    check(d.size() == 0, "既定構築は空");
    printf("既定構築           : %s\n", d.size() == 0 ? "OK" : "NG");
  }

  printf(failed ? "\nNG %d 件\n" : "\nすべて OK\n", failed);
  return failed ? 1 : 0;
}
