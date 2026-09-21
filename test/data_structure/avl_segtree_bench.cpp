// avl_segtree の機能ごとの速度を測る。正しさは avl_segtree_test.cpp が見る。
// ファイル名が *_test.cpp ではないので CI では回らない（測定は環境依存で揺れるため）。
//
//   g++ -std=gnu++20 -O2 -Wall -Wextra -I.. avl_segtree_bench.cpp -o avl_segtree_bench
//   ./avl_segtree_bench              # 既定 N = Q = 200000
//   ./avl_segtree_bench 1000000      # N を変える（Q は N と同じ）
//   ./avl_segtree_bench 200000 50000 # N と Q を別々に
//   ./avl_segtree_bench 200000 200000 apply  # 名前に apply を含む項目だけ
//
// 各項目は「準備を時間に入れず、本体だけを 3 回測って最小値」を出す。
// 実装を差し替えて比べるときは、同じ引数で両方を走らせて ns/op を並べる。
#include <chrono>
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <random>
#include <string>
#include <vector>

#include "../../data_structure/avl_segtree.hpp"

using ll = long long;
using namespace std;

/* ---- 区間和 + 区間加算。いちばん普通の使い方 ---- */
struct S : avl_value {
  ll sum = 0;
};
S op(S a, S b) {
  S r;
  r.sum = a.sum + b.sum;
  return r;
}
S e() { return S{}; }
using F = ll;
S mapping(F f, S x) {
  x.sum += f * x.sz;
  return x;
}
F composition(F f, F g) { return f + g; }
F id() { return 0; }
using tree = avl_segtree<S, op, e, F, mapping, composition, id>;

S mk(ll v) {
  S x;
  x.sum = v;
  return x;
}

/* ---- 非可換な op（文字列連結）。rev を指定する経路を通す ---- */
struct T : avl_value {
  string s;
};
T op_cat(T a, T b) {
  T r;
  r.s = a.s + b.s;
  return r;
}
T e_cat() { return T{}; }
T rev_cat(T x) {
  reverse(x.s.begin(), x.s.end());
  return x;
}
namespace ai = avl_segtree_internal;
using ctree = avl_segtree<T, op_cat, e_cat, ai::no_lazy, ai::map_<T>, ai::comp_, ai::id_, rev_cat>;

/* ---- Segment Tree Beats: 区間 chmin / 区間和 ---- */
struct B : avl_value {
  ll sum = 0;
  ll mx = LLONG_MIN / 4;
  ll mx2 = LLONG_MIN / 4;
  int cnt = 0;
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
B map_b(ll f, B x) {
  if (x.mx <= f) return x;
  if (x.mx2 < f) {
    x.sum -= (x.mx - f) * x.cnt;
    x.mx = f;
    return x;
  }
  x.fail = true;
  return x;
}
ll comp_b(ll f, ll g) { return min(f, g); }
ll id_b() { return LLONG_MAX / 4; }
using btree = avl_segtree<B, op_b, e_b, ll, map_b, comp_b, id_b>;

B mkb(ll v) {
  B x;
  x.sum = x.mx = v;
  x.cnt = 1;
  return x;
}

/* ---- 測定 ---- */
ll sink = 0;  // 最適化で本体ごと消されないように結果を溜める
int N = 200000, Q = 200000;
const char* filter = nullptr;  // 名前にこれを含む項目だけ測る

// 測るたびに同じ乱数列を使う。準備と本体で別のものを引く
mt19937 rng_setup(12345);
mt19937 rng_body(67890);

// setup() で作った木を body() に渡す。時計は body だけにかける
template <class Setup, class Body> void bench(const char* name, ll ops, Setup setup, Body body) {
  if (filter && !strstr(name, filter)) return;
  double best = 1e18;
  for (int t = 0; t < 3; t++) {
    rng_body.seed(67890);
    auto obj = setup();
    auto st = chrono::steady_clock::now();
    body(obj);
    double ms = chrono::duration<double, milli>(chrono::steady_clock::now() - st).count();
    best = min(best, ms);
  }
  printf("  %-30s %9.1f ms %10.1f ns/op\n", name, best, best * 1e6 / (double)ops);
  fflush(stdout);
}

// [l, r) をランダムに選ぶ。長さ 1 以上
pair<int, int> range(mt19937& r, int n) {
  int l = (int)(r() % (unsigned)n), rr = (int)(r() % (unsigned)n);
  if (l > rr) swap(l, rr);
  return {l, rr + 1};
}

vector<S> base_vec() {
  vector<S> v(N);
  for (int i = 0; i < N; i++) v[i] = mk((ll)(rng_setup() % 1000));
  return v;
}

// Q 回の操作で木が空にならないよう、消す系は N + Q 要素から始める
tree fresh() { return tree(base_vec()); }

int main(int argc, char** argv) {
  if (argc > 1) N = Q = atoi(argv[1]);
  if (argc > 2) Q = atoi(argv[2]);
  if (argc > 3) filter = argv[3];
  printf("avl_segtree bench  N = %d, Q = %d\n", N, Q);

  printf("\n[構築]\n");
  bench(
      "avl_segtree(vector) N 要素", N, [] { return base_vec(); },
      [](vector<S>& v) {
        tree t(v);
        sink += t.size();
      });
  // 葉 1 個で持つので O(1)。1 回では短すぎて測れないため 1000 回回す
  bench(
      "avl_segtree(n, x) N 要素 x1000", 1000, [] { return 0; },
      [](int&) {
        for (int i = 0; i < 1000; i++) {
          tree t(N, mk(1));
          sink += t.size();
        }
      });
  bench("to_vec() N 要素", N, fresh, [](tree& t) { sink += (ll)t.to_vec().size(); });

  printf("\n[読み出し]\n");
  bench("get(i)", Q, fresh, [](tree& t) {
    for (int q = 0; q < Q; q++) sink += t.get((int)(rng_body() % (unsigned)N)).sum;
  });
  bench("prod(l, r) ランダム区間", Q, fresh, [](tree& t) {
    for (int q = 0; q < Q; q++) {
      auto [l, r] = range(rng_body, N);
      sink += t.prod(l, r).sum;
    }
  });
  bench("all_prod()", Q, fresh, [](tree& t) {
    for (int q = 0; q < Q; q++) sink += t.all_prod().sum;
  });

  printf("\n[書き換え]\n");
  bench("set(i, x)", Q, fresh, [](tree& t) {
    for (int q = 0; q < Q; q++) t.set((int)(rng_body() % (unsigned)N), mk(q));
  });
  bench("apply(i, f) 1 点", Q, fresh, [](tree& t) {
    for (int q = 0; q < Q; q++) t.apply((int)(rng_body() % (unsigned)N), 1);
  });
  bench("apply(l, r, f) ランダム区間", Q, fresh, [](tree& t) {
    for (int q = 0; q < Q; q++) {
      auto [l, r] = range(rng_body, N);
      t.apply(l, r, 1);
    }
  });

  printf("\n[構造を変える]\n");
  bench(
      "insert(size(), x) 末尾に足す", Q, [] { return tree(); },
      [](tree& t) {
        for (int q = 0; q < Q; q++) t.insert(t.size(), mk(q));
      });
  bench("insert(i, x) ランダム位置", Q, fresh, [](tree& t) {
    for (int q = 0; q < Q; q++) t.insert((int)(rng_body() % (unsigned)t.size()), mk(q));
  });
  // まとめて挿す経路。要素数が int に収まる範囲で回す（N + Q * k <= 2^31）
  bench("insert(i, x, k) まとめて k=1000", Q, fresh, [](tree& t) {
    for (int q = 0; q < Q; q++) t.insert((int)(rng_body() % (unsigned)t.size()), mk(q), 1000);
  });
  bench(
      "erase(i) 1 点", Q, [] { return tree(vector<S>(N + Q, mk(1))); },
      [](tree& t) {
        for (int q = 0; q < Q; q++) t.erase((int)(rng_body() % (unsigned)t.size()));
      });
  // 1 回に 2 個消すので、Q 回ぶん余分に積んでおく（途中で空にしない）
  bench(
      "erase(l, r) ランダム区間", Q, [] { return tree(vector<S>(N + 2 * Q, mk(1))); },
      [](tree& t) {
        for (int q = 0; q < Q; q++) {
          int l = (int)(rng_body() % (unsigned)(t.size() - 2));
          t.erase(l, l + 2);
        }
      });
  bench("reverse(l, r) ランダム区間", Q, fresh, [](tree& t) {
    for (int q = 0; q < Q; q++) {
      auto [l, r] = range(rng_body, N);
      t.reverse(l, r);
    }
  });
  bench("rotate(l, r, k) ランダム区間", Q, fresh, [](tree& t) {
    for (int q = 0; q < Q; q++) {
      auto [l, r] = range(rng_body, N);
      int n = r - l;
      t.rotate(l, r, n > 1 ? (int)(rng_body() % (unsigned)(n - 1)) + 1 : 0);
    }
  });

  // 実際の問題は読み書きが交ざる。prod のたびに push が走る状態を測る
  printf("\n[混在]\n");
  bench("apply(l,r) と prod(l,r) を交互", Q, fresh, [](tree& t) {
    for (int q = 0; q < Q; q++) {
      auto [l, r] = range(rng_body, N);
      if (q & 1) t.apply(l, r, 1);
      else sink += t.prod(l, r).sum;
    }
  });
  bench("apply(l,r) と get(i) を交互", Q, fresh, [](tree& t) {
    for (int q = 0; q < Q; q++) {
      if (q & 1) {
        auto [l, r] = range(rng_body, N);
        t.apply(l, r, 1);
      } else {
        sink += t.get((int)(rng_body() % (unsigned)N)).sum;
      }
    }
  });
  bench("reverse と prod を交互", Q, fresh, [](tree& t) {
    for (int q = 0; q < Q; q++) {
      auto [l, r] = range(rng_body, N);
      if (q & 1) t.reverse(l, r);
      else sink += t.prod(l, r).sum;
    }
  });
  bench("insert と erase と prod を順に", Q, fresh, [](tree& t) {
    for (int q = 0; q < Q; q++) {
      int m = (int)t.size();
      if (q % 3 == 0) {
        t.insert((int)(rng_body() % (unsigned)m), mk(q));
      } else if (q % 3 == 1) {
        t.erase((int)(rng_body() % (unsigned)m));
      } else {
        auto [l, r] = range(rng_body, m);
        sink += t.prod(l, r).sum;
      }
    }
  });

  printf("\n[非可換な op（文字列連結）・rev あり]\n");
  {
    int n = min(N, 20000);  // op が O(長さ) なので小さめに回す
    int q = min(Q, 20000);
    auto setup = [n] {
      vector<T> v(n);
      for (int i = 0; i < n; i++) v[i].s = string(1, (char)('a' + i % 26));
      return ctree(v);
    };
    bench("prod(l, r) 連結", q, setup, [q, n](ctree& t) {
      for (int i = 0; i < q; i++) {
        auto [l, r] = range(rng_body, n);
        sink += (ll)t.prod(l, min(r, l + 32)).s.size();
      }
    });
    bench("reverse(l, r) 連結", q, setup, [q, n](ctree& t) {
      for (int i = 0; i < q; i++) {
        auto [l, r] = range(rng_body, n);
        t.reverse(l, r);
      }
    });
  }

  printf("\n[Beats（区間 chmin + 区間和）]\n");
  {
    auto setup = [] {
      vector<B> v(N);
      for (int i = 0; i < N; i++) v[i] = mkb((ll)(rng_setup() % 1000000000));
      return btree(v);
    };
    bench("apply(l, r, chmin)", Q, setup, [](btree& t) {
      for (int q = 0; q < Q; q++) {
        auto [l, r] = range(rng_body, N);
        t.apply(l, r, (ll)(rng_body() % 1000000000));
      }
    });
    bench("prod(l, r)", Q, setup, [](btree& t) {
      for (int q = 0; q < Q; q++) {
        auto [l, r] = range(rng_body, N);
        sink += t.prod(l, r).sum;
      }
    });
  }

  printf("\n(sink = %lld)\n", sink);
  return 0;
}
