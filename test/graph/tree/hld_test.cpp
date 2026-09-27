// hld の検証
#include <bits/stdc++.h>
using namespace std;

#include <atcoder/segtree>
using namespace atcoder;

#include "../../../graph/tree/hld.hpp"

int ng = 0;        // 今のブロックの NG 件数（report のたびに 0 に戻す）
int ng_total = 0;  // 全体の NG 件数
void check(bool ok, const string& msg) {
  if (!ok && ng < 5) printf("  NG: %s\n", msg.c_str());
  if (!ok) ng++, ng_total++;
}
void report(const string& name) {
  printf("%-34s : %s\n", name.c_str(), ng ? "NG" : "OK");
  ng = 0;
}

long long op_add(long long a, long long b) { return a + b; }
long long e_zero() { return 0; }

// 1 次関数 f(x) = a x + b。合成は非可換
struct Aff {
  long long a = 1, b = 0;
};
Aff aff_op(Aff f, Aff g) { return Aff{f.a * g.a, g.a * f.b + g.b}; }  // f を通してから g
Aff aff_rev(Aff f, Aff g) { return aff_op(g, f); }
Aff aff_e() { return Aff{}; }

// 素朴な木。BFS で親と深さを持つだけ
struct Naive {
  int n;
  vector<vector<int>> g;
  vector<int> par, dep;
  Naive(int n_, const vector<pair<int, int>>& es, int root) : n(n_), g(n_), par(n_, -1), dep(n_, 0) {
    for (auto [u, v] : es) g[u].push_back(v), g[v].push_back(u);
    vector<int> st{root};
    vector<char> vis(n, 0);
    vis[root] = 1;
    while (!st.empty()) {
      int v = st.back();
      st.pop_back();
      for (int w : g[v])
        if (!vis[w]) vis[w] = 1, par[w] = v, dep[w] = dep[v] + 1, st.push_back(w);
    }
  }
  vector<int> path(int u, int v) const {  // u から v への頂点列
    vector<int> a, b;
    int x = u, y = v;
    while (x != y) {
      if (dep[x] >= dep[y]) a.push_back(x), x = par[x];
      else b.push_back(y), y = par[y];
    }
    a.push_back(x);
    reverse(b.begin(), b.end());
    for (int z : b) a.push_back(z);
    return a;
  }
  int lca(int u, int v) const {
    int x = u, y = v;
    while (x != y) {
      if (dep[x] >= dep[y]) x = par[x];
      else y = par[y];
    }
    return x;
  }
  vector<int> subtree(int v) const {
    vector<int> ret, st{v};
    while (!st.empty()) {
      int x = st.back();
      st.pop_back();
      ret.push_back(x);
      for (int w : g[x])
        if (w != par[x]) st.push_back(w);
    }
    sort(ret.begin(), ret.end());
    return ret;
  }
};

vector<pair<int, int>> gen_tree(mt19937& rng, int n, int shape) {
  vector<pair<int, int>> es;
  for (int v = 1; v < n; v++) {
    int p;
    if (shape == 0) p = (int)(rng() % (unsigned)v);           // ランダム
    else if (shape == 1) p = v - 1;                            // パス
    else if (shape == 2) p = 0;                                // 星
    else p = (v - 1) / 2;                                      // 完全二分木
    es.push_back({p, v});
  }
  shuffle(es.begin(), es.end(), rng);
  for (auto& [u, v] : es)
    if (rng() & 1) swap(u, v);
  return es;
}

int main() {
  mt19937 rng(20260926);

  {  // ---- 基本（lca / dist / 部分木 / 位置） ----
    ng = 0;
    for (int it = 0; it < 400; it++) {
      int n = 1 + (int)(rng() % 30), shape = (int)(rng() % 4), root = (int)(rng() % n);
      auto es = gen_tree(rng, n, shape);
      hld t(n);
      for (auto [u, v] : es) t.add_edge(u, v);
      t.build(root);
      Naive nv(n, es, root);

      check(t.size() == n, "size");
      // in は 0..n-1 の並べ替え
      vector<int> seen(n, 0);
      for (int v = 0; v < n; v++) {
        check(0 <= t.in(v) && t.in(v) < n, "in の範囲");
        seen[t.in(v)]++;
        check(t.at(t.in(v)) == v, "at は in の逆");
      }
      for (int i = 0; i < n; i++) check(seen[i] == 1, "in は順列");

      for (int v = 0; v < n; v++) {
        check(t.depth(v) == nv.dep[v], "depth");
        check(t.la(v, 1) == nv.par[v], "la(v, 1) は親");
        check(t.size(v) == (int)nv.subtree(v).size(), "size(v)");
        // 部分木が 1 区間に収まっているか
        int l = t.in(v), r = t.out(v);
        vector<int> got;
        for (int i = l; i < r; i++) got.push_back(t.at(i));
        sort(got.begin(), got.end());
        check(got == nv.subtree(v), "部分木は [in, out) の 1 区間");
      }
      for (int u = 0; u < n; u++)
        for (int v = 0; v < n; v++) {
          check(t.lca(u, v) == nv.lca(u, v), "lca");
          check(t.dist(u, v) == (int)nv.path(u, v).size() - 1, "dist");
        }
    }
    report("基本（lca / dist / 部分木）");
  }

  {  // ---- la / jump ----
    ng = 0;
    for (int it = 0; it < 300; it++) {
      int n = 1 + (int)(rng() % 25), shape = (int)(rng() % 4), root = (int)(rng() % n);
      auto es = gen_tree(rng, n, shape);
      hld t(n);
      for (auto [u, v] : es) t.add_edge(u, v);
      t.build(root);
      Naive nv(n, es, root);

      for (int v = 0; v < n; v++) {
        for (int k = 0; k <= n; k++) {
          int want = v;
          for (int i = 0; i < k && want >= 0; i++) want = nv.par[want];
          if (k > nv.dep[v]) want = -1;
          check(t.la(v, k) == want, "la");
        }
        check(t.la(v, -1) == -1, "la に負を渡す");
      }
      for (int u = 0; u < n; u++)
        for (int v = 0; v < n; v++) {
          auto p = nv.path(u, v);
          for (int k = 0; k <= (int)p.size(); k++) {
            int want = k < (int)p.size() ? p[k] : -1;
            check(t.jump(u, v, k) == want, "jump");
          }
          check(t.jump(u, v, -1) == -1, "jump に負を渡す");
        }
    }
    report("la / jump");
  }

  {  // ---- path（頂点） ----
    ng = 0;
    for (int it = 0; it < 300; it++) {
      int n = 1 + (int)(rng() % 25), shape = (int)(rng() % 4), root = (int)(rng() % n);
      auto es = gen_tree(rng, n, shape);
      hld t(n);
      for (auto [u, v] : es) t.add_edge(u, v);
      t.build(root);
      Naive nv(n, es, root);

      for (int u = 0; u < n; u++)
        for (int v = 0; v < n; v++) {
          vector<int> got;
          for (auto [l, r, fwd] : t.path(u, v)) {
            check(l < r, "空の区間を返さない");
            for (int i = l; i < r; i++) got.push_back(t.at(i));
          }
          auto want = nv.path(u, v);
          sort(got.begin(), got.end()), sort(want.begin(), want.end());
          check(got == want, "path（頂点）");
          check((int)t.path(u, v).size() <= 2 * 20, "区間の個数は O(log n)");
        }
    }
    report("path（頂点）");
  }

  {  // ---- path がパス順に並ぶか（fwd を使って u -> v の頂点列を復元する） ----
    ng = 0;
    for (int it = 0; it < 300; it++) {
      int n = 1 + (int)(rng() % 25), shape = (int)(rng() % 4), root = (int)(rng() % n);
      auto es = gen_tree(rng, n, shape);
      hld t(n);
      for (auto [u, v] : es) t.add_edge(u, v);
      t.build(root);
      Naive nv(n, es, root);

      for (int u = 0; u < n; u++)
        for (int v = 0; v < n; v++) {
          vector<int> got;
          for (auto [l, r, fwd] : t.path(u, v)) {
            if (fwd)
              for (int i = l; i < r; i++) got.push_back(t.at(i));
            else
              for (int i = r - 1; i >= l; i--) got.push_back(t.at(i));
          }
          check(got == nv.path(u, v), "path が u -> v の順に並ぶ");
        }
    }
    report("path の並びと向き");
  }

  {  // ---- 非可換な演算（1 次関数の合成） ----
    // fwd が false の区間は逆向きセグ木から引く
    ng = 0;
    for (int it = 0; it < 200; it++) {
      int n = 1 + (int)(rng() % 20), shape = (int)(rng() % 4), root = (int)(rng() % n);
      auto es = gen_tree(rng, n, shape);
      hld t(n);
      for (auto [u, v] : es) t.add_edge(u, v);
      t.build(root);
      Naive nv(n, es, root);

      vector<Aff> f(n);
      for (auto& x : f) x = Aff{(long long)(rng() % 5) + 1, (long long)(rng() % 7)};
      segtree<Aff, aff_op, aff_e> seg(n);
      segtree<Aff, aff_rev, aff_e> rev(n);
      for (int v = 0; v < n; v++) seg.set(t.in(v), f[v]), rev.set(t.in(v), f[v]);

      for (int u = 0; u < n; u++)
        for (int v = 0; v < n; v++) {
          Aff got = aff_e();
          for (auto [l, r, fwd] : t.path(u, v)) got = aff_op(got, fwd ? seg.prod(l, r) : rev.prod(l, r));
          Aff want = aff_e();
          for (int x : nv.path(u, v)) want = aff_op(want, f[x]);
          check(got.a == want.a && got.b == want.b, "非可換な合成");
        }
    }
    report("非可換な演算");
  }

  {  // ---- path（辺） ----
    // 辺の値は子側の頂点に置く。位置は in(深いほうの端点)
    ng = 0;
    for (int it = 0; it < 300; it++) {
      int n = 2 + (int)(rng() % 24), shape = (int)(rng() % 4), root = (int)(rng() % n);
      auto es = gen_tree(rng, n, shape);
      hld t(n);
      for (auto [u, v] : es) t.add_edge(u, v);
      t.build(root);
      Naive nv(n, es, root);

      // 位置 -> 辺番号。辺と「根でない頂点」が 1 対 1 に対応する
      vector<int> pos2e(n, -1);
      for (int i = 0; i < (int)es.size(); i++) {
        auto [u, v] = es[i];
        int p = t.in(t.depth(u) > t.depth(v) ? u : v);
        check(p == t.in(t.la(u, 1) == v ? u : v), "深いほう = 親でないほう");
        check(pos2e[p] == -1, "辺の置き場が重複しない");
        pos2e[p] = i;
      }
      check(pos2e[t.in(root)] == -1, "根の位置には辺が来ない");

      for (int u = 0; u < n; u++)
        for (int v = 0; v < n; v++) {
          vector<int> got;
          for (auto [l, r, fwd] : t.path(u, v, true))
            for (int i = l; i < r; i++) {
              check(pos2e[i] >= 0, "辺の位置に辺がある");
              got.push_back(pos2e[i]);
            }
          // 素朴: パス上の隣り合う頂点の辺
          auto p = nv.path(u, v);
          vector<int> want;
          for (int i = 0; i + 1 < (int)p.size(); i++)
            for (int j = 0; j < (int)es.size(); j++) {
              auto [a, b] = es[j];
              if ((a == p[i] && b == p[i + 1]) || (b == p[i] && a == p[i + 1])) want.push_back(j);
            }
          sort(got.begin(), got.end()), sort(want.begin(), want.end());
          check(got == want, "path（辺）");
        }
    }
    report("path（辺）");
  }

  {  // ---- セグメント木と組み合わせる ----
    ng = 0;
    for (int it = 0; it < 200; it++) {
      int n = 1 + (int)(rng() % 25), shape = (int)(rng() % 4), root = (int)(rng() % n);
      auto es = gen_tree(rng, n, shape);
      hld t(n);
      for (auto [u, v] : es) t.add_edge(u, v);
      t.build(root);
      Naive nv(n, es, root);

      vector<long long> a(n);
      for (auto& x : a) x = (long long)(rng() % 1000);
      segtree<long long, op_add, e_zero> seg(n);
      for (int v = 0; v < n; v++) seg.set(t.in(v), a[v]);

      for (int u = 0; u < n; u++)
        for (int v = 0; v < n; v++) {
          long long got = 0;
          for (auto [l, r, fwd] : t.path(u, v)) got += seg.prod(l, r);
          long long want = 0;
          for (int x : nv.path(u, v)) want += a[x];
          check(got == want, "パスの総和");
        }
      for (int v = 0; v < n; v++) {
        long long got = seg.prod(t.in(v), t.out(v)), want = 0;
        for (int x : nv.subtree(v)) want += a[x];
        check(got == want, "部分木の総和");
      }
    }
    report("segtree と組み合わせる");
  }

  {  // ---- 分解の質（区間の個数が O(log n) に収まるか） ----
    // 正しさだけ見ていると「重い子を選び損ねた」「連をつなぎ損ねた」を見逃す。
    // どちらも答えは合うが区間の個数が増えるので、そこを縛る
    ng = 0;
    for (int shape = 0; shape < 6; shape++) {
      int n = 3000;
      vector<pair<int, int>> es;
      if (shape == 5) {
        // 背骨の各節点に葉を 1 枚、背骨より先に付ける。
        // 「最初の子」を選ぶ実装だと葉が連になり、背骨が毎回切れる
        int spine = n / 2;
        for (int v = 1; v < spine; v++) es.push_back({v - 1, spine + v - 1}), es.push_back({v - 1, v});
      } else {
        for (int v = 1; v < n; v++) {
          int p;
          if (shape == 0) p = (int)(rng() % (unsigned)v);
          else if (shape == 1) p = v - 1;
          else if (shape == 2) p = 0;
          else if (shape == 3) p = (v - 1) / 2;
          else p = v < n / 2 ? v - 1 : (int)(rng() % (unsigned)(n / 2));  // 毛虫
          es.push_back({p, v});
        }
      }
      n = 0;
      for (auto [u, v] : es) n = max({n, u + 1, v + 1});
      hld t(n);
      for (auto [u, v] : es) t.add_edge(u, v);
      t.build();

      int lim = 2;
      while ((1 << lim) < n) lim++;
      lim = 2 * lim + 1;
      int worst = 0;
      for (int it = 0; it < 3000; it++) {
        int u = (int)(rng() % (unsigned)n), v = (int)(rng() % (unsigned)n);
        worst = max(worst, (int)t.path(u, v).size());
      }
      check(worst <= lim, "path の区間は 2 log n + 1 個以下（shape " + to_string(shape) + " で " + to_string(worst) + " 個）");
    }
    report("分解の質");
  }

  {  // ---- 端のケース ----
    ng = 0;
    {  // 頂点 0 個。既定コンストラクタからでも落ちない
      hld t;
      t.build();
      check(t.size() == 0, "頂点 0 個");
      hld t0(0);
      t0.build();
      check(t0.size() == 0, "hld(0)");
    }
    {
      hld t(1);
      t.build();
      check(t.lca(0, 0) == 0 && t.dist(0, 0) == 0, "頂点 1 個");
      check(t.la(0, 0) == 0 && t.la(0, 1) == -1, "頂点 1 個の la");
      check(t.jump(0, 0, 0) == 0 && t.jump(0, 0, 1) == -1, "頂点 1 個の jump");
      check(t.path(0, 0).size() == 1, "頂点 1 個の path");
      check(t.path(0, 0, true).empty(), "頂点 1 個の path（辺）は空");
      check(t.in(0) == 0 && t.out(0) == 1, "頂点 1 個の in / out");
    }
    {
      hld t(2);
      t.add_edge(0, 1);
      t.build(1);
      check(t.la(0, 1) == 1 && t.la(1, 1) == -1, "根を 1 にする");
      check(t.lca(0, 1) == 1 && t.dist(0, 1) == 1, "2 頂点");
      check(t.path(0, 1, true).size() == 1, "辺 1 本");
    }
    {  // 同じ木を根を変えて何度も構築できる
      hld t(6);
      t.add_edge(0, 1), t.add_edge(1, 2), t.add_edge(1, 3), t.add_edge(3, 4), t.add_edge(3, 5);
      for (int r = 0; r < 6; r++) {
        t.build(r);
        check(t.la(r, 1) == -1, "build し直せる");
        check(t.size(r) == 6, "根の部分木は全体");
      }
    }
    {  // clear して別の木を入れ直せる
      hld t(4);
      t.add_edge(0, 1), t.add_edge(1, 2), t.add_edge(2, 3);
      t.build();
      check(t.dist(0, 3) == 3, "1 本道");
      t.clear();
      t.add_edge(0, 1), t.add_edge(0, 2), t.add_edge(0, 3);
      t.build();
      check(t.dist(1, 3) == 2, "clear 後に星へ入れ替わる");
      check(t.size(0) == 4 && t.size(1) == 1, "clear 後の部分木");
    }
    report("端のケース");
  }

#ifdef _GLIBCXX_DEBUG
  puts("速度計測                           : _GLIBCXX_DEBUG のため省略");
#else
  {  // ---- 速度 ----
    const int N = 200000, Q = 200000;
    vector<pair<int, int>> es;
    for (int v = 1; v < N; v++) es.push_back({(int)(rng() % (unsigned)v), v});
    hld t(N);
    for (auto [u, v] : es) t.add_edge(u, v);

    auto bench = [&](const char* name, auto f) {
      auto st = chrono::steady_clock::now();
      f();
      printf("%-34s : %lld ms\n", name,
             (long long)chrono::duration_cast<chrono::milliseconds>(chrono::steady_clock::now() - st).count());
    };
    bench("速度 build（N=2e5）", [&] { t.build(); });
    vector<int> qu(Q), qv(Q);
    for (int i = 0; i < Q; i++) qu[i] = (int)(rng() % (unsigned)N), qv[i] = (int)(rng() % (unsigned)N);
    long long s = 0;
    bench("速度 lca x2e5", [&] {
      for (int i = 0; i < Q; i++) s += t.lca(qu[i], qv[i]);
    });
    bench("速度 path x2e5", [&] {
      for (int i = 0; i < Q; i++) s += (long long)t.path(qu[i], qv[i]).size();
    });
    bench("速度 la x2e5", [&] {
      for (int i = 0; i < Q; i++) s += t.la(qu[i], (int)(rng() % 20));
    });
    check(s != 0, "最適化で消えない");
  }
#endif
  printf(ng_total ? "\nNG %d 件\n" : "\nすべて OK\n", ng_total);
  return ng_total ? 1 : 0;
}
