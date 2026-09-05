// data_structure/interval_set.hpp の検証。set<int> による総当たりと突き合わせる。
//   g++ -std=gnu++20 -O2 -Wall -Wextra -I.. interval_set_test.cpp -o interval_set_test
#include "../data_structure/interval_set.hpp"

#include <chrono>
#include <climits>
#include <cstdio>
#include <random>
#include <set>
#include <utility>
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

// 正規形（昇順・交わらない・隣接しない・空でない）を保っているか
bool canonical(const interval_set& s) {
  ll prev_r = LLONG_MIN, tot = 0;
  for (auto [l, r] : s.intervals()) {
    if (l >= r) return false;                              // 空区間が残っている
    if (prev_r != LLONG_MIN && l <= prev_r) return false;  // 交差 or 隣接
    prev_r = r;
    tot += r - l;
  }
  return tot == s.size();
}

int main() {
  mt19937_64 rng(20260901);

  // ---- ランダム操作を set<int> と照合 ----
  {
    const int LIM = 50;
    int bad = 0;
    for (int t = 0; t < 3000; t++) {
      interval_set s;
      set<int> b;
      for (int q = 0; q < 40; q++) {
        int l = int(rng() % LIM), r = int(rng() % LIM);
        if (l > r) swap(l, r);
        if (rng() & 1) r = l + 1;  // 単点も混ぜる
        int op = int(rng() % 3);
        if (op == 0) {
          ll got = s.insert(l, r);
          ll want = 0;
          for (int x = l; x < r; x++)
            if (b.insert(x).second) want++;
          if (got != want) bad++;
        } else if (op == 1) {
          ll got = s.erase(l, r);
          ll want = 0;
          for (int x = l; x < r; x++) want += b.erase(x);
          if (got != want) bad++;
        } else {
          // 単点版の insert / erase
          if (rng() & 1) {
            ll got = s.insert(l);
            if (got != (ll)b.insert(l).second) bad++;
          } else {
            ll got = s.erase(l);
            if (got != (ll)b.erase(l)) bad++;
          }
        }
        if (s.size() != (ll)b.size()) bad++;
        if (s.empty() != b.empty()) bad++;
        if (!canonical(s)) bad++;
        for (int x = -2; x < LIM + 2; x++)
          if (s.contains(x) != (bool)b.count(x)) bad++;
      }
    }
    printf("insert / erase / contains  : %s\n", bad == 0 ? "OK" : "NG");
    failed += bad ? 1 : 0;
  }

  // ---- mex ----
  {
    const int LIM = 40;
    int bad = 0;
    for (int t = 0; t < 3000; t++) {
      interval_set s;
      set<int> b;
      for (int q = 0; q < 15; q++) {
        int l = int(rng() % LIM), r = l + int(rng() % 5) + 1;
        if (rng() & 1) {
          s.insert(l, r);
          for (int x = l; x < r; x++) b.insert(x);
        } else {
          s.erase(l, r);
          for (int x = l; x < r; x++) b.erase(x);
        }
        for (int from = 0; from < LIM; from++) {
          ll want = from;
          while (b.count((int)want)) want++;
          if (s.mex(from) != want) bad++;
        }
      }
    }
    printf("mex                        : %s\n", bad == 0 ? "OK" : "NG");
    failed += bad ? 1 : 0;
  }

  // ---- same（同じ区間に属するか）----
  {
    const int LIM = 30;
    int bad = 0;
    for (int t = 0; t < 2000; t++) {
      interval_set s;
      vector<pair<int, int>> iv;
      for (int q = 0; q < 5; q++) {
        int l = int(rng() % LIM), r = l + int(rng() % 6) + 1;
        s.insert(l, r);
        iv.emplace_back(l, r);
      }
      for (int x = 0; x < LIM; x++)
        for (int y = 0; y < LIM; y++) {
          // 総当たり: x..y が全部覆われていれば同じ区間
          int lo = min(x, y), hi = max(x, y);
          bool all_cov = true;
          for (int z = lo; z <= hi; z++)
            if (!s.contains(z)) all_cov = false;
          bool want = s.contains(x) && s.contains(y) && all_cov;
          if (s.same(x, y) != want) bad++;
        }
    }
    printf("same                       : %s\n", bad == 0 ? "OK" : "NG");
    failed += bad ? 1 : 0;
  }

  // ---- 隣接区間の併合 ----
  {
    interval_set s;
    check(s.insert(0, 2) == 2, "insert(0,2) の戻り値");
    check(s.insert(2, 5) == 3, "insert(2,5) の戻り値");
    check(s.intervals().size() == 1, "隣接区間が 1 本に併合されない");
    check(
        s.intervals().begin()->first == 0 && s.intervals().begin()->second == 5,
        "[0,5) になる");
    check(s.size() == 5, "size");
    check(s.insert(1, 3) == 0, "既に覆われた範囲の insert は 0");
    printf("隣接区間の併合             : OK\n");
  }

  // ---- 内側を erase して分割 ----
  {
    interval_set s;
    s.insert(0, 10);
    check(s.erase(3, 5) == 2, "erase(3,5) の戻り値");
    check(s.intervals().size() == 2, "2 本に分かれない");
    check(
        s.intervals().begin()->first == 0 && s.intervals().begin()->second == 3,
        "左が [0,3)");
    check(s.intervals().rbegin()->first == 5 &&
              s.intervals().rbegin()->second == 10,
          "右が [5,10)");
    check(s.size() == 8, "size");
    check(s.erase(3, 5) == 0, "覆われていない範囲の erase は 0");
    printf("内側 erase による分割      : OK\n");
  }

  // ---- 複数区間をまたぐ操作 ----
  {
    interval_set s;
    s.insert(0, 2);
    s.insert(4, 6);
    s.insert(8, 10);
    check(s.intervals().size() == 3 && s.size() == 6, "3 本ある");
    check(s.insert(1, 9) == 4, "またぐ insert の戻り値（2,3,6,7 が新規）");
    check(s.intervals().size() == 1 && s.size() == 10, "1 本 [0,10) に併合");
    check(s.erase(1, 9) == 8, "またぐ erase の戻り値");
    check(s.intervals().size() == 2 && s.size() == 2, "[0,1) と [9,10) が残る");
    printf("複数区間をまたぐ操作       : OK\n");
  }

  // ---- 境界・空・l >= r ----
  {
    interval_set s;
    check(s.size() == 0 && s.empty(), "初期状態");
    check(s.mex() == 0, "空の mex");
    check(!s.contains(0), "空の contains");
    check(s.find(0) == s.intervals().end(), "空の find");
    check(!s.same(0, 0), "空の same");
    check(s.insert(5, 5) == 0 && s.empty(), "l == r の insert");
    check(s.insert(5, 3) == 0 && s.empty(), "l > r の insert");
    check(s.erase(5, 5) == 0, "l == r の erase");
    check(s.erase(0, 100) == 0, "空からの erase");
    s.insert(3);
    check(s.size() == 1 && s.contains(3) && !s.contains(2) && !s.contains(4),
          "単点 insert");
    check(s.mex() == 0 && s.mex(3) == 4, "単点の mex");
    s.clear();
    check(s.empty() && s.intervals().empty(), "clear");
    printf("境界 / 空 / l >= r         : OK\n");
  }

  // ---- 10^18 級の座標 ----
  {
    interval_set s;
    const ll B = 1000000000000000000LL;
    check(s.insert(B, B + 5) == 5, "大きい座標の insert");
    check(s.insert(-B - 5, -B) == 5, "負の大きい座標");
    check(s.size() == 10, "size");
    check(s.contains(B + 3) && !s.contains(B + 5), "contains");
    check(s.mex(B) == B + 5, "mex");
    check(s.erase(B + 1, B + 3) == 2, "erase");
    check(s.size() == 8 && s.intervals().size() == 3, "分割後");
    printf("10^18 級の座標             : OK\n");
  }

  // ---- 速度 ----
  {
    interval_set s;
    const int Q = 200000;
    vector<pair<ll, ll>> qs(Q);
    for (auto& q : qs) {
      ll l = ll(rng() % 1000000000LL);
      q = {l, l + ll(rng() % 1000) + 1};
    }
    auto t0 = chrono::steady_clock::now();
    for (auto [l, r] : qs) s.insert(l, r);
    auto t1 = chrono::steady_clock::now();
    for (auto [l, r] : qs) s.erase(l, r);
    auto t2 = chrono::steady_clock::now();
    printf("速度 insert x2e5           : %lld ms\n",
           (ll)chrono::duration_cast<chrono::milliseconds>(t1 - t0).count());
    printf("速度 erase  x2e5           : %lld ms  (残り %lld / 区間 %zu)\n",
           (ll)chrono::duration_cast<chrono::milliseconds>(t2 - t1).count(),
           s.size(), s.intervals().size());
    check(canonical(s), "速度テスト後も正規形");
  }

  printf("\n%s\n", failed == 0 ? "すべて OK" : "失敗あり");
  return failed == 0 ? 0 : 1;
}
