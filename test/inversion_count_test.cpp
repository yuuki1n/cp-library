// util/inversion_count.hpp の検証。総当たりと突き合わせる。
//   g++ -std=gnu++20 -O2 -Wall -Wextra -I.. inversion_count_test.cpp -o inversion_count_test
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <numeric>
#include <random>
#include <string>
#include <vector>

#include "../util/inversion_count.hpp"

using ll = long long;
using namespace std;

int failed = 0;
void check(bool ok, const char* what) {
  if (!ok) {
    printf("  NG: %s\n", what);
    failed++;
  }
}

template <class T>
ll brute(const vector<T>& a) {
  ll c = 0;
  for (size_t i = 0; i < a.size(); i++)
    for (size_t j = i + 1; j < a.size(); j++)
      if (a[j] < a[i]) c++;
  return c;
}

int main() {
  mt19937_64 rng(20260901);

  // ---- 総当たりとの照合（重複あり・少ない値域）----
  {
    int bad = 0;
    for (int t = 0; t < 20000; t++) {
      int n = int(rng() % 13);
      vector<ll> a(n);
      for (auto& x : a) x = ll(rng() % 5);
      if (inversion_count(a) != brute(a)) bad++;
    }
    printf("重複ありランダム         : %s\n", bad == 0 ? "OK" : "NG");
    failed += bad ? 1 : 0;
  }

  // ---- 総当たりとの照合（値域が広い・負数を含む）----
  {
    int bad = 0;
    for (int t = 0; t < 20000; t++) {
      int n = int(rng() % 13);
      vector<ll> a(n);
      for (auto& x : a) x = ll(rng() % 2000000001) - 1000000000;
      if (inversion_count(a) != brute(a)) bad++;
    }
    printf("広い値域 / 負数          : %s\n", bad == 0 ? "OK" : "NG");
    failed += bad ? 1 : 0;
  }

  // ---- 引数が壊れないか ----
  {
    vector<ll> a = {3, 1, 2};
    vector<ll> before = a;
    (void)inversion_count(a);
    check(a == before, "引数の配列が書き換えられた");
    printf("引数を壊さない           : OK\n");
  }

  // ---- 境界 ----
  {
    check(inversion_count(vector<ll>{}) == 0, "空");
    check(inversion_count(vector<ll>{7}) == 0, "1 要素");
    check(inversion_count(vector<ll>{1, 2}) == 0, "昇順 2 要素");
    check(inversion_count(vector<ll>{2, 1}) == 1, "降順 2 要素");
    check(inversion_count(vector<ll>{5, 5, 5}) == 0, "全部同値");
    printf("境界ケース               : OK\n");
  }

  // ---- 昇順 / 降順の理論値 ----
  {
    int n = 1000;
    vector<ll> asc(n), desc(n);
    iota(asc.begin(), asc.end(), 0);
    for (int i = 0; i < n; i++) desc[i] = n - i;
    check(inversion_count(asc) == 0, "昇順は 0");
    check(inversion_count(desc) == ll(n) * (n - 1) / 2, "降順は N(N-1)/2");
    printf("昇順 / 降順の理論値      : OK\n");
  }

  // ---- ll が必要な規模 ----
  {
    int n = 200000;
    vector<ll> a(n);
    for (int i = 0; i < n; i++) a[i] = n - i;
    auto t0 = chrono::steady_clock::now();
    ll r = inversion_count(a);
    auto ms = chrono::duration_cast<chrono::milliseconds>(
                  chrono::steady_clock::now() - t0)
                  .count();
    check(r == ll(n) * (n - 1) / 2, "N=2e5 降順の値");
    printf("N=2e5 降順               : OK (%lld, %lld ms)\n", r, (ll)ms);
  }

  // ---- ll 以外の型 ----
  {
    vector<int> vi = {3, 1, 2};
    check(inversion_count(vi) == 2, "vector<int>");
    vector<string> vs = {"c", "a", "b"};
    check(inversion_count(vs) == 2, "vector<string>");
    vector<pair<int, int>> vp = {{2, 0}, {1, 5}, {1, 3}};
    check(inversion_count(vp) == 3, "vector<pair>");
    printf("int / string / pair      : OK\n");
  }

  printf("\n%s\n", failed == 0 ? "すべて OK" : "失敗あり");
  return failed == 0 ? 0 : 1;
}
