// data_structure/cumsum.hpp の検証。総当たりと突き合わせる。
//   g++ -std=gnu++20 -O2 -Wall -Wextra -I.. cumsum_test.cpp -o cumsum_test
#include "../data_structure/cumsum.hpp"

#include <cstdio>
#include <random>
#include <vector>

using ll = long long;
using namespace std;

int failed = 0;
void check(bool ok, const char* what) {
  if (!ok) {
    printf("  NG: %s\n", what);
    failed++;
  }
}

int main() {
  mt19937_64 rng(20260831);

  // ---- vector から構築 ----
  {
    int bad = 0;
    for (int t = 0; t < 2000; t++) {
      int n = int(rng() % 30);
      vector<int> a(n);
      for (auto& x : a) x = int(rng() % 2001) - 1000;
      cumsum<ll> cs(a);
      if ((int)cs.size() != n) bad++;
      for (int l = 0; l <= n; l++) {
        ll acc = 0;
        for (int r = l; r <= n; r++) {
          if (cs(l, r) != acc) bad++;
          if (r < n) acc += a[r];
        }
      }
      for (int r = 0; r <= n; r++) {
        ll acc = 0;
        for (int i = 0; i < r; i++) acc += a[i];
        if (cs(r) != acc) bad++;
      }
      for (int i = 0; i < n; i++)
        if (cs.at(i) != a[i]) bad++;
      ll tot = 0;
      for (int x : a) tot += x;
      if (cs.all_sum() != tot) bad++;
      if (cs.empty() != (n == 0)) bad++;
    }
    printf("vector から構築          : %s\n", bad == 0 ? "OK" : "NG");
    failed += bad ? 1 : 0;
  }

  // ---- 空 / 長さ指定 + push_back / operator+= ----
  {
    int bad = 0;
    for (int t = 0; t < 2000; t++) {
      int n = int(rng() % 30);
      bool useLen = rng() & 1;
      cumsum<ll> cs = useLen ? cumsum<ll>(size_t(rng() % 10)) : cumsum<ll>();
      vector<int> a;
      for (int i = 0; i < n; i++) {
        int x = int(rng() % 2001) - 1000;
        if (i & 1)
          cs += x;
        else
          cs.push_back(x);
        a.push_back(x);
      }
      if ((int)cs.size() != n) bad++;
      for (int l = 0; l <= n; l++) {
        ll acc = 0;
        for (int r = l; r <= n; r++) {
          if (cs(l, r) != acc) bad++;
          if (r < n) acc += a[r];
        }
      }
    }
    printf("push_back / operator+=   : %s\n", bad == 0 ? "OK" : "NG");
    failed += bad ? 1 : 0;
  }

  // ---- clear / reserve ----
  {
    cumsum<ll> cs;
    cs += 5;
    cs += 7;
    check(cs.all_sum() == 12, "clear 前の合計");
    cs.clear();
    check(cs.size() == 0 && cs.empty() && cs.all_sum() == 0, "clear 後");
    cs.reserve(100);
    cs += 3;
    check(cs.size() == 1 && cs(0, 1) == 3, "reserve 後の push_back");
    printf("clear / reserve          : OK\n");
  }

  // ---- int の vector でも T で累積されるか ----
  {
    vector<int> big(200000, 1000000000);
    cumsum<ll> cs(big);
    check(cs.all_sum() == 200000LL * 1000000000LL, "int の vector で溢れない");
    printf("int の vector -> ll 累積 : OK (%lld)\n", cs.all_sum());
  }

  printf("\n%s\n", failed == 0 ? "すべて OK" : "失敗あり");
  return failed == 0 ? 0 : 1;
}
