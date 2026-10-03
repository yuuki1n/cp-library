// graph/tree/tree_mo.hpp の検証。パスを直に辿る素朴な実装と突き合わせる。
//   g++ -std=gnu++20 -O2 -Wall -Wextra -I.. tree_mo_test.cpp -o tree_mo_test
#include "../../../graph/tree/hld.hpp"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <random>
#include <set>
#include <string>
#include <vector>

#include "../../../util/mo.hpp"

#include "../../../graph/tree/tree_mo.hpp"

using ll = long long;
using namespace std;

int ng = 0;        // 今のブロックの NG 件数（report のたびに 0 に戻す）
int ng_total = 0;  // 全体の NG 件数
void check(bool ok, const string& msg) {
  if (!ok && ng < 5) printf("  NG: %s\n", msg.c_str());
  if (!ok) ng++, ng_total++;
}
void report(const string& name) {
  printf("%-38s : %s\n", name.c_str(), ng ? "NG" : "OK");
  ng = 0;
}

// 親配列から木を作る。shape 0: ランダム / 1: パス / 2: 星 / 3: 二分木
vector<int> gen_tree(mt19937& rng, int n, int shape) {
  vector<int> par(n, -1);
  for (int v = 1; v < n; v++) {
    if (shape == 0) par[v] = (int)(rng() % (unsigned)v);
    else if (shape == 1) par[v] = v - 1;
    else if (shape == 2) par[v] = 0;
    else par[v] = (v - 1) / 2;
  }
  return par;
}

// s から t のパス上の頂点を素朴に集める
vector<int> path_naive(const vector<int>& par, const vector<int>& dep, int s, int t) {
  vector<int> a, b;
  while (dep[s] > dep[t]) a.push_back(s), s = par[s];
  while (dep[t] > dep[s]) b.push_back(t), t = par[t];
  while (s != t) a.push_back(s), b.push_back(t), s = par[s], t = par[t];
  a.push_back(s);
  a.insert(a.end(), b.rbegin(), b.rend());
  return a;
}

int main() {
  mt19937 rng(20261001);

  {  // パスの頂点数が dist + 1 に一致する（LCA の足し引きが合っているか）
    for (int t = 0; t < 300; t++) {
      int n = 1 + (int)(rng() % 40);
      auto par = gen_tree(rng, n, (int)(rng() % 4));
      vector<int> dep(n, 0);
      for (int v = 1; v < n; v++) dep[v] = dep[par[v]] + 1;

      tree_mo tm(n);
      hld h(n);
      for (int v = 1; v < n; v++) tm.add_edge(par[v], v), h.add_edge(par[v], v);
      tm.build(), h.build();
      check(tm.size() == n, "size");

      int q = 1 + (int)(rng() % 20);
      vector<int> qs(q), qt(q);
      for (int i = 0; i < q; i++) {
        qs[i] = (int)(rng() % (unsigned)n), qt[i] = (int)(rng() % (unsigned)n);
        tm.add_query(qs[i], qt[i]);
      }
      ll c = 0;
      auto got = tm.solve([&](int) { c++; }, [&](int) { c--; }, [&] { return c; });
      for (int i = 0; i < q; i++)
        check(got[i] == h.dist(qs[i], qt[i]) + 1, "頂点数 n=" + to_string(n));
    }
    report("パスの頂点数が dist + 1");
  }

  {  // パス上の頂点集合が素朴な実装と一致する
    for (int t = 0; t < 400; t++) {
      int n = 1 + (int)(rng() % 40), shape = (int)(rng() % 4);
      auto par = gen_tree(rng, n, shape);
      vector<int> dep(n, 0);
      for (int v = 1; v < n; v++) dep[v] = dep[par[v]] + 1;

      tree_mo tm(n);
      for (int v = 1; v < n; v++) tm.add_edge(par[v], v);
      tm.build();

      int q = 1 + (int)(rng() % 30);
      vector<int> qs(q), qt(q);
      for (int i = 0; i < q; i++) {
        qs[i] = (int)(rng() % (unsigned)n), qt[i] = (int)(rng() % (unsigned)n);
        tm.add_query(qs[i], qt[i]);
      }

      // 今パスに入っている頂点の集合をそのまま持って比べる
      set<int> cur;
      auto add = [&](int v) { cur.insert(v); };
      auto rem = [&](int v) { cur.erase(v); };
      auto got = tm.solve(add, rem, [&] { return cur; });

      for (int i = 0; i < q; i++) {
        auto p = path_naive(par, dep, qs[i], qt[i]);
        set<int> want(p.begin(), p.end());
        check(got[i] == want,
              "パス集合 n=" + to_string(n) + " shape=" + to_string(shape) + " (" + to_string(qs[i]) + "," + to_string(qt[i]) + ")");
      }
    }
    report("パス上の頂点集合が一致");
  }

  {  // 重み付き。パス上の重みの和を比べる（add / rem が対称に呼ばれるか）
    for (int t = 0; t < 300; t++) {
      int n = 1 + (int)(rng() % 60);
      auto par = gen_tree(rng, n, (int)(rng() % 4));
      vector<int> dep(n, 0);
      for (int v = 1; v < n; v++) dep[v] = dep[par[v]] + 1;
      vector<ll> w(n);
      for (auto& x : w) x = (ll)(rng() % 1000) - 500;

      tree_mo tm(n);
      for (int v = 1; v < n; v++) tm.add_edge(par[v], v);
      tm.build(n > 1 ? (int)(rng() % (unsigned)n) : 0);  // 根を変えても答えは変わらない

      int q = 1 + (int)(rng() % 30);
      vector<int> qs(q), qt(q);
      for (int i = 0; i < q; i++) {
        qs[i] = (int)(rng() % (unsigned)n), qt[i] = (int)(rng() % (unsigned)n);
        tm.add_query(qs[i], qt[i]);
      }

      ll sum = 0;
      auto got = tm.solve([&](int v) { sum += w[v]; }, [&](int v) { sum -= w[v]; }, [&] { return sum; });
      for (int i = 0; i < q; i++) {
        auto p = path_naive(par, dep, qs[i], qt[i]);
        ll want = 0;
        for (int v : p) want += w[v];
        check(got[i] == want, "重み和 n=" + to_string(n));
      }
    }
    report("重み和が一致（根を変えても同じ）");
  }

  {  // 値の種類数。実際の使い道に近い形
    for (int t = 0; t < 200; t++) {
      int n = 1 + (int)(rng() % 60);
      auto par = gen_tree(rng, n, (int)(rng() % 4));
      vector<int> dep(n, 0);
      for (int v = 1; v < n; v++) dep[v] = dep[par[v]] + 1;
      vector<int> x(n);
      for (auto& v : x) v = (int)(rng() % 8);

      tree_mo tm(n);
      for (int v = 1; v < n; v++) tm.add_edge(par[v], v);
      tm.build();
      int q = 1 + (int)(rng() % 20);
      vector<int> qs(q), qt(q);
      for (int i = 0; i < q; i++) {
        qs[i] = (int)(rng() % (unsigned)n), qt[i] = (int)(rng() % (unsigned)n);
        tm.add_query(qs[i], qt[i]);
      }

      vector<int> cnt(8, 0);
      ll kind = 0;
      auto add = [&](int v) { if (cnt[x[v]]++ == 0) kind++; };
      auto rem = [&](int v) { if (--cnt[x[v]] == 0) kind--; };
      auto got = tm.solve(add, rem, [&] { return kind; });

      for (int i = 0; i < q; i++) {
        auto p = path_naive(par, dep, qs[i], qt[i]);
        set<int> s;
        for (int v : p) s.insert(x[v]);
        check(got[i] == (ll)s.size(), "種類数 n=" + to_string(n));
      }
    }
    report("値の種類数が一致");
  }

  {  // get(i) でクエリ番号を受け取れる
    for (int t = 0; t < 200; t++) {
      int n = 1 + (int)(rng() % 40);
      auto par = gen_tree(rng, n, (int)(rng() % 4));
      vector<int> dep(n, 0);
      for (int v = 1; v < n; v++) dep[v] = dep[par[v]] + 1;
      vector<ll> w(n);
      for (auto& x : w) x = (ll)(rng() % 100);

      tree_mo tm(n);
      for (int v = 1; v < n; v++) tm.add_edge(par[v], v);
      tm.build();
      int q = 1 + (int)(rng() % 20);
      vector<int> qs(q), qt(q);
      for (int i = 0; i < q; i++) {
        qs[i] = (int)(rng() % (unsigned)n), qt[i] = (int)(rng() % (unsigned)n);
        tm.add_query(qs[i], qt[i]);
      }

      // クエリごとに違う係数を掛ける。i が並べ替え前の番号でないと合わない
      ll sum = 0;
      vector<int> seen(q, 0);
      auto got = tm.solve([&](int v) { sum += w[v]; }, [&](int v) { sum -= w[v]; },
                          [&](int i) { seen[i]++; return sum * (i + 1); });
      for (int i = 0; i < q; i++) {
        auto p = path_naive(par, dep, qs[i], qt[i]);
        ll want = 0;
        for (int v : p) want += w[v];
        check(got[i] == want * (i + 1), "get(i) の i n=" + to_string(n) + " i=" + to_string(i));
        check(seen[i] == 1, "各クエリちょうど 1 回 i=" + to_string(i));
      }
    }
    report("get(i) でクエリ番号を受け取れる");
  }

  {  // 端点が同じ / 隣り合う / 根を含む といった端のケース
    int n = 7;
    vector<int> par = {-1, 0, 0, 1, 1, 2, 2};
    vector<int> dep(n, 0);
    for (int v = 1; v < n; v++) dep[v] = dep[par[v]] + 1;
    tree_mo tm(n);
    for (int v = 1; v < n; v++) tm.add_edge(par[v], v);
    tm.build();
    vector<pair<int, int>> qq = {{0, 0}, {3, 3}, {0, 1}, {1, 0}, {3, 4}, {3, 6}, {6, 3}, {0, 6}, {5, 4}};
    for (auto [s, t] : qq) tm.add_query(s, t);
    set<int> cur;
    auto got = tm.solve([&](int v) { cur.insert(v); }, [&](int v) { cur.erase(v); }, [&] { return cur; });
    for (int i = 0; i < (int)qq.size(); i++) {
      auto p = path_naive(par, dep, qq[i].first, qq[i].second);
      set<int> want(p.begin(), p.end());
      check(got[i] == want, "端のケース (" + to_string(qq[i].first) + "," + to_string(qq[i].second) + ")");
    }
    report("端のケース");
  }

  {  // 頂点 1 個 / クエリ 0 件
    tree_mo one(1);
    one.build();
    one.add_query(0, 0);
    ll c = 0;
    auto got = one.solve([&](int) { c++; }, [&](int) { c--; }, [&] { return c; });
    check(got.size() == 1 && got[0] == 1, "頂点 1 個");

    tree_mo tm(5);
    for (int v = 1; v < 5; v++) tm.add_edge(v - 1, v);
    tm.build();
    auto e = tm.solve([&](int) {}, [&](int) {}, [&] { return 0LL; });
    check(e.empty(), "クエリ 0 件");
    report("頂点 1 個 / クエリ 0 件");
  }

  {  // 速度。N = Q = 2e5
    int n = 200000, q = 200000;
    auto par = gen_tree(rng, n, 0);
    vector<int> x(n);
    for (auto& v : x) v = (int)(rng() % 200000);
    tree_mo tm(n);
    for (int v = 1; v < n; v++) tm.add_edge(par[v], v);
    tm.build();
    for (int i = 0; i < q; i++) tm.add_query((int)(rng() % (unsigned)n), (int)(rng() % (unsigned)n));

    auto st = chrono::steady_clock::now();
    vector<int> cnt(200000, 0);
    ll kind = 0, calls = 0;
    auto add = [&](int v) { calls++; if (cnt[x[v]]++ == 0) kind++; };
    auto rem = [&](int v) { calls++; if (--cnt[x[v]] == 0) kind--; };
    auto got = tm.solve(add, rem, [&] { return kind; });
    auto ms = chrono::duration_cast<chrono::milliseconds>(chrono::steady_clock::now() - st).count();
    ll s = 0;
    for (ll v : got) s += v;
    printf("%-38s : %lld ms  add/rem %lld 回 (s=%lld)\n", "N=Q=2e5 ランダム木", (ll)ms, calls, s);
    // 窓の動きは O((2n + q) sqrt(q)) が目安。係数つきで上から押さえる。
    // 区間を s -> t の向きに揃えないと 1.5 倍ほどに増えるので、それも捕まる
    check(calls < (ll)(1.0 * (2.0 * n + q) * 320), "窓の動きが見積もりの範囲（" + to_string(calls) + "）");
    check(ms < 4000, "4 秒以内");
    report("速度");
  }

  printf("\n合計 NG: %d\n", ng_total);
  return ng_total ? 1 : 0;
}
