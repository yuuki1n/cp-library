// util/digit_dp.hpp の検証。素朴な全探索と突き合わせる。
//   g++ -std=gnu++20 -O2 -Wall -Wextra -I.. digit_dp_test.cpp -o digit_dp_test
#include "../../util/digit_dp.hpp"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <numeric>
#include <random>
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
  printf("%-40s : %s\n", name.c_str(), ng ? "NG" : "OK");
  ng = 0;
}

// 基数 base で x を表した桁の列（先頭から）
vector<int> digs(ll x, int base) {
  if (x == 0) return {0};
  vector<int> d;
  while (x) d.push_back(int(x % base)), x /= base;
  reverse(d.begin(), d.end());
  return d;
}
string to_str(ll x, int base) {
  string s;
  for (int d : digs(x, base)) s += char('0' + d);
  return s;
}

// n の桁数にゼロ埋めした上で、先頭のゼロを除いた開始位置
int first_nz(const vector<int>& d) {
  int f = 0;
  while (f + 1 < (int)d.size() && d[f] == 0) f++;
  return f;
}
vector<int> pad(ll x, int len, int base) {
  vector<int> d = digs(x, base), r(len, 0);
  for (int i = 0; i < (int)d.size(); i++) r[len - (int)d.size() + i] = d[i];
  return r;
}

int main() {
  {  // 桁和 mod M の個数。全探索と突き合わせる
    for (int M = 2; M <= 5; M++)
      for (ll N : {0LL, 1LL, 9LL, 10LL, 99LL, 100LL, 777LL, 1234LL, 5000LL}) {
        auto r = digit_dp<ll>(to_str(N, 10), M, 0, [M](int s, int d) { return (s + d) % M; });
        vector<ll> want(M, 0);
        for (ll x = 0; x <= N; x++) {
          ll t = 0;
          for (int d : digs(x, 10)) t += d;
          want[t % M]++;
        }
        for (int s = 0; s < M; s++)
          check(r[s] == want[s], "桁和 mod " + to_string(M) + " N=" + to_string(N) + " s=" + to_string(s));
      }
    report("桁和 mod M の個数");
  }

  {  // 隣り合う桁が同じでない。lead の扱いを見る
    for (ll N = 0; N <= 3000; N += 7) {
      auto r = digit_dp<ll>(to_str(N, 10), 10, 0, [](int s, int d, bool first) {
        return first ? d : (s == d ? -1 : d);
      });
      ll got = accumulate(r.begin(), r.end(), 0LL);
      ll want = 0;
      for (ll x = 0; x <= N; x++) {
        auto d = digs(x, 10);
        bool ok = true;
        for (int i = 1; i < (int)d.size(); i++)
          if (d[i] == d[i - 1]) ok = false;
        if (ok) want++;
      }
      check(got == want, "隣接相異 N=" + to_string(N));
    }
    report("隣り合う桁が同じでない");
  }

  {  // nxt の first が効く条件。最初の桁が偶数の数を数える
    for (ll N = 0; N <= 2000; N += 17) {
      // 状態 0 = まだ始まっていない、1 = 最初の桁が偶数だった
      auto r = digit_dp<ll>(to_str(N, 10), 2, 0, [](int s, int d, bool first) {
        return first ? (d % 2 == 0 ? 1 : -1) : s;
      });
      ll want = 0;
      for (ll x = 1; x <= N; x++)
        if ((to_str(x, 10)[0] - '0') % 2 == 0) want++;
      check(r[1] == want, "最初の桁が偶数 N=" + to_string(N));
      check(r[0] == 1, "始まらない 0 が s0 に残る N=" + to_string(N));
    }
    report("nxt の first が効く条件");
  }

  {  // 位置に依存する条件。k と lead の組み合わせ
    for (ll N : {9LL, 10LL, 100LL, 1234LL, 9999LL, 54321LL}) {
      int len = (int)to_str(N, 10).size();
      auto r = digit_dp<ll>(to_str(N, 10), 1, 0, [](int, int d, bool, int k) { return d <= k ? 0 : -1; });
      ll want = 0;
      for (ll x = 0; x <= N; x++) {
        auto d = pad(x, len, 10);
        int f = first_nz(d);
        bool ok = true;
        for (int i = f; i < len; i++)
          if (d[i] > len - 1 - i) ok = false;
        if (ok) want++;
      }
      check(r[0] == want, "位置依存 N=" + to_string(N));
    }
    report("位置に依存する条件");
  }

  {  // 総和。個数と総和を一緒に運ぶ
    struct V {
      ll cnt = 0, sum = 0;
      V() = default;
      explicit V(int o) : cnt(o) {}
      V& operator+=(const V& o) { return cnt += o.cnt, sum += o.sum, *this; }
      V& operator-=(const V& o) { return cnt -= o.cnt, sum -= o.sum, *this; }
      bool operator==(const V& o) const { return cnt == o.cnt && sum == o.sum; }
    };
    ll pw[20];
    pw[0] = 1;
    for (int i = 1; i < 20; i++) pw[i] = pw[i - 1] * 10;

    for (ll N : {0LL, 9LL, 10LL, 100LL, 1234LL, 9999LL, 54321LL}) {
      auto r = digit_dp<V>(to_str(N, 10), 3, 0, [](int s, int d) { return (s + d) % 3; },
                           [&](V& a, const V& b, int, int d, bool, int k) {
                             a.cnt += b.cnt, a.sum += b.sum + b.cnt * d * pw[k];
                           });
      ll wc = 0, ws = 0;
      for (ll x = 0; x <= N; x++) {
        ll t = 0;
        for (int d : digs(x, 10)) t += d;
        if (t % 3 == 0) wc++, ws += x;
      }
      check(r[0].cnt == wc, "総和の個数 N=" + to_string(N));
      check(r[0].sum == ws, "総和 N=" + to_string(N));
    }
    report("総和（add を差し替える）");
  }

  {  // add が遷移元の状態 s を見る。隣り合う桁の差の絶対値の総和
    struct W {
      ll cnt = 0, sum = 0;
      W() = default;
      explicit W(int o) : cnt(o) {}
      W& operator+=(const W& o) { return cnt += o.cnt, sum += o.sum, *this; }
      bool operator==(const W& o) const { return cnt == o.cnt && sum == o.sum; }
    };
    for (ll N : {0LL, 9LL, 10LL, 15LL, 100LL, 999LL, 1234LL, 5000LL}) {
      // 状態 = 直前の桁。まだ始まっていない間は s0 のまま
      auto r = digit_dp<W>(to_str(N, 10), 10, 0, [](int, int d) { return d; },
                           [](W& a, const W& b, int s, int d, bool first, int) {
                             a.cnt += b.cnt, a.sum += b.sum;
                             if (!first) a.sum += b.cnt * (ll)abs(s - d);
                           });
      ll got = 0;
      for (const W& v : r) got += v.sum;
      ll want = 0;
      for (ll x = 0; x <= N; x++) {
        auto dg = digs(x, 10);
        for (int i = 1; i < (int)dg.size(); i++) want += abs(dg[i] - dg[i - 1]);
      }
      check(got == want, "隣接差の総和 N=" + to_string(N));
    }
    report("add が遷移元の状態を見る");
  }

  {  // [l, r] 版
    for (ll R = 0; R <= 400; R += 13)
      for (ll L = 0; L <= R; L += 7) {
        auto r = digit_dp<ll>(to_str(L, 10), to_str(R, 10), 3, 0, [](int s, int d) { return (s + d) % 3; });
        vector<ll> want(3, 0);
        for (ll x = L; x <= R; x++) {
          ll t = 0;
          for (int d : digs(x, 10)) t += d;
          want[t % 3]++;
        }
        for (int s = 0; s < 3; s++)
          check(r[s] == want[s], "[L,R] L=" + to_string(L) + " R=" + to_string(R) + " s=" + to_string(s));
      }
    // 桁上がりをまたぐ境目
    for (ll L : {1LL, 10LL, 100LL, 1000LL, 10000LL}) {
      auto r = digit_dp<ll>(to_str(L, 10), to_str(L, 10), 1, 0, [](int, int) { return 0; });
      check(r[0] == 1, "1 点区間 L=" + to_string(L));
    }
    report("[l, r] 版");
  }

  {  // [l, r] で k を見る遷移。l-1 と r で桁数の基準が揃っているか
    for (ll R : {9LL, 10LL, 100LL, 999LL, 1000LL, 1234LL}) {
      int len = (int)to_str(R, 10).size();
      for (ll L = 0; L <= R; L += (R / 7 + 1)) {
        auto r = digit_dp<ll>(to_str(L, 10), to_str(R, 10), 1, 0,
                              [](int, int d, bool, int k) { return d <= k ? 0 : -1; });
        ll want = 0;
        for (ll x = L; x <= R; x++) {
          auto dg = pad(x, len, 10);
          int f = first_nz(dg);
          bool ok = true;
          for (int i = f; i < len; i++)
            if (dg[i] > len - 1 - i) ok = false;
          if (ok) want++;
        }
        check(r[0] == want, "[L,R] で k を見る L=" + to_string(L) + " R=" + to_string(R));
      }
    }
    report("[l, r] で k を見る遷移");
  }

  {  // 基数を変える
    for (int base : {2, 3, 7, 8}) {
      for (ll N = 0; N <= 300; N += 11) {
        auto r = digit_dp<ll>(to_str(N, base), 2, 0, [](int s, int d) { return s ^ (d & 1); }, {}, base);
        vector<ll> want(2, 0);
        for (ll x = 0; x <= N; x++) {
          int p = 0;
          for (int d : digs(x, base)) p ^= d & 1;
          want[p]++;
        }
        for (int s = 0; s < 2; s++)
          check(r[s] == want[s], "base=" + to_string(base) + " N=" + to_string(N) + " s=" + to_string(s));
      }
    }
    report("基数を変える");
  }

  {  // 遷移が止まる場合と端のケース
    // 数 0 は 1 桁も置かずに終わるので、どの桁も置けなくても s0 に 1 個残る
    auto none = digit_dp<ll>("1000", 1, 0, [](int, int) { return -1; });
    check(none[0] == 1, "どの桁も置けなくても 0 は残る");

    auto zero = digit_dp<ll>("0", 1, 0, [](int, int) { return 0; });
    check(zero[0] == 1, "n=0 は 0 の 1 個");

    auto one = digit_dp<ll>("1", 1, 0, [](int, int) { return 0; });
    check(one[0] == 2, "n=1 は 0 と 1 の 2 個");

    // 大きい n（64bit に収まらない）も文字列なら通る。
    // 全桁 9 の数は 9, 99, ... と 60 個あり、始まらない 0 も s0 に残る
    string big(60, '9');
    auto r = digit_dp<ll>(big, 1, 0, [](int, int d) { return d == 9 ? 0 : -1; });
    check(r[0] == 61, "9 だけの数は 60 個 + 0");

    // [l, r] で l == r == 0
    auto p = digit_dp<ll>("0", "0", 1, 0, [](int, int) { return 0; });
    check(p[0] == 1, "[0, 0]");
    report("端のケース");
  }

  {  // 速度。桁数 10^5、状態 10
    string n(100000, '7');
    auto st = chrono::steady_clock::now();
    auto r = digit_dp<ll>(n, 10, 0, [](int s, int d) { return (s + d) % 10; });
    auto ms = chrono::duration_cast<chrono::milliseconds>(chrono::steady_clock::now() - st).count();
    printf("%-40s : %lld ms (r[0]=%lld)\n", "桁数 1e5 * 状態 10", (ll)ms, r[0]);
    check(ms < 1000, "1 秒以内");
    report("速度");
  }

  printf("\n合計 NG: %d\n", ng_total);
  return ng_total ? 1 : 0;
}
