// monoid_dsu / rollback_dsu / relational_dsu / dynamic_dsu の検証
#include <bits/stdc++.h>
using namespace std;
#include <atcoder/dsu>
using namespace atcoder;

#include "../graph/dsu/dynamic_dsu.hpp"
#include "../graph/dsu/monoid_dsu.hpp"
#include "../graph/dsu/relational_dsu.hpp"
#include "../graph/dsu/rollback_dsu.hpp"

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

// 素朴な参照実装
struct Naive {
  vector<int> p;
  Naive(int n) : p(n) { iota(p.begin(), p.end(), 0); }
  int root(int x) { return p[x] == x ? x : p[x] = root(p[x]); }
  bool same(int a, int b) { return root(a) == root(b); }
  bool unite(int a, int b) {
    a = root(a);
    b = root(b);
    if (a == b) return false;
    p[b] = a;
    return true;
  }
  int size(int x) {
    int c = 0;
    for (int i = 0; i < (int)p.size(); i++) c += same(i, x);
    return c;
  }
  int count() {
    int c = 0;
    for (int i = 0; i < (int)p.size(); i++) c += root(i) == i;
    return c;
  }
  vector<int> group(int x) {
    vector<int> r;
    for (int i = 0; i < (int)p.size(); i++)
      if (same(i, x)) r.push_back(i);
    return r;
  }
};

int main() {
  mt19937 rng(20260905);

  {  // ---- monoid_dsu ----
    ng = 0;
    for (int iter = 0; iter < 300; iter++) {
      int n = 1 + (int)(rng() % 30);
      vector<long long> a(n);
      for (auto& x : a) x = (long long)(rng() % 1000) - 500;
      monoid_dsu<long long> sum(a);
      auto mxop = [](long long x, long long y) { return max(x, y); };
      monoid_dsu<long long, decltype(mxop)> mx(a, mxop);
      Naive nv(n);
      for (int q = 0; q < 60; q++) {
        int u = (int)(rng() % n), v = (int)(rng() % n);
        bool r = sum.merge(u, v);
        check(r == mx.merge(u, v), "2 つの monoid_dsu の merge が一致");
        check(r == nv.unite(u, v), "merge の戻り値");
        int x = (int)(rng() % n);
        long long s = 0, m = LLONG_MIN;
        for (int i : nv.group(x)) {
          s += a[i];
          m = max(m, a[i]);
        }
        check(sum.prod(x) == s, "prod（総和）");
        check(mx.prod(x) == m, "prod（最大値）");
      }
      // apply: 成分ごとの辺の本数を数え、総和が辺の本数と一致するか
      monoid_dsu<long long> cnt(n, 0);
      long long add = 0;
      for (int q = 0; q < 40; q++) {
        int u = (int)(rng() % n), v = (int)(rng() % n);
        cnt.merge(u, v);
        cnt.apply(u, 1);
        add++;
      }
      long long tot = 0;
      for (auto& g : cnt.groups()) tot += cnt.prod(g[0]);
      check(tot == add, "apply の総和");
    }
    report("monoid_dsu");
  }

  {  // ---- rollback_dsu ----
    ng = 0;
    for (int iter = 0; iter < 200; iter++) {
      int n = 1 + (int)(rng() % 20);
      rollback_dsu uf(n);
      vector<Naive> hist{Naive(n)};  // hist[t] = merge を t 回呼んだ時点の状態
      for (int q = 0; q < 80; q++) {
        if (rng() % 3 == 0 && (int)hist.size() > 1) {
          uf.undo();
          hist.pop_back();
        } else {
          int a = (int)(rng() % n), b = (int)(rng() % n);
          Naive nv = hist.back();
          bool r = nv.unite(a, b);
          check(uf.merge(a, b) == r, "merge の戻り値");
          hist.push_back(nv);
        }
        Naive& nv = hist.back();
        check(uf.group_count() == nv.count(), "count");
        check((int)hist.size() - 1 == uf.snapshot(), "snapshot");
        int x = (int)(rng() % n), y = (int)(rng() % n);
        check(uf.same(x, y) == nv.same(x, y), "same");
        check(uf.size(x) == nv.size(x), "size");
        auto g = uf.group(x);
        sort(g.begin(), g.end());
        check(g == nv.group(x), "group");
      }
      int t = uf.snapshot() / 2;
      uf.rollback(t);
      Naive& nv = hist[t];
      check(uf.group_count() == nv.count(), "rollback 後の count");
      for (int x = 0; x < n; x++)
        for (int y = 0; y < n; y++)
          check(uf.same(x, y) == nv.same(x, y), "rollback 後の same");
    }
    report("rollback_dsu");
  }

  {  // ---- relational_dsu（差の制約）----
    ng = 0;
    for (int iter = 0; iter < 400; iter++) {
      int n = 1 + (int)(rng() % 12);
      relational_dsu<> uf(n);
      Naive nv(n);
      vector<long long> pot(n, 0);  // 成分内での相対値（参照実装）
      for (int q = 0; q < 40; q++) {
        int u = (int)(rng() % n), v = (int)(rng() % n);
        long long f;
        if (nv.same(u, v) && rng() % 2)
          f = pot[v] - pot[u];  // 正しい制約
        else
          f = (long long)(rng() % 200) - 100;  // でたらめ
        bool want = !nv.same(u, v) || pot[v] - pot[u] == f;
        check(uf.consistent(u, v, f) == want, "consistent");
        check(uf.merge(u, v, f) == want, "merge の戻り値（矛盾検出）");
        if (want && !nv.same(u, v)) {
          long long shift = (pot[u] + f) - pot[v];
          for (int i : nv.group(v)) pot[i] += shift;
          nv.unite(u, v);
        }
        check(uf.group_count() == nv.count(), "count");
        check(uf.size(u) == nv.size(u), "size");
        for (int x = 0; x < n; x++)
          for (int y = 0; y < n; y++) {
            check(uf.same(x, y) == nv.same(x, y), "same");
            if (nv.same(x, y)) check(uf.diff(x, y) == pot[y] - pot[x], "diff");
          }
      }
    }
    report("relational_dsu（差）");
  }

  {  // ---- relational_dsu（xor / 二部グラフ判定）----
    ng = 0;
    for (int iter = 0; iter < 300; iter++) {
      int n = 2 + (int)(rng() % 10);
      vector<pair<int, int>> es;
      for (int q = 0; q < 12; q++)
        es.push_back({(int)(rng() % n), (int)(rng() % n)});
      relational_dsu<int, bit_xor<int>, identity> uf(n);
      bool ok = true;
      for (auto [u, v] : es)
        if (!uf.merge(u, v, 1)) {
          ok = false;
          break;
        }
      // 参照: 彩色して二部判定
      vector<vector<int>> g(n);
      for (auto [u, v] : es) {
        g[u].push_back(v);
        g[v].push_back(u);
      }
      vector<int> col(n, -1);
      bool ok2 = true;
      for (int s = 0; s < n; s++)
        if (col[s] < 0) {
          col[s] = 0;
          vector<int> st{s};
          while (!st.empty()) {
            int x = st.back();
            st.pop_back();
            for (int y : g[x]) {
              if (col[y] < 0) {
                col[y] = col[x] ^ 1;
                st.push_back(y);
              } else if (col[y] == col[x])
                ok2 = false;
            }
          }
        }
      check(ok == ok2, "二部グラフ判定");
    }
    report("relational_dsu（xor）");
  }

  {  // ---- dynamic_dsu ----
    ng = 0;
    for (int iter = 0; iter < 200; iter++) {
      int n = 1 + (int)(rng() % 20);
      vector<long long> key(n);  // 10^12 級のキー
      for (int i = 0; i < n; i++) key[i] = 1000000000000LL + i * 7;
      dynamic_dsu<long long> uf;
      Naive nv(n);
      set<int> added;
      for (int q = 0; q < 60; q++) {
        int a = (int)(rng() % n), b = (int)(rng() % n);
        check(uf.merge(key[a], key[b]) == nv.unite(a, b), "merge の戻り値");
        added.insert(a);
        added.insert(b);
        check(uf.vertex_count() == (int)added.size(), "vertices");
        int isolated = 0;  // まだ足していない頂点は成分として数えない
        for (int i = 0; i < n; i++)
          if (!added.count(i)) isolated++;
        check(uf.group_count() == nv.count() - isolated, "count");
        int x = *added.begin();
        check(uf.size(key[x]) == nv.size(x), "size");
        auto g = uf.group(key[x]);
        sort(g.begin(), g.end());
        vector<long long> want;
        for (int i : nv.group(x))
          if (added.count(i)) want.push_back(key[i]);
        sort(want.begin(), want.end());
        check(g == want, "group");
        int y = (int)(rng() % n);
        if (added.count(y)) {
          check(uf.same(key[x], key[y]) == nv.same(x, y), "same");
          check((uf.leader(key[x]) == uf.leader(key[y])) == nv.same(x, y),
                "leader");
        }
      }
    }
    dynamic_dsu<pair<int, int>> p;
    check(p.add({0, 0}) == true, "add 初回");
    check(p.add({0, 0}) == false, "add 2 回目");
    p.merge({0, 0}, {0, 1});
    p.merge({5, 5}, {0, 0});
    check(p.vertex_count() == 3 && p.group_count() == 1 && p.size({0, 1}) == 3,
          "pair キー");
    check(p.group({5, 5}).size() == 3, "pair キーの group");
    report("dynamic_dsu");
  }

  {  // ---- clear / ガード ----
    ng = 0;
    // rollback_dsu: 履歴が空のまま undo / 負の t で rollback しても壊れない
    {
      rollback_dsu uf(4);
      uf.undo();
      uf.rollback(-5);
      check(uf.group_count() == 4 && uf.snapshot() == 0,
            "空履歴の undo / rollback");
      uf.merge(0, 1);
      uf.merge(2, 3);
      uf.rollback(0);
      uf.undo();  // ここでも落ちない
      check(uf.group_count() == 4 && !uf.same(0, 1), "rollback(0) 後の undo");
      uf.merge(0, 1);
      uf.merge(1, 2);
      uf.clear();
      check(uf.group_count() == 4 && uf.snapshot() == 0 && !uf.same(0, 1) &&
                uf.size(0) == 1 && uf.group(0).size() == 1,
            "rollback_dsu::clear");
    }
    // relational_dsu: clear で制約が消える
    {
      relational_dsu<> uf(4);
      check(uf.merge(0, 1, 5), "制約を入れる");
      check(!uf.merge(0, 1, 6), "矛盾を検出");
      uf.clear();
      check(uf.group_count() == 4 && !uf.same(0, 1), "relational_dsu::clear");
      check(uf.merge(0, 1, 6), "clear 後は同じ制約が通る");
      check(uf.diff(0, 1) == 6, "clear 後の diff");
    }
    // monoid_dsu: clear で値ごと入れ直す
    {
      monoid_dsu<long long> uf(vector<long long>{1, 2, 3, 4});
      uf.merge(0, 1);
      check(uf.prod(0) == 3, "merge 後の prod");
      uf.clear(vector<long long>{10, 20, 30, 40});
      check(!uf.same(0, 1) && uf.prod(0) == 10, "monoid_dsu::clear(v)");
      uf.clear(4, 7);
      check(uf.prod(3) == 7 && uf.groups().size() == 4,
            "monoid_dsu::clear(n, e)");
    }
    // dynamic_dsu: clear で頂点ごと消える
    {
      dynamic_dsu<long long> uf;
      uf.merge(100, 200);
      check(uf.vertex_count() == 2 && uf.group_count() == 1, "merge 後");
      uf.clear();
      check(uf.vertex_count() == 0 && uf.group_count() == 0,
            "dynamic_dsu::clear");
      uf.add(100);
      check(uf.vertex_count() == 1 && !uf.same(100, 200),
            "clear 後に足し直せる");
    }
    // デフォルトコンストラクタ
    {
      monoid_dsu<long long> a;
      relational_dsu<> b;
      rollback_dsu c;
      dynamic_dsu<long long> d;
      check(a.groups().empty() && b.group_count() == 0 &&
                c.group_count() == 0 && d.vertex_count() == 0,
            "デフォルトコンストラクタ");
      a = monoid_dsu<long long>(3, 1);
      check(a.prod(0) == 1, "代入で入れ直せる");
    }
    report("clear / ガード");
  }

  {  // ---- 速度 ----
    const int N = 200000, Q = 400000;
    vector<int> a(Q), b(Q);
    for (int i = 0; i < Q; i++) {
      a[i] = (int)(rng() % N);
      b[i] = (int)(rng() % N);
    }
    auto bench = [&](const char* name, auto f) {
      auto st = chrono::steady_clock::now();
      f();
      printf("%-30s : %lld ms\n", name,
             (long long)chrono::duration_cast<chrono::milliseconds>(
                 chrono::steady_clock::now() - st)
                 .count());
    };
    bench("速度 dsu merge x4e5", [&] {
      dsu uf(N);
      for (int i = 0; i < Q; i++) uf.merge(a[i], b[i]);
    });
    bench("速度 monoid_dsu merge x4e5", [&] {
      monoid_dsu<long long> uf(N, 1);
      for (int i = 0; i < Q; i++) uf.merge(a[i], b[i]);
    });
    bench("速度 rollback_dsu merge x4e5", [&] {
      rollback_dsu uf(N);
      for (int i = 0; i < Q; i++) uf.merge(a[i], b[i]);
    });
    bench("速度 relational_dsu merge x4e5", [&] {
      relational_dsu<> uf(N);
      for (int i = 0; i < Q; i++) uf.merge(a[i], b[i], i);
    });
    bench("速度 dynamic_dsu merge x4e5", [&] {
      dynamic_dsu<long long> uf;
      for (int i = 0; i < Q; i++) uf.merge(a[i], b[i]);
    });
  }

  printf(ng_total ? "\nNG %d 件\n" : "\nすべて OK\n", ng_total);
  return ng_total ? 1 : 0;
}
