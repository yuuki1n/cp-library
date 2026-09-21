// union-find 系の機能ごとの速度を測る。正しさは union_find_test.cpp が見る。
// ファイル名が *_test.cpp ではないので CI では回らない（測定は環境依存で揺れるため）。
//
//   g++ -std=gnu++20 -O2 -Wall -Wextra -I../../.. union_find_bench.cpp -o union_find_bench
//   ./union_find_bench                    # 既定 N = 200000, Q = 400000
//   ./union_find_bench 1000000            # N を変える（Q は 2N）
//   ./union_find_bench 200000 400000      # N と Q を別々に
//   ./union_find_bench 200000 400000 key  # 名前に key を含む項目だけ
//
// 各項目は「準備を時間に入れず、本体だけを 3 回測って最小値」を出す。
// 軸ごとの代償（巻き戻し・値・キー）が見えるよう、同じ操作を並べてある。
#include <algorithm>
#include <chrono>
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <random>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "../../../graph/union_find/keyed_union_find.hpp"
#include "../../../graph/union_find/relational_union_find.hpp"
#include "../../../graph/union_find/union_find.hpp"

using ll = long long;
using namespace std;

ll op_max(ll a, ll b) { return a > b ? a : b; }
ll e_min() { return LLONG_MIN / 4; }

/* ---- 測定 ---- */
ll sink = 0;  // 最適化で本体ごと消されないように結果を溜める
int N = 200000, Q = 400000;
const char* filter = nullptr;   // 名前にこれを含む項目だけ測る
const char* pending = nullptr;  // まだ出していない見出し。中身が空の節は出さない

mt19937 rng_setup(12345);
mt19937 rng_body(67890);

// 併合する辺と、問い合わせる頂点対。どの項目も同じ列を使う
vector<int> ea, eb, qa, qb;

template <class Setup, class Body> void bench(const char* name, ll ops, Setup setup, Body body) {
  if (filter && !strstr(name, filter)) return;
  if (pending) printf("\n[%s]\n", pending), pending = nullptr;
  double best = 1e18;
  for (int t = 0; t < 3; t++) {
    rng_body.seed(67890);
    auto obj = setup();
    auto st = chrono::steady_clock::now();
    body(obj);
    double ms = chrono::duration<double, milli>(chrono::steady_clock::now() - st).count();
    best = min(best, ms);
  }
  printf("  %-42s %8.1f ms %9.1f ns/op\n", name, best, best * 1e6 / (double)ops);
  fflush(stdout);
}

void head(const char* s) { pending = s; }

int main(int argc, char** argv) {
  if (argc > 1) N = atoi(argv[1]), Q = 2 * N;
  if (argc > 2) Q = atoi(argv[2]);
  if (argc > 3) filter = argv[3];
  printf("union_find bench  N = %d, Q = %d\n", N, Q);

  ea.resize(Q), eb.resize(Q), qa.resize(Q), qb.resize(Q);
  for (int i = 0; i < Q; i++) {
    ea[i] = (int)(rng_setup() % (unsigned)N), eb[i] = (int)(rng_setup() % (unsigned)N);
    qa[i] = (int)(rng_setup() % (unsigned)N), qb[i] = (int)(rng_setup() % (unsigned)N);
  }
  // 問い合わせ系の準備。全部つないだ状態から測る
  auto linked = [] {
    union_find<> u(N);
    for (int i = 0; i < Q; i++) u.merge(ea[i], eb[i]);
    return u;
  };

  head("素の union_find");
  bench("merge x Q", Q, [] { return union_find<>(N); },
        [](union_find<>& u) {
          for (int i = 0; i < Q; i++) sink += u.merge(ea[i], eb[i]);
        });
  bench("same x Q（併合済み）", Q, linked, [](union_find<>& u) {
    for (int i = 0; i < Q; i++) sink += u.same(qa[i], qb[i]);
  });
  bench("leader x Q（併合済み）", Q, linked, [](union_find<>& u) {
    for (int i = 0; i < Q; i++) sink += u.leader(qa[i]);
  });
  bench("size x Q（併合済み）", Q, linked, [](union_find<>& u) {
    for (int i = 0; i < Q; i++) sink += u.size(qa[i]);
  });
  bench("merge と same を交互 x Q", Q, [] { return union_find<>(N); },
        [](union_find<>& u) {
          for (int i = 0; i < Q; i++) sink += u.merge(ea[i], eb[i]) + u.same(qa[i], qb[i]);
        });
  bench("group(x) x Q（孤立点）", Q, [] { return union_find<>(N); },
        [](union_find<>& u) {
          for (int i = 0; i < Q; i++) sink += (ll)u.group(qa[i]).size();
        });
  // groups() は成分の大きさによらず O(n) なので、頂点あたりで見る
  bench("groups() x 10（併合済み。頂点あたり）", 10LL * N, linked, [](union_find<>& u) {
    for (int t = 0; t < 10; t++) sink += (ll)u.groups().size();
  });

  head("巻き戻し（経路圧縮なしの代償）");
  bench("rollback merge x Q", Q, [] { return rollback_union_find<>(N); },
        [](rollback_union_find<>& u) {
          for (int i = 0; i < Q; i++) sink += u.merge(ea[i], eb[i]);
        });
  bench("rollback same x Q（併合済み）", Q,
        [] {
          rollback_union_find<> u(N);
          for (int i = 0; i < Q; i++) u.merge(ea[i], eb[i]);
          return u;
        },
        [](rollback_union_find<>& u) {
          for (int i = 0; i < Q; i++) sink += u.same(qa[i], qb[i]);
        });
  bench("rollback merge x Q -> 全部 undo", 2 * Q, [] { return rollback_union_find<>(N); },
        [](rollback_union_find<>& u) {
          for (int i = 0; i < Q; i++) u.merge(ea[i], eb[i]);
          u.rollback(0);
          sink += u.group_count();
        });

  head("値を載せる");
  bench("値つき merge x Q（和）", Q, [] { return union_find<ll>(N); },
        [](union_find<ll>& u) {
          for (int i = 0; i < Q; i++) sink += u.merge(ea[i], eb[i]);
        });
  bench("prod(x) x Q（併合済み）", Q,
        [] {
          union_find<ll> u(N);
          for (int i = 0; i < Q; i++) u.merge(ea[i], eb[i]);
          return u;
        },
        [](union_find<ll>& u) {
          for (int i = 0; i < Q; i++) sink += u.prod(qa[i]);
        });
  bench("値つき巻き戻し merge x Q（max）", Q, [] { return rollback_union_find<ll, op_max, e_min>(N); },
        [](rollback_union_find<ll, op_max, e_min>& u) {
          for (int i = 0; i < Q; i++) sink += u.merge(ea[i], eb[i]);
        });
  bench("コールバックで和を維持 merge x Q", Q, [] { return union_find<>(N); },
        [](union_find<>& u) {
          vector<ll> s(N, 1);
          for (int i = 0; i < Q; i++) sink += u.merge(ea[i], eb[i], [&](int to, int from) { s[to] += s[from]; });
          sink += s[0];
        });

  head("ポテンシャル");
  bench("relational merge x Q", Q, [] { return relational_union_find<>(N); },
        [](relational_union_find<>& u) {
          for (int i = 0; i < Q; i++) sink += u.merge(ea[i], eb[i], i);
        });
  bench("relational diff x Q（併合済み）", Q,
        [] {
          relational_union_find<> u(N);
          for (int i = 0; i < Q; i++) u.merge(ea[i], eb[i], i);
          return u;
        },
        [](relational_union_find<>& u) {
          for (int i = 0; i < Q; i++)
            if (u.same(qa[i], qb[i])) sink += u.diff(qa[i], qb[i]);
        });
  bench("rollback relational merge x Q", Q, [] { return rollback_relational_union_find<>(N); },
        [](rollback_relational_union_find<>& u) {
          for (int i = 0; i < Q; i++) sink += u.merge(ea[i], eb[i], i);
        });

  head("キーで引く（Map の代償）");
  bench("keyed<ll, map> merge x Q", Q, [] { return keyed_union_find<ll, union_find<>>(); },
        [](keyed_union_find<ll, union_find<>>& u) {
          for (int i = 0; i < Q; i++) sink += u.merge(ea[i], eb[i]);
        });
  bench("keyed<ll, unordered_map> merge x Q", Q,
        [] { return keyed_union_find<ll, union_find<>, unordered_map<ll, int>>(); },
        [](keyed_union_find<ll, union_find<>, unordered_map<ll, int>>& u) {
          for (int i = 0; i < Q; i++) sink += u.merge(ea[i], eb[i]);
        });
  bench("keyed<pair, map> merge x Q", Q, [] { return keyed_union_find<pair<int, int>, union_find<>>(); },
        [](keyed_union_find<pair<int, int>, union_find<>>& u) {
          for (int i = 0; i < Q; i++) sink += u.merge({ea[i] >> 8, ea[i] & 255}, {eb[i] >> 8, eb[i] & 255});
        });
  bench("keyed<string, map> merge x Q", Q, [] { return keyed_union_find<string, union_find<>>(); },
        [](keyed_union_find<string, union_find<>>& u) {
          for (int i = 0; i < Q; i++) sink += u.merge(to_string(ea[i]), to_string(eb[i]));
        });

  printf("\n（sink = %lld）\n", sink);
  return 0;
}
