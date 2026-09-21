// data_structure/avl_segtree.hpp の検証。std::vector を素朴に操作したものと
// 突き合わせる。
//   g++ -std=gnu++20 -O2 -Wall -Wextra -I.. avl_segtree_test.cpp -o avl_segtree_test
#include "../../data_structure/avl_segtree.hpp"

#include <algorithm>
#include <chrono>
#include <climits>
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
  printf("%-34s : %s\n", name.c_str(), ng ? "NG" : "OK");
  ng = 0;
}

/* ---- 和 ---- */
struct S : avl_value {
  ll sum = 0;
};
S op(S a, S b) {
  S r;
  r.sum = a.sum + b.sum;
  return r;
}
S e() { return S{}; }
using tree = avl_segtree<S, op, e>;  // 作用なし（F 以降は既定）

// 区間加算・区間和
using F = ll;
S mapping(F f, S x) {
  x.sum += f * x.sz;
  return x;
}
F composition(F f, F g) { return f + g; }
F id() { return 0; }
using ltree = avl_segtree<S, op, e, F, mapping, composition, id>;

// 区間 affine（x -> a x + b）。非可換な作用で合成順を確かめる
struct A {
  ll a = 1, b = 0;
};
S map_affine(A f, S x) {
  x.sum = f.a * x.sum + f.b * x.sz;
  return x;
}
A comp_affine(A f, A g) { return A{f.a * g.a, f.a * g.b + f.b}; }
A id_affine() { return A{}; }
using atree = avl_segtree<S, op, e, A, map_affine, comp_affine, id_affine>;

S mk(ll v) {
  S s;
  s.sum = v;
  return s;
}

/* ---- 非可換（合成順の検査用）: 文字列の連結 ---- */
struct T : avl_value {
  string s;
};
T op_cat(T a, T b) {
  T r;
  r.s = a.s + b.s;
  return r;
}
T e_cat() { return T{}; }
// 向きで変わる集約なので rev で文字列自体を逆にする
T rev_cat(T x) {
  reverse(x.s.begin(), x.s.end());
  return x;
}
using ctree = avl_segtree<T, op_cat, e_cat, avl_segtree_internal::no_lazy, avl_segtree_internal::map_<T>, avl_segtree_internal::comp_,
                          avl_segtree_internal::id_, rev_cat>;

/* ---- Segment Tree Beats: 区間 chmin / 区間和 / 区間最大 ---- */
struct B : avl_value {
  ll sum = 0;
  ll mx = LLONG_MIN / 4;   // 最大値
  ll mx2 = LLONG_MIN / 4;  // 2 番目に大きい値（最大値と異なるもの）
  int cnt = 0;             // 最大値の個数
};
B op_b(B a, B b) {
  B r;
  r.sum = a.sum + b.sum;
  r.mx = max(a.mx, b.mx);
  r.cnt = (a.mx == r.mx ? a.cnt : 0) + (b.mx == r.mx ? b.cnt : 0);
  r.mx2 = max(a.mx == r.mx ? a.mx2 : a.mx, b.mx == r.mx ? b.mx2 : b.mx);
  return r;
}
B e_b() { return B{}; }
// f より大きい要素を f にする
B map_b(ll f, B x) {
  if (x.mx <= f) return x;  // 何も変わらない
  if (x.mx2 < f) {          // 最大値だけが下がる
    x.sum -= (x.mx - f) * x.cnt;
    x.mx = f;
    return x;
  }
  x.fail = true;  // まとめて適用できない
  return x;
}
ll comp_b(ll f, ll g) { return min(f, g); }
ll id_b() { return LLONG_MAX / 4; }
using btree = avl_segtree<B, op_b, e_b, ll, map_b, comp_b, id_b>;

B mkb(ll v) {
  B x;
  x.sum = v;
  x.mx = v;
  x.cnt = 1;
  return x;
}

vector<ll> raw2(btree& t) {
  vector<ll> r;
  for (auto& x : t.to_vec()) r.push_back(x.sum);
  return r;
}

vector<ll> raw2(ltree& t) {
  vector<ll> r;
  for (auto& x : t.to_vec()) r.push_back(x.sum);
  return r;
}

// to_vec は遅延を配りながら降りるので非 const
vector<ll> raw(tree& t) {
  vector<ll> r;
  for (auto& x : t.to_vec()) r.push_back(x.sum);
  return r;
}

int main() {
  mt19937 rng(20260920);

  {  // ---- 構築と素朴な比較 ----
    ng = 0;
    for (int it = 0; it < 300; it++) {
      int n = (int)(rng() % 20);
      vector<ll> a(n);
      vector<S> v(n);
      for (int i = 0; i < n; i++) {
        a[i] = (ll)(rng() % 100) - 50;
        v[i] = mk(a[i]);
      }
      tree t(v);
      check(t.size() == n, "size");
      check(raw(t) == a, "to_vec が元の列と一致");
      ll all = 0;
      for (ll x : a) all += x;
      check(t.all_prod().sum == all, "all_prod");
      for (int l = 0; l <= n; l++)
        for (int r = l; r <= n; r++) {
          ll s = 0;
          for (int i = l; i < r; i++) s += a[i];
          check(t.prod(l, r).sum == s, "prod(" + to_string(l) + "," + to_string(r) + ")");
        }
      for (int i = 0; i < n; i++) check(t.get(i).sum == a[i], "get");
    }
    report("構築 / prod / get");
  }

  {  // ---- insert / erase / set をランダムに撃つ ----
    ng = 0;
    for (int it = 0; it < 200; it++) {
      vector<ll> a;
      tree t;
      for (int q = 0; q < 120; q++) {
        int n = (int)a.size();
        int kind = (int)(rng() % 100);
        if (kind < 40 || n == 0) {  // insert
          int i = (int)(rng() % (n + 1));
          ll x = (ll)(rng() % 100) - 50;
          a.insert(a.begin() + i, x);
          t.insert(i, mk(x));
        } else if (kind < 65) {  // erase 1 点
          int i = (int)(rng() % n);
          a.erase(a.begin() + i);
          t.erase(i);
        } else if (kind < 80) {  // erase 区間
          int l = (int)(rng() % (n + 1)), r = (int)(rng() % (n + 1));
          if (l > r) swap(l, r);
          a.erase(a.begin() + l, a.begin() + r);
          t.erase(l, r);
        } else if (kind < 92) {  // set
          int i = (int)(rng() % n);
          ll x = (ll)(rng() % 100) - 50;
          a[i] = x;
          t.set(i, mk(x));
        } else {  // prod
          int l = (int)(rng() % (n + 1)), r = (int)(rng() % (n + 1));
          if (l > r) swap(l, r);
          ll s = 0;
          for (int i = l; i < r; i++) s += a[i];
          check(t.prod(l, r).sum == s, "prod");
        }
        check(t.size() == (int)a.size(), "size が一致");
        check(raw(t) == a, "列が一致");
      }
    }
    report("insert / erase / set ランダム");
  }

  {  // ---- 非可換な op で合成順が保たれるか ----
    ng = 0;
    for (int it = 0; it < 200; it++) {
      int n = 1 + (int)(rng() % 12);
      string a;
      vector<T> v(n);
      for (int i = 0; i < n; i++) {
        char c = char('a' + rng() % 4);
        a += c;
        v[i].s = string(1, c);
      }
      avl_segtree<T, op_cat, e_cat> t(v);
      for (int l = 0; l <= n; l++)
        for (int r = l; r <= n; r++) check(t.prod(l, r).s == a.substr(l, r - l), "連結の順序");
      // 挿入しても順序が崩れないか
      for (int q = 0; q < 20; q++) {
        int i = (int)(rng() % (t.size() + 1));
        char c = char('A' + rng() % 4);
        T x;
        x.s = string(1, c);
        a.insert(a.begin() + i, c);
        t.insert(i, x);
        check(t.all_prod().s == a, "挿入後の全体");
      }
    }
    report("非可換な op（文字列連結）");
  }

  {  // ---- sz がライブラリから入るか ----
    ng = 0;
    vector<S> v(7);
    for (int i = 0; i < 7; i++) v[i] = mk(i);
    tree t(v);
    check(t.all_prod().sz == 7, "all_prod の sz");
    check(t.prod(2, 5).sz == 3, "prod の sz");
    check(t.get(3).sz == 1, "get の sz");
    check(t.prod(4, 4).sz == 0, "空区間の sz は 0");
    report("sz がライブラリから入る");
  }

  {  // ---- 境界 ----
    ng = 0;
    tree t;
    check(t.size() == 0 && t.all_prod().sum == 0, "空の木");
    check(t.prod(0, 0).sum == 0, "空の prod");
    check(t.to_vec().empty(), "空の to_vec");

    t.insert(0, mk(5));
    check(t.size() == 1 && t.get(0).sum == 5, "1 要素");
    t.erase(0);
    check(t.size() == 0, "消すと空に戻る");

    tree u(vector<S>{mk(1), mk(2), mk(3)});
    check(u.prod(2, 1).sum == 0, "l > r は e()");
    u.erase(1, 1);
    check(u.size() == 3, "空区間の erase は何もしない");
    u.erase(2, 1);
    check(u.size() == 3, "l > r の erase も何もしない");
    {  // set / get は木の形を変えない（prod が壊れない）
      tree w(vector<S>{mk(1), mk(2), mk(3), mk(4)});
      w.set(2, mk(30));
      check(w.get(2).sum == 30 && w.prod(0, 4).sum == 37, "set 後の get / prod");
      check(w.to_vec().size() == 4, "set で要素数は変わらない");
      check(w.get(0).sz == 1, "get の sz");
    }
    u.erase(0, 3);
    check(u.size() == 0, "全部消す");

    tree z(5);
    check(z.size() == 5 && z.all_prod().sum == 0, "n 個の e()");
    z.clear();
    check(z.size() == 0, "clear");
    report("境界");
  }

  {  // ---- 平衡が保たれているか（末尾に足し続けても深くならない）----
    ng = 0;
    const int N = 20000;
    tree t;
    for (int i = 0; i < N; i++) t.insert(t.size(), mk(1));
    check(t.size() == N, "末尾に 2e4 回 insert");
    check(t.all_prod().sum == N, "総和");
    // 先頭に足し続ける場合も
    tree u;
    for (int i = 0; i < N; i++) u.insert(0, mk(1));
    check(u.all_prod().sum == N, "先頭に 2e4 回 insert");
    report("片側に偏らせても平衡");
  }

  {  // ---- apply: 区間加算を素朴な配列と突き合わせる ----
    ng = 0;
    for (int it = 0; it < 200; it++) {
      int n = 1 + (int)(rng() % 20);
      vector<ll> a(n);
      vector<S> v(n);
      for (int i = 0; i < n; i++) {
        a[i] = (ll)(rng() % 100) - 50;
        v[i] = mk(a[i]);
      }
      ltree t(v);
      for (int q = 0; q < 60; q++) {
        int l = (int)(rng() % (n + 1)), r = (int)(rng() % (n + 1));
        if (l > r) swap(l, r);
        ll f = (ll)(rng() % 21) - 10;
        for (int i = l; i < r; i++) a[i] += f;
        t.apply(l, r, f);

        vector<ll> got;
        for (auto& x : t.to_vec()) got.push_back(x.sum);
        check(got == a, "apply 後の列");

        int ql = (int)(rng() % (n + 1)), qr = (int)(rng() % (n + 1));
        if (ql > qr) swap(ql, qr);
        ll want = 0;
        for (int i = ql; i < qr; i++) want += a[i];
        check(t.prod(ql, qr).sum == want, "apply 後の prod");
        int i = (int)(rng() % n);
        check(t.get(i).sum == a[i], "apply 後の get");
      }
    }
    report("apply（区間加算）");
  }

  {  // ---- 非可換な作用（affine）で合成順が正しいか ----
    ng = 0;
    for (int it = 0; it < 200; it++) {
      int n = 1 + (int)(rng() % 15);
      vector<ll> a(n);
      vector<S> v(n);
      for (int i = 0; i < n; i++) {
        a[i] = (ll)(rng() % 10);
        v[i] = mk(a[i]);
      }
      atree t(v);
      for (int q = 0; q < 40; q++) {
        int l = (int)(rng() % (n + 1)), r = (int)(rng() % (n + 1));
        if (l > r) swap(l, r);
        A f{(ll)(rng() % 5) + 1, (ll)(rng() % 7)};
        for (int i = l; i < r; i++) a[i] = f.a * a[i] + f.b;
        t.apply(l, r, f);

        vector<ll> got;
        for (auto& x : t.to_vec()) got.push_back(x.sum);
        check(got == a, "affine 後の列");
        int ql = (int)(rng() % (n + 1)), qr = (int)(rng() % (n + 1));
        if (ql > qr) swap(ql, qr);
        ll want = 0;
        for (int i = ql; i < qr; i++) want += a[i];
        check(t.prod(ql, qr).sum == want, "affine 後の prod");
      }
    }
    report("apply（非可換な affine）");
  }

  {  // ---- 作用と構造変化を混ぜる（push 漏れを炙り出す）----
    ng = 0;
    for (int it = 0; it < 200; it++) {
      vector<ll> a;
      ltree t;
      for (int q = 0; q < 100; q++) {
        int n = (int)a.size();
        int kind = (int)(rng() % 100);
        if (kind < 30 || n == 0) {
          int i = (int)(rng() % (n + 1));
          ll x = (ll)(rng() % 20) - 10;
          a.insert(a.begin() + i, x);
          t.insert(i, mk(x));
        } else if (kind < 50) {
          int l = (int)(rng() % (n + 1)), r = (int)(rng() % (n + 1));
          if (l > r) swap(l, r);
          a.erase(a.begin() + l, a.begin() + r);
          t.erase(l, r);
        } else if (kind < 65) {
          int i = (int)(rng() % n);
          ll x = (ll)(rng() % 20) - 10;
          a[i] = x;
          t.set(i, mk(x));
        } else {
          int l = (int)(rng() % (n + 1)), r = (int)(rng() % (n + 1));
          if (l > r) swap(l, r);
          ll f = (ll)(rng() % 11) - 5;
          for (int i = l; i < r; i++) a[i] += f;
          t.apply(l, r, f);
        }
        vector<ll> got;
        for (auto& x : t.to_vec()) got.push_back(x.sum);
        check(got == a, "insert/erase/set/apply 混在");
        ll all = 0;
        for (ll x : a) all += x;
        check(t.all_prod().sum == all, "all_prod");
      }
    }
    report("作用と構造変化の混在");
  }

  {  // ---- apply の境界 ----
    ng = 0;
    ltree t(vector<S>{mk(1), mk(2), mk(3)});
    t.apply(1, 1, 100);
    check(t.all_prod().sum == 6, "空区間の apply は何もしない");
    t.apply(2, 1, 100);
    check(t.all_prod().sum == 6, "l > r の apply も何もしない");
    t.apply(1, 5);
    check(t.get(1).sum == 7, "1 点 apply");
    t.apply(0, 3, 10);
    check(t.all_prod().sum == 41, "全体 apply");
    // 作用を省いた型でも apply は呼べる（何もしない）
    tree u(vector<S>{mk(1), mk(2)});
    u.apply(0, 2, avl_segtree_internal::no_lazy{});
    check(u.all_prod().sum == 3, "作用なしの型では apply は無視される");
    report("apply の境界");
  }

  {  // ---- reverse: 和（向きに依らない集約） ----
    ng = 0;
    for (int it = 0; it < 200; it++) {
      int n = 1 + (int)(rng() % 20);
      vector<ll> a(n);
      vector<S> v(n);
      for (int i = 0; i < n; i++) {
        a[i] = (ll)(rng() % 100) - 50;
        v[i] = mk(a[i]);
      }
      ltree t(v);
      for (int q = 0; q < 60; q++) {
        int l = (int)(rng() % (n + 1)), r = (int)(rng() % (n + 1));
        if (l > r) swap(l, r);
        std::reverse(a.begin() + l, a.begin() + r);
        t.reverse(l, r);

        vector<ll> got;
        for (auto& x : t.to_vec()) got.push_back(x.sum);
        check(got == a, "reverse 後の列");

        int ql = (int)(rng() % (n + 1)), qr = (int)(rng() % (n + 1));
        if (ql > qr) swap(ql, qr);
        ll want = 0;
        for (int i = ql; i < qr; i++) want += a[i];
        check(t.prod(ql, qr).sum == want, "reverse 後の prod");
      }
    }
    report("reverse（和）");
  }

  {  // ---- reverse: 文字列連結（rev が効かないと壊れる） ----
    ng = 0;
    for (int it = 0; it < 200; it++) {
      int n = 1 + (int)(rng() % 14);
      string a;
      vector<T> v(n);
      for (int i = 0; i < n; i++) {
        char c = char('a' + rng() % 5);
        a += c;
        v[i].s = string(1, c);
      }
      ctree t(v);
      for (int q = 0; q < 30; q++) {
        int l = (int)(rng() % (n + 1)), r = (int)(rng() % (n + 1));
        if (l > r) swap(l, r);
        std::reverse(a.begin() + l, a.begin() + r);
        t.reverse(l, r);

        check(t.all_prod().s == a, "reverse 後の全体");
        for (int ql = 0; ql <= n; ql++)
          for (int qr = ql; qr <= n; qr++) check(t.prod(ql, qr).s == a.substr(ql, qr - ql), "部分区間の連結");
      }
    }
    report("reverse（文字列連結 / rev あり）");
  }

  {  // ---- reverse と apply / insert / erase を混ぜる ----
    ng = 0;
    for (int it = 0; it < 200; it++) {
      vector<ll> a;
      ltree t;
      for (int q = 0; q < 100; q++) {
        int n = (int)a.size();
        int kind = (int)(rng() % 100);
        if (kind < 30 || n == 0) {
          int i = (int)(rng() % (n + 1));
          ll x = (ll)(rng() % 20) - 10;
          a.insert(a.begin() + i, x);
          t.insert(i, mk(x));
        } else if (kind < 45) {
          int l = (int)(rng() % (n + 1)), r = (int)(rng() % (n + 1));
          if (l > r) swap(l, r);
          a.erase(a.begin() + l, a.begin() + r);
          t.erase(l, r);
        } else if (kind < 70) {
          int l = (int)(rng() % (n + 1)), r = (int)(rng() % (n + 1));
          if (l > r) swap(l, r);
          ll f = (ll)(rng() % 11) - 5;
          for (int i = l; i < r; i++) a[i] += f;
          t.apply(l, r, f);
        } else {
          int l = (int)(rng() % (n + 1)), r = (int)(rng() % (n + 1));
          if (l > r) swap(l, r);
          std::reverse(a.begin() + l, a.begin() + r);
          t.reverse(l, r);
        }
        vector<ll> got;
        for (auto& x : t.to_vec()) got.push_back(x.sum);
        check(got == a, "reverse と他の操作の混在");
      }
    }
    report("reverse と構造変化の混在");
  }

  {  // ---- reverse の境界 ----
    ng = 0;
    ltree t(vector<S>{mk(1), mk(2), mk(3), mk(4)});
    t.reverse(1, 1);
    check(raw2(t) == (vector<ll>{1, 2, 3, 4}), "空区間の reverse");
    t.reverse(2, 1);
    check(raw2(t) == (vector<ll>{1, 2, 3, 4}), "l > r の reverse");
    t.reverse(0, 4);
    check(raw2(t) == (vector<ll>{4, 3, 2, 1}), "全体 reverse");
    t.reverse(1, 3);
    check(raw2(t) == (vector<ll>{4, 2, 3, 1}), "部分 reverse");
    t.reverse(0, 1);
    check(raw2(t) == (vector<ll>{4, 2, 3, 1}), "1 要素 reverse は変化なし");
    // 反転してから作用、作用してから反転
    ltree u(vector<S>{mk(1), mk(2), mk(3)});
    u.reverse(0, 3);
    u.apply(0, 2, 10);
    check(raw2(u) == (vector<ll>{13, 12, 1}), "反転してから apply");
    report("reverse の境界");
  }

  {  // ---- rotate: std::rotate と突き合わせる ----
    ng = 0;
    for (int it = 0; it < 300; it++) {
      int n = 1 + (int)(rng() % 20);
      vector<ll> a(n);
      vector<S> v(n);
      for (int i = 0; i < n; i++) {
        a[i] = (ll)(rng() % 100) - 50;
        v[i] = mk(a[i]);
      }
      ltree t(v);
      for (int q = 0; q < 50; q++) {
        int l = (int)(rng() % (n + 1)), r = (int)(rng() % (n + 1));
        if (l > r) swap(l, r);
        // k は (-(r-l), r-l) の範囲。負は右回り
        int k = r - l > 1 ? (int)(rng() % (2 * (r - l) - 1)) - (r - l) + 1 : 0;
        if (r - l > 1) {
          int m = k < 0 ? k + (r - l) : k;
          std::rotate(a.begin() + l, a.begin() + l + m, a.begin() + r);
        }
        t.rotate(l, r, k);

        vector<ll> got;
        for (auto& x : t.to_vec()) got.push_back(x.sum);
        check(got == a, "rotate 後の列");

        int ql = (int)(rng() % (n + 1)), qr = (int)(rng() % (n + 1));
        if (ql > qr) swap(ql, qr);
        ll want = 0;
        for (int i = ql; i < qr; i++) want += a[i];
        check(t.prod(ql, qr).sum == want, "rotate 後の prod");
      }
    }
    report("rotate（std::rotate と比較）");
  }

  {  // ---- rotate と他の操作を混ぜる ----
    ng = 0;
    for (int it = 0; it < 200; it++) {
      vector<ll> a;
      ltree t;
      for (int q = 0; q < 100; q++) {
        int n = (int)a.size();
        int kind = (int)(rng() % 100);
        if (kind < 30 || n == 0) {
          int i = (int)(rng() % (n + 1));
          ll x = (ll)(rng() % 20) - 10;
          a.insert(a.begin() + i, x);
          t.insert(i, mk(x));
        } else if (kind < 42) {
          int l = (int)(rng() % (n + 1)), r = (int)(rng() % (n + 1));
          if (l > r) swap(l, r);
          a.erase(a.begin() + l, a.begin() + r);
          t.erase(l, r);
        } else if (kind < 60) {
          int l = (int)(rng() % (n + 1)), r = (int)(rng() % (n + 1));
          if (l > r) swap(l, r);
          ll f = (ll)(rng() % 11) - 5;
          for (int i = l; i < r; i++) a[i] += f;
          t.apply(l, r, f);
        } else if (kind < 78) {
          int l = (int)(rng() % (n + 1)), r = (int)(rng() % (n + 1));
          if (l > r) swap(l, r);
          std::reverse(a.begin() + l, a.begin() + r);
          t.reverse(l, r);
        } else {
          int l = (int)(rng() % (n + 1)), r = (int)(rng() % (n + 1));
          if (l > r) swap(l, r);
          int k = r - l > 1 ? (int)(rng() % (2 * (r - l) - 1)) - (r - l) + 1 : 0;
          if (r - l > 1) {
            int m = k < 0 ? k + (r - l) : k;
            std::rotate(a.begin() + l, a.begin() + l + m, a.begin() + r);
          }
          t.rotate(l, r, k);
        }
        vector<ll> got;
        for (auto& x : t.to_vec()) got.push_back(x.sum);
        check(got == a, "rotate を含む混在");
      }
    }
    report("rotate と構造変化の混在");
  }

  {  // ---- rotate の境界 ----
    ng = 0;
    ltree t(vector<S>{mk(1), mk(2), mk(3), mk(4), mk(5)});
    t.rotate(0, 5, 2);
    check(raw2(t) == (vector<ll>{3, 4, 5, 1, 2}), "左へ 2");
    t.rotate(0, 5, -2);
    check(raw2(t) == (vector<ll>{1, 2, 3, 4, 5}), "負なら逆向き（戻る）");
    t.rotate(0, 5, 4);
    check(raw2(t) == (vector<ll>{5, 1, 2, 3, 4}), "端まで左へ");
    t.rotate(0, 5, -4);
    check(raw2(t) == (vector<ll>{1, 2, 3, 4, 5}), "端まで右へ（戻る）");
    t.rotate(0, 5, 0);
    check(raw2(t) == (vector<ll>{1, 2, 3, 4, 5}), "k = 0 は何もしない");
    t.rotate(1, 4, 1);
    check(raw2(t) == (vector<ll>{1, 3, 4, 2, 5}), "部分区間");
    t.rotate(2, 3, 1);
    check(raw2(t) == (vector<ll>{1, 3, 4, 2, 5}), "長さ 1 なら何もしない");
    t.rotate(2, 2, 3);
    check(raw2(t) == (vector<ll>{1, 3, 4, 2, 5}), "空区間なら何もしない");
    t.rotate(3, 2, 1);
    check(raw2(t) == (vector<ll>{1, 3, 4, 2, 5}), "l > r なら何もしない");
    check(t.all_prod().sum == 15, "総和は変わらない");
    report("rotate の境界");
  }

  {  // ---- まとまった葉: 巨大な n でも構築できるか ----
    ng = 0;
    const int BIG = 1000000000;
    ltree t(BIG, mk(2));
    check(t.size() == BIG, "size が n");
    check(t.prod(0, 10).sum == 20, "先頭 10 個の和");
    check(t.prod(BIG - 3, BIG).sum == 6, "末尾 3 個の和");
    check(t.get(BIG / 2).sum == 2, "真ん中の 1 個");
    check(t.all_prod().sum == 2LL * BIG, "全体の和");

    t.apply(0, BIG, 1);  // 全部に +1
    check(t.all_prod().sum == 3LL * BIG, "全体 apply");
    t.apply(5, 8, 10);  // 途中だけ +10。ここで葉が割れる
    check(t.prod(5, 8).sum == 39, "割れた区間");
    check(t.prod(0, 5).sum == 15, "割れた左");
    check(t.all_prod().sum == 3LL * BIG + 30, "割った後の全体");

    t.set(7, mk(100));
    check(t.get(7).sum == 100, "set で 1 個だけ変わる");
    check(t.get(6).sum == 13, "同じ区間の隣は 13 のまま");
    check(t.get(8).sum == 3, "区間外の隣は 3 のまま");

    t.erase(0, 1000);
    check(t.size() == BIG - 1000, "まとめて消す");
    report("まとまった葉（n = 1e9）");
  }

  {  // ---- insert(i, x, k): まとめて挿す ----
    ng = 0;
    ltree t;
    t.insert(0, mk(5), 4);
    check(raw2(t) == (vector<ll>{5, 5, 5, 5}), "4 個まとめて");
    t.insert(2, mk(9), 3);
    check(raw2(t) == (vector<ll>{5, 5, 9, 9, 9, 5, 5}), "途中に 3 個");
    t.insert(7, mk(1), 2);
    check(raw2(t) == (vector<ll>{5, 5, 9, 9, 9, 5, 5, 1, 1}), "末尾に 2 個");
    check(t.size() == 9, "size");
    check(t.all_prod().sum == 5 * 4 + 9 * 3 + 1 * 2, "全体の和");
    report("insert(i, x, k)");
  }

  {  // ---- まとまった葉を素朴な実装と突き合わせる ----
    ng = 0;
    for (int it = 0; it < 300; it++) {
      // 同じ値が続く列にして、葉がまとまりやすい状況を作る
      vector<ll> a;
      ltree t;
      {
        int k = 1 + (int)(rng() % 8);
        ll x = (ll)(rng() % 5);
        t = ltree(k, mk(x));
        a.assign(k, x);
      }
      for (int q = 0; q < 80; q++) {
        int n = (int)a.size();
        int kind = (int)(rng() % 100);
        if (kind < 25) {  // まとめて挿す
          int i = (int)(rng() % (n + 1)), k = 1 + (int)(rng() % 4);
          ll x = (ll)(rng() % 5);
          a.insert(a.begin() + i, (size_t)k, x);
          t.insert(i, mk(x), k);
        } else if (kind < 40 && n > 0) {
          int i = (int)(rng() % n);
          a.erase(a.begin() + i);
          t.erase(i);
        } else if (kind < 52 && n > 0) {
          int l = (int)(rng() % (n + 1)), r = (int)(rng() % (n + 1));
          if (l > r) swap(l, r);
          a.erase(a.begin() + l, a.begin() + r);
          t.erase(l, r);
        } else if (kind < 64 && n > 0) {
          int i = (int)(rng() % n);
          ll x = (ll)(rng() % 5);
          a[i] = x;
          t.set(i, mk(x));
        } else if (kind < 80) {
          int l = (int)(rng() % (n + 1)), r = (int)(rng() % (n + 1));
          if (l > r) swap(l, r);
          ll f = (ll)(rng() % 7) - 3;
          for (int i = l; i < r; i++) a[i] += f;
          t.apply(l, r, f);
        } else if (kind < 90) {
          int l = (int)(rng() % (n + 1)), r = (int)(rng() % (n + 1));
          if (l > r) swap(l, r);
          std::reverse(a.begin() + l, a.begin() + r);
          t.reverse(l, r);
        } else {
          int l = (int)(rng() % (n + 1)), r = (int)(rng() % (n + 1));
          if (l > r) swap(l, r);
          int k = r - l > 1 ? (int)(rng() % (2 * (r - l) - 1)) - (r - l) + 1 : 0;
          if (r - l > 1) {
            int m = k < 0 ? k + (r - l) : k;
            std::rotate(a.begin() + l, a.begin() + l + m, a.begin() + r);
          }
          t.rotate(l, r, k);
        }
        check(raw2(t) == a, "まとまった葉を含む混在");
        int ql = (int)(rng() % ((int)a.size() + 1));
        int qr = (int)(rng() % ((int)a.size() + 1));
        if (ql > qr) swap(ql, qr);
        ll want = 0;
        for (int i = ql; i < qr; i++) want += a[i];
        check(t.prod(ql, qr).sum == want, "まとまった葉を含む prod");
      }
    }
    report("まとまった葉 x 素朴な実装");
  }

  {  // ---- 非可換な op でも冪が正しいか ----
    ng = 0;
    for (int k = 1; k <= 40; k++) {
      vector<T> v(1);
      v[0].s = "ab";
      avl_segtree<T, op_cat, e_cat> t(k, v[0]);
      string want;
      for (int i = 0; i < k; i++) want += "ab";
      check(t.all_prod().s == want, "ab を " + to_string(k) + " 個");
      check(t.prod(0, k).s == want, "prod で全体");
      if (k >= 2) check(t.prod(1, k - 1).s == want.substr(2, (k - 2) * 2), "prod で一部");
    }
    report("まとまった葉の冪（文字列連結）");
  }

  {  // ---- Beats: 区間 chmin を素朴な実装と突き合わせる ----
    ng = 0;
    for (int it = 0; it < 300; it++) {
      int n = 1 + (int)(rng() % 20);
      vector<ll> a(n);
      vector<B> v(n);
      for (int i = 0; i < n; i++) {
        a[i] = (ll)(rng() % 50);
        v[i] = mkb(a[i]);
      }
      btree t(v);
      for (int q = 0; q < 60; q++) {
        int l = (int)(rng() % (n + 1)), r = (int)(rng() % (n + 1));
        if (l > r) swap(l, r);
        ll f = (ll)(rng() % 50);
        for (int i = l; i < r; i++) a[i] = min(a[i], f);
        t.apply(l, r, f);

        check(raw2(t) == a, "chmin 後の列");
        int ql = (int)(rng() % (n + 1)), qr = (int)(rng() % (n + 1));
        if (ql > qr) swap(ql, qr);
        ll ws = 0, wm = LLONG_MIN / 4;
        for (int i = ql; i < qr; i++) {
          ws += a[i];
          wm = max(wm, a[i]);
        }
        auto got = t.prod(ql, qr);
        check(got.sum == ws, "chmin 後の区間和");
        check(qr <= ql || got.mx == wm, "chmin 後の区間最大");
      }
    }
    report("Beats（区間 chmin）");
  }

  {  // ---- Beats と構造変化を混ぜる ----
    ng = 0;
    for (int it = 0; it < 200; it++) {
      vector<ll> a;
      btree t;
      for (int q = 0; q < 80; q++) {
        int n = (int)a.size();
        int kind = (int)(rng() % 100);
        if (kind < 30 || n == 0) {
          int i = (int)(rng() % (n + 1));
          ll x = (ll)(rng() % 40);
          a.insert(a.begin() + i, x);
          t.insert(i, mkb(x));
        } else if (kind < 45) {
          int i = (int)(rng() % n);
          a.erase(a.begin() + i);
          t.erase(i);
        } else if (kind < 75) {
          int l = (int)(rng() % (n + 1)), r = (int)(rng() % (n + 1));
          if (l > r) swap(l, r);
          ll f = (ll)(rng() % 40);
          for (int i = l; i < r; i++) a[i] = min(a[i], f);
          t.apply(l, r, f);
        } else {
          int l = (int)(rng() % (n + 1)), r = (int)(rng() % (n + 1));
          if (l > r) swap(l, r);
          std::reverse(a.begin() + l, a.begin() + r);
          t.reverse(l, r);
        }
        check(raw2(t) == a, "Beats と構造変化の混在");
        ll all = 0;
        for (ll x : a) all += x;
        check(t.all_prod().sum == all, "全体の和");
      }
    }
    report("Beats と構造変化の混在");
  }

  {  // ---- Beats: まとまった葉と組み合わせる ----
    ng = 0;
    const int BIG = 1000000;
    btree t(BIG, mkb(100));
    check(t.all_prod().sum == 100LL * BIG, "まとめて構築");
    t.apply(0, BIG, 50);  // 全部 50 に
    check(t.all_prod().sum == 50LL * BIG, "全体 chmin");
    t.apply(10, 20, 5);  // 途中だけ 5 に。葉が割れる
    check(t.prod(10, 20).sum == 50, "割れた区間");
    check(t.prod(0, 10).sum == 500, "割れた左");
    check(t.all_prod().sum == 50LL * BIG - 450, "割った後の全体");
    check(t.prod(0, BIG).mx == 50, "全体の最大");
    check(t.prod(10, 20).mx == 5, "割れた区間の最大");
    report("Beats とまとまった葉");
  }

#ifdef _GLIBCXX_DEBUG
  puts("速度計測                           : _GLIBCXX_DEBUG のため省略");
#else
  {  // ---- 速度 ----
    const int N = 200000, Q = 200000;
    auto bench = [&](const char* name, auto f) {
      auto st = chrono::steady_clock::now();
      f();
      printf("%-34s : %lld ms\n", name, (ll)chrono::duration_cast<chrono::milliseconds>(chrono::steady_clock::now() - st).count());
    };
    vector<S> v(N);
    for (int i = 0; i < N; i++) v[i] = mk(i);
    tree t(v);
    bench("速度 構築 N=2e5", [&] { tree u(v); });
    bench("速度 prod x2e5", [&] {
      ll s = 0;
      for (int q = 0; q < Q; q++) {
        int l = (int)(rng() % N), r = (int)(rng() % N);
        if (l > r) swap(l, r);
        s += t.prod(l, r).sum;
      }
      check(s != 0, "計算が消えない");
    });
    bench("速度 apply x2e5", [&] {
      ltree u(v);
      for (int q = 0; q < Q; q++) {
        int l = (int)(rng() % N), r = (int)(rng() % N);
        if (l > r) swap(l, r);
        u.apply(l, r, 1);
      }
      check(u.size() == N, "要素数は変わらない");
    });
    bench("速度 reverse x2e5", [&] {
      ltree u(v);
      for (int q = 0; q < Q; q++) {
        int l = (int)(rng() % N), r = (int)(rng() % N);
        if (l > r) swap(l, r);
        u.reverse(l, r);
      }
      check(u.size() == N, "要素数は変わらない");
    });
    bench("速度 rotate x2e5", [&] {
      ltree u(v);
      for (int q = 0; q < Q; q++) {
        int l = (int)(rng() % N), r = (int)(rng() % N);
        if (l > r) swap(l, r);
        u.rotate(l, r, r - l > 1 ? (int)(rng() % (r - l)) : 0);
      }
      check(u.size() == N, "要素数は変わらない");
    });
    bench("速度 insert x2e5", [&] {
      tree u;
      for (int q = 0; q < Q; q++) u.insert((int)(rng() % (u.size() + 1)), mk(1));
      check(u.size() == Q, "全部入る");
    });
  }
#endif

  printf(ng_total ? "\nNG %d 件\n" : "\nすべて OK\n", ng_total);
  return ng_total ? 1 : 0;
}
