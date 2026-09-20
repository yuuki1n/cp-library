// union_find / monoid_union_find / rollback_union_find / relational_union_find / dynamic_union_find
// の検証
#include <bits/stdc++.h>
using namespace std;
#include <atcoder/dsu>
using namespace atcoder;

#include "../graph/union_find/dynamic_union_find.hpp"
#include "../graph/union_find/monoid_union_find.hpp"
#include "../graph/union_find/relational_union_find.hpp"
#include "../graph/union_find/rollback_union_find.hpp"
#include "../graph/union_find/union_find.hpp"

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

// ACL 流の型引数に渡す自由関数
long long op_max(long long a, long long b) { return a > b ? a : b; }
long long e_min() { return LLONG_MIN / 4; }
int op_xor(int a, int b) { return a ^ b; }
int e_zero() { return 0; }
int inv_id(int a) { return a; }

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

  {  // ---- monoid_union_find ----
    ng = 0;
    for (int iter = 0; iter < 300; iter++) {
      int n = 1 + (int)(rng() % 30);
      vector<long long> a(n);
      for (auto& x : a) x = (long long)(rng() % 1000) - 500;
      monoid_union_find<long long> sum(a);
      monoid_union_find<long long, op_max, e_min> mx(a);
      Naive nv(n);
      for (int q = 0; q < 60; q++) {
        int u = (int)(rng() % n), v = (int)(rng() % n);
        bool r = sum.merge(u, v);
        check(r == mx.merge(u, v), "2 つの monoid_union_find の merge が一致");
        check(r == nv.unite(u, v), "merge の戻り値");
        int x = (int)(rng() % n);
        long long s = 0, m = e_min();
        for (int i : nv.group(x)) {
          s += a[i];
          m = max(m, a[i]);
        }
        check(sum.prod(x) == s, "prod（総和）");
        check(mx.prod(x) == m, "prod（最大値）");
      }
      // apply: 成分ごとの辺の本数を数え、総和が辺の本数と一致するか
      monoid_union_find<long long> cnt(n);
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
    report("monoid_union_find");
  }

  {  // ---- rollback_union_find ----
    ng = 0;
    for (int iter = 0; iter < 200; iter++) {
      int n = 1 + (int)(rng() % 20);
      rollback_union_find uf(n);
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
    report("rollback_union_find");
  }

  {  // ---- relational_union_find（差の制約）----
    ng = 0;
    for (int iter = 0; iter < 400; iter++) {
      int n = 1 + (int)(rng() % 12);
      relational_union_find<> uf(n);
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
    report("relational_union_find（差）");
  }

  {  // ---- relational_union_find（xor / 二部グラフ判定）----
    ng = 0;
    for (int iter = 0; iter < 300; iter++) {
      int n = 2 + (int)(rng() % 10);
      vector<pair<int, int>> es;
      for (int q = 0; q < 12; q++)
        es.push_back({(int)(rng() % n), (int)(rng() % n)});
      relational_union_find<int, op_xor, e_zero, inv_id> uf(n);
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
    report("relational_union_find（xor）");
  }

  {  // ---- dynamic_union_find ----
    ng = 0;
    for (int iter = 0; iter < 200; iter++) {
      int n = 1 + (int)(rng() % 20);
      vector<long long> key(n);  // 10^12 級のキー
      for (int i = 0; i < n; i++) key[i] = 1000000000000LL + i * 7;
      dynamic_union_find<long long> uf;
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
    dynamic_union_find<pair<int, int>> p;
    check(p.add({0, 0}) == true, "add 初回");
    check(p.add({0, 0}) == false, "add 2 回目");
    p.merge({0, 0}, {0, 1});
    p.merge({5, 5}, {0, 0});
    check(p.vertex_count() == 3 && p.group_count() == 1 && p.size({0, 1}) == 3,
          "pair キー");
    check(p.group({5, 5}).size() == 3, "pair キーの group");
    report("dynamic_union_find");
  }

  {  // ---- clear / ガード ----
    ng = 0;
    // rollback_union_find: 履歴が空のまま undo / 負の t で rollback しても壊れない
    {
      rollback_union_find uf(4);
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
            "rollback_union_find::clear");
    }
    // relational_union_find: clear で制約が消える
    {
      relational_union_find<> uf(4);
      check(uf.merge(0, 1, 5), "制約を入れる");
      check(!uf.merge(0, 1, 6), "矛盾を検出");
      uf.clear();
      check(uf.group_count() == 4 && !uf.same(0, 1),
            "relational_union_find::clear");
      check(uf.merge(0, 1, 6), "clear 後は同じ制約が通る");
      check(uf.diff(0, 1) == 6, "clear 後の diff");
    }
    // monoid_union_find: clear で値ごと入れ直す
    {
      monoid_union_find<long long> uf(vector<long long>{1, 2, 3, 4});
      uf.merge(0, 1);
      check(uf.prod(0) == 3, "merge 後の prod");
      uf.clear(vector<long long>{10, 20, 30, 40});
      check(!uf.same(0, 1) && uf.prod(0) == 10, "monoid_union_find::clear(v)");
      uf.clear();
      check(uf.prod(3) == 0 && uf.groups().size() == 4,
            "monoid_union_find::clear()");
    }
    // dynamic_union_find: clear で頂点ごと消える
    {
      dynamic_union_find<long long> uf;
      uf.merge(100, 200);
      check(uf.vertex_count() == 2 && uf.group_count() == 1, "merge 後");
      uf.clear();
      check(uf.vertex_count() == 0 && uf.group_count() == 0,
            "dynamic_union_find::clear");
      uf.add(100);
      check(uf.vertex_count() == 1 && !uf.same(100, 200),
            "clear 後に足し直せる");
    }
    // デフォルトコンストラクタ
    {
      monoid_union_find<long long> a;
      relational_union_find<> b;
      rollback_union_find c;
      dynamic_union_find<long long> d;
      check(a.groups().empty() && b.group_count() == 0 &&
                c.group_count() == 0 && d.vertex_count() == 0,
            "デフォルトコンストラクタ");
      a = monoid_union_find<long long>(vector<long long>(3, 1));
      check(a.prod(0) == 1, "代入で入れ直せる");
    }
    report("clear / ガード");
  }

  {  // ---- group(): 5 種すべてを素朴な実装と突き合わせる ----
    ng = 0;
    for (int it = 0; it < 300; it++) {
      int n = 1 + (int)(rng() % 12);
      Naive nv(n);
      union_find gd(n);
      monoid_union_find<long long> md(vector<long long>(n, 1));
      relational_union_find<> rd(n);
      rollback_union_find rb(n);
      dynamic_union_find<long long> dd;
      for (int i = 0; i < n; i++) dd.add(i);

      int m = (int)(rng() % (2 * n + 1));
      for (int q = 0; q < m; q++) {
        int a = (int)(rng() % n), b = (int)(rng() % n);
        bool want = nv.unite(a, b);
        check(gd.merge(a, b) == want, "union_find merge の返り値");
        check(md.merge(a, b) == want, "monoid_union_find merge の返り値");
        check(rb.merge(a, b) == want, "rollback_union_find merge の返り値");
        check(dd.merge(a, b) == want, "dynamic_union_find merge の返り値");
        // relational は矛盾しない制約だけ入れる（同じ成分なら同じ差を入れ直す）
        long long f = rd.same(a, b) ? rd.diff(a, b) : (long long)(rng() % 100);
        rd.merge(a, b, f);
      }

      for (int x = 0; x < n; x++) {
        vector<int> want = nv.group(x);

        auto sorted_of = [](vector<int> v) {
          sort(v.begin(), v.end());
          return v;
        };
        check(sorted_of(gd.group(x)) == want, "union_find group");
        check(sorted_of(md.group(x)) == want, "monoid_union_find group");
        check(sorted_of(rd.group(x)) == want, "relational_union_find group");
        check(sorted_of(rb.group(x)) == want, "rollback_union_find group");

        vector<long long> gk = dd.group(x);
        vector<int> gi(gk.begin(), gk.end());
        check(sorted_of(gi) == want, "dynamic_union_find group");

        // 自分自身を必ず含み、大きさは size() と一致する
        check((int)want.size() == gd.size(x), "union_find size");
        check((int)want.size() == md.size(x), "monoid_union_find size");
        check((int)want.size() == rd.size(x), "relational_union_find size");
        check((int)want.size() == rb.size(x), "rollback_union_find size");
        check((int)want.size() == dd.size(x), "dynamic_union_find size");
        check(find(want.begin(), want.end(), x) != want.end(), "自分を含む");
      }

      {  // groups(): 成分の集合として一致するか（成分の並び順は問わない）
        vector<vector<int>> want;
        for (int x = 0; x < n; x++)
          if (nv.root(x) == x) want.push_back(nv.group(x));
        sort(want.begin(), want.end());

        auto norm = [](vector<vector<int>> v) {
          for (auto& g : v) sort(g.begin(), g.end());
          sort(v.begin(), v.end());
          return v;
        };
        check(norm(gd.groups()) == want, "union_find groups");
        check(norm(md.groups()) == want, "monoid_union_find groups");
        check(norm(rd.groups()) == want, "relational_union_find groups");
        check(norm(rb.groups()) == want, "rollback_union_find groups");

        vector<vector<int>> di;
        for (auto& g : dd.groups()) di.emplace_back(g.begin(), g.end());
        check(norm(di) == want, "dynamic_union_find groups");

        // 各成分の中が昇順になっているか（dynamic_union_find は add 順なので除く）
        auto asc = [](const vector<vector<int>>& v) {
          for (auto& g : v)
            if (!is_sorted(g.begin(), g.end())) return false;
          return true;
        };
        check(asc(gd.groups()), "union_find groups は成分内が昇順");
        check(asc(rd.groups()), "relational_union_find groups は成分内が昇順");
        check(asc(rb.groups()), "rollback_union_find groups は成分内が昇順");
      }

      int c = nv.count();
      check(gd.group_count() == c, "union_find group_count");
      check(md.group_count() == c, "monoid_union_find group_count");
      check(rd.group_count() == c, "relational_union_find group_count");
      check(rb.group_count() == c, "rollback_union_find group_count");
      check(dd.group_count() == c, "dynamic_union_find group_count");
    }
    report("group / groups 5 種 x 素朴な実装");
  }

  {  // ---- union_find: groups() と clear、undo 後の環 ----
    ng = 0;
    union_find uf(6);
    uf.merge(0, 1);
    uf.merge(1, 2);
    uf.merge(4, 5);
    check(uf.group_count() == 3, "成分は {0,1,2} {3} {4,5} の 3 つ");

    auto gs = uf.groups();  // atcoder::dsu 由来
    check((int)gs.size() == 3, "groups() の個数");
    long long tot = 0;
    for (auto& g : gs) tot += (long long)g.size();
    check(tot == 6, "groups() の要素数の合計");

    auto g0 = uf.group(0);
    sort(g0.begin(), g0.end());
    check((g0 == vector<int>{0, 1, 2}), "group(0)");
    check(uf.group(3) == vector<int>{3}, "孤立点は自分だけ");

    uf.clear();
    check(uf.group_count() == 6, "clear で成分が 6 個に戻る");
    check(uf.group(0) == vector<int>{0}, "clear で環も戻る");
    check(!uf.same(0, 1), "clear で連結も切れる");

    union_find e;
    check(e.group_count() == 0, "既定構築は空");
    report("union_find groups / clear");
  }

  {  // ---- rollback_union_find: undo したら環も戻るか ----
    ng = 0;
    for (int it = 0; it < 200; it++) {
      int n = 1 + (int)(rng() % 8);
      rollback_union_find uf(n);
      vector<vector<int>> snap;
      int m = (int)(rng() % (n + 3));
      for (int q = 0; q < m; q++) {
        vector<int> cur;
        for (int x = 0; x < n; x++) {
          auto g = uf.group(x);
          sort(g.begin(), g.end());
          cur.insert(cur.end(), g.begin(), g.end());
          cur.push_back(-1);
        }
        snap.push_back(cur);
        uf.merge((int)(rng() % n), (int)(rng() % n));
      }
      while (!snap.empty()) {
        uf.undo();
        vector<int> cur;
        for (int x = 0; x < n; x++) {
          auto g = uf.group(x);
          sort(g.begin(), g.end());
          cur.insert(cur.end(), g.begin(), g.end());
          cur.push_back(-1);
        }
        check(cur == snap.back(), "undo で環が元に戻る");
        snap.pop_back();
      }
    }
    report("rollback_union_find undo と環");
  }

#ifdef _GLIBCXX_DEBUG
  puts("速度計測                       : _GLIBCXX_DEBUG のため省略");
#else
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
    bench("速度 union_find merge x4e5", [&] {
      union_find uf(N);
      for (int i = 0; i < Q; i++) uf.merge(a[i], b[i]);
    });
    bench("速度 monoid_union_find merge x4e5", [&] {
      monoid_union_find<long long> uf(vector<long long>(N, 1));
      for (int i = 0; i < Q; i++) uf.merge(a[i], b[i]);
    });
    bench("速度 rollback_union_find merge x4e5", [&] {
      rollback_union_find uf(N);
      for (int i = 0; i < Q; i++) uf.merge(a[i], b[i]);
    });
    bench("速度 relational_union_find merge x4e5", [&] {
      relational_union_find<> uf(N);
      for (int i = 0; i < Q; i++) uf.merge(a[i], b[i], i);
    });
    bench("速度 dynamic_union_find merge x4e5", [&] {
      dynamic_union_find<long long> uf;
      for (int i = 0; i < Q; i++) uf.merge(a[i], b[i]);
    });
    {  // group() が成分の大きさぶんで済むか（全部つないでから 1 点ずつ引く）
      union_find uf(N);
      for (int i = 1; i < N; i++) uf.merge(i - 1, i);
      bench("速度 union_find group x1（N=2e5）",
            [&] { check((int)uf.group(0).size() == N, "全連結"); });
      union_find sm(N);  // 孤立点のまま 4e5 回取り出す
      bench("速度 union_find group x4e5（孤立点）", [&] {
        long long t = 0;
        for (int i = 0; i < Q; i++) t += (long long)sm.group(i % N).size();
        check(t == Q, "孤立点は 1 個ずつ");
      });
    }
  }

#endif
  printf(ng_total ? "\nNG %d 件\n" : "\nすべて OK\n", ng_total);
  return ng_total ? 1 : 0;
}
