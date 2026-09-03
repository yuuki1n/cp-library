// data_structure/cumsum2d.hpp の検証。総当たりと突き合わせる。
//   g++ -std=gnu++20 -O2 -Wall -Wextra -I.. cumsum2d_test.cpp -o cumsum2d_test
#include <cstdio>
#include <random>
#include <vector>

#include "../data_structure/cumsum2d.hpp"

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

  // ---- 全矩形を総当たりで照合 ----
  {
    int bad = 0;
    for (int t = 0; t < 500; t++) {
      int h = int(rng() % 12), w = int(rng() % 12);
      if (h == 0) w = 0;
      vector<vector<int>> g(h, vector<int>(w));
      for (auto& row : g)
        for (auto& x : row) x = int(rng() % 201) - 100;
      CumSum2D<ll> cs(g);
      if ((int)cs.h() != h || (int)cs.w() != w) bad++;
      for (int i1 = 0; i1 <= h; i1++)
        for (int i2 = i1; i2 <= h; i2++)
          for (int j1 = 0; j1 <= w; j1++)
            for (int j2 = j1; j2 <= w; j2++) {
              ll acc = 0;
              for (int i = i1; i < i2; i++)
                for (int j = j1; j < j2; j++) acc += g[i][j];
              if (cs(i1, j1, i2, j2) != acc) bad++;
            }
      for (int i2 = 0; i2 <= h; i2++)
        for (int j2 = 0; j2 <= w; j2++) {
          ll acc = 0;
          for (int i = 0; i < i2; i++)
            for (int j = 0; j < j2; j++) acc += g[i][j];
          if (cs(i2, j2) != acc) bad++;
        }
      ll tot = 0;
      for (auto& row : g)
        for (int x : row) tot += x;
      if (cs.all_sum() != tot) bad++;
    }
    printf("全矩形の総当たり照合     : %s\n", bad == 0 ? "OK" : "NG");
    failed += bad ? 1 : 0;
  }

  // ---- 空 / デフォルト構築 ----
  {
    CumSum2D<ll> cs;
    check(cs.h() == 0 && cs.w() == 0 && cs.all_sum() == 0, "デフォルト構築");
    vector<vector<int>> e;
    CumSum2D<ll> cs2(e);
    check(cs2.h() == 0 && cs2.w() == 0 && cs2.all_sum() == 0, "空 vector から構築");
    printf("空 / デフォルト構築      : OK\n");
  }

  // ---- int の grid でも T で累積されるか ----
  {
    vector<vector<int>> big(400, vector<int>(400, 1000000000));
    CumSum2D<ll> cs(big);
    check(cs.all_sum() == 400LL * 400LL * 1000000000LL, "int の grid で溢れない");
    printf("int の grid -> ll 累積   : OK (%lld)\n", cs.all_sum());
  }

  printf("\n%s\n", failed == 0 ? "すべて OK" : "失敗あり");
  return failed == 0 ? 0 : 1;
}
