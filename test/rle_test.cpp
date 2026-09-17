// util/rle.hpp の検証。素朴な実装と突き合わせる。
//   g++ -std=gnu++20 -O2 -Wall -Wextra -I.. rle_test.cpp -o rle_test
#include "../util/rle.hpp"

#include <array>
#include <chrono>
#include <cstdio>
#include <deque>
#include <random>
#include <ranges>
#include <string>
#include <vector>

using ll = long long;
using namespace std;

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

int main() {
  mt19937 rng(20260916);

  {  // ---- vector<int> ----
    ng = 0;
    for (int it = 0; it < 2000; it++) {
      int n = (int)(rng() % 12);
      vector<int> v(n);
      for (auto& x : v) x = (int)(rng() % 3);

      auto r = rle(v);
      // 連続が正しくまとまっているか
      for (size_t i = 0; i + 1 < r.size(); i++)
        check(r[i].first != r[i + 1].first, "隣り合う値は異なる");
      ll tot = 0;
      for (auto& [x, c] : r) {
        check(c >= 1, "個数は 1 以上");
        tot += c;
      }
      check(tot == n, "個数の合計が元の長さ");
      check(rle_decode(r) == v, "展開すると元に戻る");
      check(r.empty() == v.empty(), "空なら空");
    }
    report("vector<int>");
  }

  {  // ---- string ----
    ng = 0;
    for (int it = 0; it < 2000; it++) {
      int n = (int)(rng() % 12);
      string s;
      for (int i = 0; i < n; i++) s += char('a' + rng() % 3);

      auto r = rle(s);
      // 素朴に作ったものと比べる
      vector<pair<char, ll>> want;
      for (char c : s) {
        if (!want.empty() && want.back().first == c)
          want.back().second++;
        else
          want.emplace_back(c, 1);
      }
      check(r == want, "素朴な実装と一致");
      string back;
      for (auto& [c, k] : r) back += string((size_t)k, c);
      check(back == s, "展開すると元に戻る");
    }
    report("string");
  }

  {  // ---- 具体例 / いろいろな型 ----
    ng = 0;
    {
      auto r = rle(string("aaabbc"));
      vector<pair<char, ll>> want{{'a', 3}, {'b', 2}, {'c', 1}};
      check(r == want, "aaabbc");
    }
    {
      auto r = rle(vector<int>{1, 1, 2, 3, 3, 3});
      vector<pair<int, ll>> want{{1, 2}, {2, 1}, {3, 3}};
      check(r == want, "1 1 2 3 3 3");
    }
    {
      deque<int> d{5, 5, 7};
      auto r = rle(d);
      check(r.size() == 2 && r[0] == make_pair(5, 1LL * 2), "deque");
    }
    {
      array<char, 4> a{'x', 'x', 'y', 'y'};
      auto r = rle(a);
      check(r.size() == 2 && r[1].second == 2, "array");
    }
    {
      vector<int> v{1, 1, 2, 3, 3, 3};
      auto r = rle(v | views::drop(1));
      check(r.size() == 3 && r[0].second == 1, "views で切った範囲");
    }
    {
      check(rle(string("")).empty(), "空文字列");
      check(rle(vector<int>{}).empty(), "空 vector");
      auto one = rle(vector<ll>{7});
      check(one.size() == 1 && one[0] == make_pair(7LL, 1LL), "1 要素");
      auto same = rle(vector<int>(5, 9));
      check(same.size() == 1 && same[0].second == 5, "全部同じ");
    }
    {  // 個数が int を超える
      vector<pair<char, ll>> big{{'a', 3000000000LL}};
      check(big[0].second > 2147483647LL, "個数は long long で持つ");
    }
    report("具体例 / いろいろな型");
  }

#ifdef _GLIBCXX_DEBUG
  puts("速度計測                       : _GLIBCXX_DEBUG のため省略");
#else
  {  // ---- 速度 ----
    const int N = 2000000;
    string s(N, 'a');
    for (int i = 0; i < N; i++) s[i] = char('a' + rng() % 2);
    auto st = chrono::steady_clock::now();
    auto r = rle(s);
    printf("%-30s : %lld ms  (%zu 区間)\n", "速度 長さ 2e6",
           (ll)chrono::duration_cast<chrono::milliseconds>(
               chrono::steady_clock::now() - st)
               .count(),
           r.size());
  }
#endif

  printf(ng_total ? "\nNG %d 件\n" : "\nすべて OK\n", ng_total);
  return ng_total ? 1 : 0;
}
