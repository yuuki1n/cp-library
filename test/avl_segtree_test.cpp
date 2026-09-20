// data_structure/avl_segtree.hpp の検証。std::vector を素朴に操作したものと
// 突き合わせる。
//   g++ -std=gnu++20 -O2 -Wall -Wextra -I.. avl_segtree_test.cpp -o avl_segtree_test
#include "../data_structure/avl_segtree.hpp"

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
using tree = avl_segtree<S, op, e>;

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

vector<ll> raw(const tree& t) {
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
          check(t.prod(l, r).sum == s,
                "prod(" + to_string(l) + "," + to_string(r) + ")");
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
        for (int r = l; r <= n; r++)
          check(t.prod(l, r).s == a.substr(l, r - l), "連結の順序");
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
      check(w.get(2).sum == 30 && w.prod(0, 4).sum == 37,
            "set 後の get / prod");
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

#ifdef _GLIBCXX_DEBUG
  puts("速度計測                           : _GLIBCXX_DEBUG のため省略");
#else
  {  // ---- 速度 ----
    const int N = 200000, Q = 200000;
    auto bench = [&](const char* name, auto f) {
      auto st = chrono::steady_clock::now();
      f();
      printf("%-34s : %lld ms\n", name,
             (ll)chrono::duration_cast<chrono::milliseconds>(
                 chrono::steady_clock::now() - st)
                 .count());
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
    bench("速度 insert x2e5", [&] {
      tree u;
      for (int q = 0; q < Q; q++)
        u.insert((int)(rng() % (u.size() + 1)), mk(1));
      check(u.size() == Q, "全部入る");
    });
  }
#endif

  printf(ng_total ? "\nNG %d 件\n" : "\nすべて OK\n", ng_total);
  return ng_total ? 1 : 0;
}
