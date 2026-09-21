// union_find / relational_union_find（どちらも巻き戻せる版つき）
// / keyed_union_find
// の検証
#include <bits/stdc++.h>
using namespace std;
#include <atcoder/dsu>
using namespace atcoder;

#include "../graph/union_find/keyed_union_find.hpp"
#include "../graph/union_find/relational_union_find.hpp"
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

// 3 次対称群。非可換なので、合成順を間違えると落ちる
using P = array<int, 3>;
P op_perm(P a, P b) {  // a を通してから b
  P c;
  for (int i = 0; i < 3; i++) c[i] = b[a[i]];
  return c;
}
P e_perm() { return P{0, 1, 2}; }
P inv_perm(P a) {
  P c;
  for (int i = 0; i < 3; i++) c[a[i]] = i;
  return c;
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
        for (int y = 0; y < n; y++) check(uf.same(x, y) == nv.same(x, y), "rollback 後の same");
    }
    report("rollback_union_find");
  }

  {  // ---- union_find に値を載せる ----
    ng = 0;
    {  // 和。初期値つき
      union_find<long long> uf(vector<long long>{1, 2, 3, 4});
      uf.merge(0, 1);
      check(uf.prod(0) == 3 && uf.prod(1) == 3, "merge で総和がまとまる");
      uf.apply(0, 10);
      check(uf.prod(1) == 13, "apply");
      uf.merge(2, 3), uf.merge(0, 3);
      check(uf.prod(2) == 20 && uf.group_count() == 1, "全部つなぐ");
      uf.clear();
      check(uf.prod(0) == 0 && uf.group_count() == 4, "clear で値も戻る");
    }
    {  // 巻き戻し x 逆元の無い演算。コールバックでは打ち消せない形
      rollback_union_find<long long, op_max, e_min> uf(vector<long long>{5, 9, 2, 7});
      int t = uf.snapshot();
      uf.merge(0, 1);
      check(uf.prod(0) == 9, "max がまとまる");
      uf.merge(0, 2);
      check(uf.prod(2) == 9, "さらにまとまる");
      uf.rollback(t);
      check(uf.prod(0) == 5 && uf.prod(1) == 9 && uf.prod(2) == 2, "max が巻き戻る");
      check(uf.group_count() == 4, "成分数も戻る");
    }
    for (int it = 0; it < 300; it++) {  // 巻き戻し x max を素朴な実装と突き合わせる
      int n = 1 + (int)(rng() % 8), Q = 20;
      vector<long long> a(n);
      for (auto& x : a) x = (long long)(rng() % 100);
      rollback_union_find<long long, op_max, e_min> uf(a);
      vector<Naive> snap{Naive(n)};
      for (int q = 0; q < Q; q++) {
        int u = (int)(rng() % n), v = (int)(rng() % n);
        Naive cur = snap.back();
        check(uf.merge(u, v) == cur.unite(u, v), "merge の戻り値");
        snap.push_back(cur);
      }
      for (int k = Q; k >= 0; k--) {
        uf.rollback(k);
        Naive& want = snap[k];
        check(uf.group_count() == want.count(), "rollback 後の成分数");
        for (int x = 0; x < n; x++) {
          long long m = e_min();
          for (int i : want.group(x)) m = max(m, a[i]);
          check(uf.prod(x) == m, "rollback 後の prod");
        }
      }
    }
    {  // 値とコールバックは併用できる
      union_find<long long> uf(vector<long long>(5, 1));
      int hit = 0;
      uf.merge(0, 1, [&](int, int) { hit++; });
      check(hit == 1 && uf.prod(0) == 2, "値とコールバックの併用");
    }
    {  // keyed をかぶせる（キー x 値）
      keyed_union_find<string, union_find<long long>> uf;
      uf.apply("a", 5), uf.apply("b", 7);
      uf.merge("a", "b");
      check(uf.prod("a") == 12 && uf.group_count() == 1, "キー x 値");
    }
    report("union_find（値つき）");
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
        if (nv.same(u, v) && rng() % 2) f = pot[v] - pot[u];  // 正しい制約
        else f = (long long)(rng() % 200) - 100;              // でたらめ
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
      for (int q = 0; q < 12; q++) es.push_back({(int)(rng() % n), (int)(rng() % n)});
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
            for (int y : g[x])
              if (col[y] < 0) {
                col[y] = col[x] ^ 1;
                st.push_back(y);
              } else if (col[y] == col[x]) ok2 = false;
          }
        }
      check(ok == ok2, "二部グラフ判定");
    }
    report("relational_union_find（xor）");
  }

  {  // ---- relational_union_find（非可換な群 / 3 次対称群）----
    ng = 0;
    P all[6] = {{0, 1, 2}, {0, 2, 1}, {1, 0, 2}, {1, 2, 0}, {2, 0, 1}, {2, 1, 0}};
    for (int iter = 0; iter < 400; iter++) {
      int n = 1 + (int)(rng() % 10);
      relational_union_find<P, op_perm, e_perm, inv_perm> uf(n);
      Naive nv(n);
      vector<P> pot(n, e_perm());  // 成分内での相対値（参照実装）
      for (int q = 0; q < 40; q++) {
        int u = (int)(rng() % n), v = (int)(rng() % n);
        P want_f = op_perm(inv_perm(pot[u]), pot[v]);
        P f = nv.same(u, v) && rng() % 2 ? want_f : all[rng() % 6];
        bool want = !nv.same(u, v) || want_f == f;
        check(uf.consistent(u, v, f) == want, "非可換 consistent");
        check(uf.merge(u, v, f) == want, "非可換 merge の戻り値");
        if (want && !nv.same(u, v)) {
          // v 側の基準を u 側へ合わせる。左から掛けても成分内の diff は変わらない
          P shift = op_perm(op_perm(pot[u], f), inv_perm(pot[v]));
          for (int i : nv.group(v)) pot[i] = op_perm(shift, pot[i]);
          nv.unite(u, v);
        }
        for (int x = 0; x < n; x++)
          for (int y = 0; y < n; y++) {
            check(uf.same(x, y) == nv.same(x, y), "非可換 same");
            if (nv.same(x, y)) check(uf.diff(x, y) == op_perm(inv_perm(pot[x]), pot[y]), "非可換 diff");
          }
      }
    }
    report("relational_union_find（非可換 / 3 次対称群）");
  }

  {  // ---- merge / undo のコールバック（成分ごとの和を自分で持つ）----
    ng = 0;
    for (int iter = 0; iter < 300; iter++) {
      int n = 1 + (int)(rng() % 10);
      vector<long long> w(n);
      for (auto& x : w) x = (long long)(rng() % 20) - 10;

      union_find uf0(n);
      vector<long long> s0 = w;  // union_find 側。コールバックで維持する
      rollback_union_find uf(n);
      vector<long long> s1 = w;  // rollback 側
      vector<pair<int, int>> es;   // rollback 側に今入っている辺
      vector<pair<int, int>> es0;  // union_find 側（巻き戻さないので増える一方）
      vector<pair<int, int>> snap;  // {merge の回数, 辺の本数}

      for (int q = 0; q < 30; q++) {
        int k = (int)(rng() % 4);
        if (k <= 1) {
          int a = (int)(rng() % n), b = (int)(rng() % n);
          uf0.merge(a, b, [&](int to, int from) { s0[to] += s0[from]; });
          uf.merge(a, b, [&](int to, int from) { s1[to] += s1[from]; });
          es.push_back({a, b}), es0.push_back({a, b});
        } else if (k == 2) {
          snap.push_back({uf.snapshot(), (int)es.size()});
        } else if (!snap.empty()) {
          auto [t, m] = snap.back();
          snap.pop_back();
          uf.rollback(t, [&](int to, int from) { s1[to] -= s1[from]; });
          es.resize(m);
        }
        // 参照実装: 今の辺集合から成分と和を作り直す
        Naive nv(n);
        for (auto [a, b] : es) nv.unite(a, b);
        vector<long long> want(n, 0);
        for (int i = 0; i < n; i++) want[nv.root(i)] += w[i];
        for (int x = 0; x < n; x++) {
          check(s1[uf.leader(x)] == want[nv.root(x)], "rollback のコールバックで和が合う");
          check(uf.same(x, 0) == nv.same(x, 0), "rollback のコールバックで連結性が合う");
        }
        Naive nv0(n);
        for (auto [a, b] : es0) nv0.unite(a, b);
        vector<long long> want0(n, 0);
        for (int i = 0; i < n; i++) want0[nv0.root(i)] += w[i];
        for (int x = 0; x < n; x++) check(s0[uf0.leader(x)] == want0[nv0.root(x)], "union_find のコールバックで和が合う");
      }
    }
    report("merge / undo のコールバック");
  }

  {  // ---- keyed_union_find x 素朴な実装（キーを遅れて足す） ----
    ng = 0;
    for (int iter = 0; iter < 200; iter++) {
      int n = 1 + (int)(rng() % 20);
      vector<long long> key(n);  // 10^12 級のキー
      for (int i = 0; i < n; i++) key[i] = 1000000000000LL + i * 7;
      keyed_union_find<long long, union_find<>> uf;
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
          check((uf.leader(key[x]) == uf.leader(key[y])) == nv.same(x, y), "leader");
        }
      }
    }
    keyed_union_find<pair<int, int>, union_find<>> p;
    check(p.add({0, 0}) == true, "add 初回");
    check(p.add({0, 0}) == false, "add 2 回目");
    p.merge({0, 0}, {0, 1});
    p.merge({5, 5}, {0, 0});
    check(p.vertex_count() == 3 && p.group_count() == 1 && p.size({0, 1}) == 3, "pair キー");
    check(p.group({5, 5}).size() == 3, "pair キーの group");
    report("keyed_union_find（キーの総当たり）");
  }

  {  // ---- clear / ガード ----
    ng = 0;
    // rollback_union_find: 履歴が空のまま undo / 負の t で rollback しても壊れない
    {
      rollback_union_find uf(4);
      uf.undo();
      uf.rollback(-5);
      check(uf.group_count() == 4 && uf.snapshot() == 0, "空履歴の undo / rollback");
      uf.merge(0, 1);
      uf.merge(2, 3);
      uf.rollback(0);
      uf.undo();  // ここでも落ちない
      check(uf.group_count() == 4 && !uf.same(0, 1), "rollback(0) 後の undo");
      uf.merge(0, 1);
      uf.merge(1, 2);
      uf.clear();
      check(uf.group_count() == 4 && uf.snapshot() == 0 && !uf.same(0, 1) && uf.size(0) == 1 && uf.group(0).size() == 1,
            "rollback_union_find::clear");
    }
    // relational_union_find: clear で制約が消える
    {
      relational_union_find<> uf(4);
      check(uf.merge(0, 1, 5), "制約を入れる");
      check(!uf.merge(0, 1, 6), "矛盾を検出");
      uf.clear();
      check(uf.group_count() == 4 && !uf.same(0, 1), "relational_union_find::clear");
      check(uf.merge(0, 1, 6), "clear 後は同じ制約が通る");
      check(uf.diff(0, 1) == 6, "clear 後の diff");
    }
    // keyed_union_find: clear でキーごと消える
    {
      keyed_union_find<long long, union_find<>> uf;
      uf.merge(100, 200);
      check(uf.vertex_count() == 2 && uf.group_count() == 1, "merge 後");
      uf.clear();
      check(uf.vertex_count() == 0 && uf.group_count() == 0, "keyed_union_find::clear");
      uf.add(100);
      check(uf.vertex_count() == 1 && !uf.same(100, 200), "clear 後に足し直せる");
    }
    // デフォルトコンストラクタ
    {
      relational_union_find<> b;
      rollback_union_find c;
      keyed_union_find<long long, union_find<>> d;
      check(b.group_count() == 0 && c.group_count() == 0 && d.vertex_count() == 0, "デフォルトコンストラクタ");
    }
    report("clear / ガード");
  }

  {  // ---- keyed_union_find ----
    ng = 0;
    {  // union_find に pair のキーをかぶせる
      keyed_union_find<pair<int, int>, union_find<>> uf;
      check(uf.merge({0, 0}, {0, 1}) && !uf.merge({0, 1}, {0, 0}), "merge の戻り値");
      uf.merge({5, 5}, {0, 0});
      check(uf.same({5, 5}, {0, 1}) && !uf.same({5, 5}, {9, 9}), "same");
      check(uf.size({0, 0}) == 3 && uf.vertex_count() == 4, "size / vertex_count");
      check(uf.group_count() == 2 && uf.groups().size() == 2, "group_count / groups");
      check(uf.group({0, 1}).size() == 3, "group");
      check(uf.leader({0, 0}) == uf.leader({5, 5}), "leader はキーで返る");
      uf.clear();
      check(uf.vertex_count() == 0 && uf.group_count() == 0 && uf.groups().empty(), "clear");
      check(uf.add({7, 7}) && !uf.add({7, 7}), "add");
    }
    {  // rollback をかぶせる。巻き戻るのは併合だけ
      keyed_union_find<pair<int, int>, rollback_union_find<>> uf;
      uf.merge({0, 0}, {0, 1});
      int t = uf.snapshot();
      uf.merge({0, 1}, {2, 2});
      check(uf.same({0, 0}, {2, 2}) && uf.size({0, 0}) == 3, "rollback 前");
      uf.rollback(t);
      check(!uf.same({0, 0}, {2, 2}) && uf.size({0, 0}) == 2, "rollback 後");
      check(uf.vertex_count() == 3, "キーの登録は巻き戻らない");

      vector<long long> sum(8, 1);
      keyed_union_find<long long, rollback_union_find<>> vu;
      vu.merge(1LL << 40, 1LL << 41, [&](int to, int from) { sum[to] += sum[from]; });
      check(sum[vu.uf.leader(vu.id(1LL << 40))] == 2, "コールバックは添字で来る");
      vu.undo([&](int to, int from) { sum[to] -= sum[from]; });
      check(sum[vu.id(1LL << 40)] == 1 && sum[vu.id(1LL << 41)] == 1, "undo のコールバック");
    }
    {  // relational をかぶせる
      keyed_union_find<string, relational_union_find<>> uf;
      check(uf.merge("a", "b", 3) && uf.merge("b", "c", 4), "merge に重みを渡せる");
      check(uf.diff("a", "c") == 7, "diff");
      check(!uf.merge("a", "c", 8) && uf.consistent("a", "c", 7), "矛盾の検出");
      int hit = 0;
      uf.merge("x", "y", 1, [&](int, int) { hit++; });
      check(hit == 1 && uf.group_count() == 2, "重み + コールバック");
    }
    {  // Map を差し替える
      keyed_union_find<long long, union_find<>, unordered_map<long long, int>> uf;
      uf.merge(1LL << 40, 1LL << 41);
      check(uf.same(1LL << 40, 1LL << 41) && uf.leader(1LL << 40) == (1LL << 40), "unordered_map");
    }
    for (int it = 0; it < 300; it++) {  // 総当たりとの突き合わせ
      int n = 8;
      keyed_union_find<pair<int, int>, union_find<>> uf;
      Naive nv(n);
      for (int q = 0; q < 20; q++) {
        int a = (int)(rng() % n), b = (int)(rng() % n);
        pair<int, int> ka{a / 3, a % 3}, kb{b / 3, b % 3};
        int ia = uf.id(ka), ib = uf.id(kb);
        check(uf.merge(ka, kb) == nv.unite(ia, ib), "merge の戻り値");
        check(uf.same(ka, kb) && (int)uf.group(ka).size() == uf.size(ka), "same / group / size");
      }
      int comps = 0;
      size_t tot = 0;
      for (int i = 0; i < uf.vertex_count(); i++) comps += (nv.root(i) == i);
      for (auto& g : uf.groups()) tot += g.size();
      check(uf.group_count() == comps && (int)uf.groups().size() == comps, "group_count");
      check((int)tot == uf.vertex_count(), "groups がキーを漏らさない");
    }
    report("keyed_union_find");
  }

  {  // ---- rollback_relational_union_find ----
    ng = 0;
    using RR = rollback_relational_union_find<>;
    for (int it = 0; it < 300; it++) {  // 巻き戻さない限り relational_union_find と一致する
      int n = 1 + (int)(rng() % 12);
      RR rr(n);
      relational_union_find<> rl(n);
      for (int q = 0; q < 40; q++) {
        int u = (int)(rng() % n), v = (int)(rng() % n);
        long long w = (long long)(rng() % 21) - 10;
        check(rr.consistent(u, v, w) == rl.consistent(u, v, w), "consistent が一致");
        check(rr.merge(u, v, w) == rl.merge(u, v, w), "merge の戻り値が一致");
        int a = (int)(rng() % n), b = (int)(rng() % n);
        check(rr.same(a, b) == rl.same(a, b), "same が一致");
        if (rr.same(a, b)) check(rr.diff(a, b) == rl.diff(a, b), "diff が一致");
        check(rr.size(a) == rl.size(a) && rr.group_count() == rl.group_count(), "size / group_count が一致");
        vector<int> ga = rr.group(a), gb = rl.group(a);
        sort(ga.begin(), ga.end()), sort(gb.begin(), gb.end());
        check(ga == gb, "group が一致");
      }
    }
    for (int it = 0; it < 200; it++) {  // どの時点へ戻しても、その時点の状態と一致する
      int n = 1 + (int)(rng() % 10), Q = 30;
      RR rr(n);
      vector<relational_union_find<>> snap{relational_union_find<>(n)};  // 各時刻を丸ごと保存
      for (int q = 0; q < Q; q++) {
        int u = (int)(rng() % n), v = (int)(rng() % n);
        long long w = (long long)(rng() % 11) - 5;
        auto cur = snap.back();
        check(rr.merge(u, v, w) == cur.merge(u, v, w), "merge の戻り値");
        snap.push_back(cur);
      }
      for (int k = Q; k >= 0; k--) {
        rr.rollback(k);
        auto& want = snap[k];
        for (int x = 0; x < n; x++)
          for (int y = 0; y < n; y++) {
            check(rr.same(x, y) == want.same(x, y), "rollback 後の same");
            if (rr.same(x, y)) check(rr.diff(x, y) == want.diff(x, y), "rollback 後の diff");
          }
        check(rr.group_count() == want.group_count(), "rollback 後の group_count");
      }
      check(rr.snapshot() == 0, "全部戻すと履歴が空になる");
    }
    {  // 非可換な群（3 次対称群）でも合成順が合っているか
      for (int it = 0; it < 200; it++) {
        int n = 1 + (int)(rng() % 8);
        rollback_relational_union_find<P, op_perm, e_perm, inv_perm> rr(n);
        relational_union_find<P, op_perm, e_perm, inv_perm> rl(n);
        for (int q = 0; q < 25; q++) {
          int u = (int)(rng() % n), v = (int)(rng() % n);
          P w = e_perm();
          swap(w[rng() % 3], w[rng() % 3]);
          check(rr.merge(u, v, w) == rl.merge(u, v, w), "非可換 merge の戻り値");
          int a = (int)(rng() % n), b = (int)(rng() % n);
          if (rr.same(a, b)) check(rr.diff(a, b) == rl.diff(a, b), "非可換 diff");
        }
      }
    }
    {  // コールバックと pot / clear
      RR uf(6);
      int hit = 0;
      uf.merge(0, 1, 5, [&](int, int) { hit++; });
      uf.merge(0, 1, 5, [&](int, int) { hit++; });  // 併合しないので呼ばれない
      check(hit == 1, "merge コールバックは併合したときだけ");
      uf.undo([&](int, int) { hit--; });
      check(hit == 1, "併合しなかった undo では呼ばれない");
      uf.undo([&](int, int) { hit--; });
      check(hit == 0 && !uf.same(0, 1), "undo コールバック");
      uf.merge(0, 1, 5);
      check(uf.pot(0) == 0 || uf.pot(1) == 0, "根の pot は単位元");
      check(uf.diff(0, 1) == 5, "diff");
      uf.clear();
      check(!uf.same(0, 1) && uf.group_count() == 6 && uf.snapshot() == 0, "clear");
      check(uf.merge(0, 1, 7) && uf.diff(0, 1) == 7, "clear 後に入れ直せる");
    }
    {  // keyed をかぶせる（キー x 巻き戻し x ポテンシャル）
      keyed_union_find<string, RR> uf;
      check(uf.merge("a", "b", 3) && uf.merge("b", "c", 4), "キーで merge");
      check(uf.diff("a", "c") == 7, "キーで diff");
      int t = uf.snapshot();
      uf.merge("c", "d", 5);
      check(uf.same("a", "d") && uf.diff("a", "d") == 12, "rollback 前");
      uf.rollback(t);
      check(!uf.same("a", "d") && uf.diff("a", "c") == 7, "rollback 後");
    }
    report("rollback_relational_union_find");
  }

  {  // ---- group(): 4 種すべてを素朴な実装と突き合わせる ----
    ng = 0;
    for (int it = 0; it < 300; it++) {
      int n = 1 + (int)(rng() % 12);
      Naive nv(n);
      union_find gd(n);
      relational_union_find<> rd(n);
      rollback_union_find rb(n);
      keyed_union_find<long long, union_find<>> dd;
      for (int i = 0; i < n; i++) dd.add(i);

      int m = (int)(rng() % (2 * n + 1));
      for (int q = 0; q < m; q++) {
        int a = (int)(rng() % n), b = (int)(rng() % n);
        bool want = nv.unite(a, b);
        check(gd.merge(a, b) == want, "union_find merge の返り値");
        check(rb.merge(a, b) == want, "rollback_union_find merge の返り値");
        check(dd.merge(a, b) == want, "keyed_union_find merge の返り値");
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
        check(sorted_of(rd.group(x)) == want, "relational_union_find group");
        check(sorted_of(rb.group(x)) == want, "rollback_union_find group");

        vector<long long> gk = dd.group(x);
        vector<int> gi(gk.begin(), gk.end());
        check(sorted_of(gi) == want, "keyed_union_find group");

        // 自分自身を必ず含み、大きさは size() と一致する
        check((int)want.size() == gd.size(x), "union_find size");
        check((int)want.size() == rd.size(x), "relational_union_find size");
        check((int)want.size() == rb.size(x), "rollback_union_find size");
        check((int)want.size() == dd.size(x), "keyed_union_find size");
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
        check(norm(rd.groups()) == want, "relational_union_find groups");
        check(norm(rb.groups()) == want, "rollback_union_find groups");

        vector<vector<int>> di;
        for (auto& g : dd.groups()) di.emplace_back(g.begin(), g.end());
        check(norm(di) == want, "keyed_union_find groups");

        // 各成分の中が昇順になっているか（keyed_union_find は add 順なので除く）
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
      check(rd.group_count() == c, "relational_union_find group_count");
      check(rb.group_count() == c, "rollback_union_find group_count");
      check(dd.group_count() == c, "keyed_union_find group_count");
    }
    report("group / groups 4 種 x 素朴な実装");
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
      printf("%-30s : %lld ms\n", name, (long long)chrono::duration_cast<chrono::milliseconds>(chrono::steady_clock::now() - st).count());
    };
    bench("速度 dsu merge x4e5", [&] {
      dsu uf(N);
      for (int i = 0; i < Q; i++) uf.merge(a[i], b[i]);
    });
    bench("速度 union_find merge x4e5", [&] {
      union_find uf(N);
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
    bench("速度 keyed_union_find merge x4e5", [&] {
      keyed_union_find<long long, union_find<>> uf;
      for (int i = 0; i < Q; i++) uf.merge(a[i], b[i]);
    });
    {  // group() が成分の大きさぶんで済むか（全部つないでから 1 点ずつ引く）
      union_find uf(N);
      for (int i = 1; i < N; i++) uf.merge(i - 1, i);
      bench("速度 union_find group x1（N=2e5）", [&] { check((int)uf.group(0).size() == N, "全連結"); });
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
