// string/rolling_hash.hpp の検証。素朴な比較と突き合わせる。
//   g++ -std=gnu++20 -O2 -Wall -Wextra -I.. rolling_hash_test.cpp -o rolling_hash_test
#include "../../string/rolling_hash.hpp"

#include <algorithm>
#include <chrono>
#include <cstdio>
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

string rnd_str(mt19937& rng, int n, int k) {
  string s(n, 'a');
  for (auto& c : s) c = char('a' + rng() % (unsigned)k);
  return s;
}

int main() {
  mt19937 rng(20261008);

  {  // 部分列が一致する <=> ハッシュが一致する
    for (int t = 0; t < 200; t++) {
      int n = 1 + (int)(rng() % 30), k = 1 + (int)(rng() % 3);
      string s = rnd_str(rng, n, k);
      rolling_hash<> h(s);
      check(h.size() == n, "size");
      for (int l1 = 0; l1 <= n; l1++)
        for (int r1 = l1; r1 <= n; r1++)
          for (int l2 = 0; l2 <= n; l2++)
            for (int r2 = l2; r2 <= n; r2++) {
              bool want = s.substr(l1, r1 - l1) == s.substr(l2, r2 - l2);
              bool got = r1 - l1 == r2 - l2 && h.get(l1, r1) == h.get(l2, r2);
              check(got == want, "get s=" + s);
              check(h.same(l1, r1, l2, r2) == want, "same s=" + s);
            }
    }
    report("部分列の一致とハッシュの一致");
  }

  {  // 1 要素ぶん
    for (int t = 0; t < 200; t++) {
      int n = 1 + (int)(rng() % 20);
      string s = rnd_str(rng, n, 4);
      rolling_hash<> h(s);
      for (int i = 0; i < n; i++)
        for (int j = 0; j < n; j++) check((h.get(i) == h.get(j)) == (s[i] == s[j]), "get(i) s=" + s);
    }
    report("get(i) は 1 要素ぶん");
  }

  {  // 逆から読んだハッシュ
    for (int t = 0; t < 200; t++) {
      int n = 1 + (int)(rng() % 25);
      string s = rnd_str(rng, n, 2);
      rolling_hash<> h(s);
      string r = s;
      reverse(r.begin(), r.end());
      rolling_hash<> hr(r);
      for (int l = 0; l <= n; l++)
        for (int rr = l; rr <= n; rr++) {
          // s[l, rr) を逆から読んだものは、反転列の [n-rr, n-l) と同じ
          check(h.rev(l, rr) == hr.get(n - rr, n - l), "rev s=" + s);
          string sub = s.substr(l, rr - l), rsub = sub;
          reverse(rsub.begin(), rsub.end());
          check((h.rev(l, rr) == h.get(l, rr)) == (sub == rsub), "rev と get の一致 s=" + s);
          check(h.palindrome(l, rr) == (sub == rsub), "palindrome s=" + s);
        }
    }
    report("逆から読む / 回文判定");
  }

  {  // 最長共通接頭辞
    for (int t = 0; t < 200; t++) {
      int n = 1 + (int)(rng() % 25);
      string s = rnd_str(rng, n, 2);
      rolling_hash<> h(s);
      for (int i = 0; i <= n; i++)
        for (int j = 0; j <= n; j++) {
          int want = 0;
          while (i + want < n && j + want < n && s[i + want] == s[j + want]) want++;
          check(h.lcp(i, j) == want, "lcp s=" + s + " i=" + to_string(i) + " j=" + to_string(j));
        }
      // 別の列との lcp
      string u = rnd_str(rng, 1 + (int)(rng() % 25), 2);
      rolling_hash<> hu(u);
      for (int i = 0; i <= n; i++)
        for (int j = 0; j <= (int)u.size(); j++) {
          int want = 0;
          while (i + want < n && j + want < (int)u.size() && s[i + want] == u[j + want]) want++;
          check(h.lcp(hu, i, j) == want, "lcp 別の列 s=" + s + " u=" + u);
        }
    }
    report("最長共通接頭辞");
  }

  {  // 別インスタンスどうしでも比べられる
    for (int t = 0; t < 200; t++) {
      int n = 1 + (int)(rng() % 20), m = 1 + (int)(rng() % 20);
      string s = rnd_str(rng, n, 2), u = rnd_str(rng, m, 2);
      rolling_hash<> a(s), b(u);
      for (int l1 = 0; l1 <= n; l1++)
        for (int r1 = l1; r1 <= n; r1++)
          for (int l2 = 0; l2 <= m; l2++)
            for (int r2 = l2; r2 <= m; r2++) {
              bool want = s.substr(l1, r1 - l1) == u.substr(l2, r2 - l2);
              bool got = r1 - l1 == r2 - l2 && a.get(l1, r1) == b.get(l2, r2);
              check(got == want, "別インスタンス s=" + s + " u=" + u);
            }
    }
    report("別インスタンスどうしの比較");
  }

  {  // vector<整数> からも作れる。値が大きくても通る
    for (int t = 0; t < 100; t++) {
      int n = 1 + (int)(rng() % 20);
      vector<ll> a(n);
      for (auto& x : a) x = (ll)(rng() % 1000000000u) * 1000000007LL;
      rolling_hash<> h(a);
      for (int l1 = 0; l1 <= n; l1++)
        for (int r1 = l1; r1 <= n; r1++)
          for (int l2 = 0; l2 <= n; l2++)
            for (int r2 = l2; r2 <= n; r2++) {
              bool want = equal(a.begin() + l1, a.begin() + r1, a.begin() + l2, a.begin() + r2);
              bool got = r1 - l1 == r2 - l2 && h.get(l1, r1) == h.get(l2, r2);
              check(got == want, "vector<ll> n=" + to_string(n));
            }
    }
    report("vector<整数> から作る");
  }

  {  // 繋ぐ
    for (int t = 0; t < 200; t++) {
      int n = 1 + (int)(rng() % 20);
      string s = rnd_str(rng, n, 3);
      rolling_hash<> h(s);
      for (int i = 0; i <= n; i++)
        for (int j = i; j <= n; j++)
          for (int k = j; k <= n; k++)
            check(rolling_hash<>::concat(h.get(i, j), h.get(j, k), k - j) == h.get(i, k),
                  "concat s=" + s);
    }
    report("concat");
  }

  {  // 更新つき。更新後に作り直したものと一致するか
    for (int t = 0; t < 100; t++) {
      int n = 1 + (int)(rng() % 20);
      string s = rnd_str(rng, n, 3);
      dynamic_rolling_hash h(s);
      for (int q = 0; q < 10; q++) {
        int i = (int)(rng() % (unsigned)n);
        char c = char('a' + rng() % 3);
        s[i] = c, h.set(i, (unsigned long long)c);
        rolling_hash<> fresh(s);
        for (int l = 0; l <= n; l++)
          for (int r = l; r <= n; r++) {
            check(h.get(l, r) == fresh.get(l, r), "更新後 get s=" + s);
            check(h.rev(l, r) == fresh.rev(l, r), "更新後 rev s=" + s);
          }
      }
    }
    report("更新つき（dynamic_rolling_hash）");
  }

  {  // rev を先に作ってから更新しても追従するか
    for (int t = 0; t < 100; t++) {
      int n = 2 + (int)(rng() % 15);
      string s = rnd_str(rng, n, 2);
      dynamic_rolling_hash h(s);
      h.rev(0, n);  // 先に逆向きの表を作らせる
      for (int q = 0; q < 8; q++) {
        int i = (int)(rng() % (unsigned)n);
        char c = char('a' + rng() % 2);
        s[i] = c, h.set(i, (unsigned long long)c);
        string r = s;
        reverse(r.begin(), r.end());
        rolling_hash<> fresh(r);
        for (int l = 0; l <= n; l++)
          for (int rr = l; rr <= n; rr++) check(h.rev(l, rr) == fresh.get(n - rr, n - l), "rev 先作り s=" + s);
      }
    }
    report("rev を作った後の更新");
  }

  {  // 端のケース
    rolling_hash<> e(string(""));
    check(e.size() == 0, "空の size");
    check(e.get(0, 0) == e.get(0, 0), "空の get");
    check(e.palindrome(0, 0), "空は回文");
    check(e.lcp(0, 0) == 0, "空の lcp");

    rolling_hash<> one(string("a"));
    check(one.size() == 1, "1 要素の size");
    check(one.palindrome(0, 1), "1 要素は回文");
    check(one.get(0, 0) == e.get(0, 0), "空区間は列が違っても同じ");
    check(one.lcp(0, 1) == 0, "lcp(0, 1)");
    check(one.lcp(0, 0) == 1, "lcp(0, 0)");

    // 0 を含む列。0 と「無い」が混ざらないか
    rolling_hash<> z(vector<ll>{0, 0, 0});
    check(z.get(0, 1) == z.get(1, 2), "0 どうしは一致");
    check(z.get(0, 1) != z.get(0, 2), "長さが違えば別");
    check(z.get(0, 0) != z.get(0, 1), "空と 0 一つは別");

    rolling_hash<> p(string("abcba"));
    check(p.palindrome(0, 5), "abcba は回文");
    check(!p.palindrome(0, 4), "abcb は回文でない");
    check(p.palindrome(1, 4), "bcb は回文");
    report("端のケース");
  }

  {  // 速度
    int n = 1000000;
    string s = rnd_str(rng, n, 26);
    auto st = chrono::steady_clock::now();
    rolling_hash<> h(s);
    auto bt = chrono::duration_cast<chrono::milliseconds>(chrono::steady_clock::now() - st).count();
    st = chrono::steady_clock::now();
    unsigned long long acc = 0;
    for (int i = 0; i + 100 <= n; i++) acc ^= h.get(i, i + 100);
    auto qt = chrono::duration_cast<chrono::milliseconds>(chrono::steady_clock::now() - st).count();

    int m = 200000;
    string u = rnd_str(rng, m, 26);
    st = chrono::steady_clock::now();
    dynamic_rolling_hash d(u);
    for (int i = 0; i < m; i++) d.set(i, 'a' + (unsigned)i % 26);
    unsigned long long acc2 = 0;
    for (int i = 0; i + 50 <= m; i++) acc2 ^= d.get(i, i + 50);
    auto dt = chrono::duration_cast<chrono::milliseconds>(chrono::steady_clock::now() - st).count();

    // rev の表は 1 回作ったら使い回す。毎回作り直すとここで落ちる
    st = chrono::steady_clock::now();
    unsigned long long acc3 = 0;
    for (int i = 0; i < 200; i++) acc3 ^= h.rev(i, n - i);
    auto rt = chrono::duration_cast<chrono::milliseconds>(chrono::steady_clock::now() - st).count();

    printf("%-40s : 構築 %lld ms / get 1e6 回 %lld ms / rev 200 回 %lld ms / 更新つき 2e5 %lld ms (%llu %llu %llu)\n",
           "n=1e6", (ll)bt, (ll)qt, (ll)rt, (ll)dt, acc, acc2, acc3);
    check(bt < 500, "構築 0.5 秒以内");
    check(qt < 500, "get 1e6 回が 0.5 秒以内");
    check(rt < 500, "rev 200 回が 0.5 秒以内（表を使い回しているか）");
    check(dt < 2000, "更新つき 2 秒以内");
    report("速度");
  }

  printf("\n合計 NG: %d\n", ng_total);
  return ng_total ? 1 : 0;
}
