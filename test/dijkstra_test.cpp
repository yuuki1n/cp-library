// graph.hpp / dijkstra.hpp の検証。Bellman-Ford / Floyd-Warshall と突き合わせる。
//   g++ -std=gnu++20 -O2 -Wall -Wextra -I.. dijkstra_test.cpp -o dijkstra_test
#include "../graph/dijkstra.hpp"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <limits>
#include <random>
#include <string>
#include <vector>

#include "../graph/graph.hpp"

using ll = long long;
using namespace std;

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

constexpr ll INF = 2002003004005006007LL;  // dijkstra の既定 inf と同じ値

// 参照実装: Bellman-Ford（多始点対応）
vector<ll> bellman(int n, const vector<array<int, 3>>& es,
                   const vector<int>& src) {
  vector<ll> d(n, INF);
  for (int s : src) d[s] = 0;
  for (int it = 0; it < n; it++)
    for (auto& e : es)
      if (d[e[0]] < INF && d[e[0]] + e[2] < d[e[1]]) d[e[1]] = d[e[0]] + e[2];
  return d;
}

int main() {
  mt19937 rng(20260905);

  {  // ---- 無向グラフ ----
    ng = 0;
    for (int iter = 0; iter < 400; iter++) {
      int n = 1 + (int)(rng() % 12);
      int m = (int)(rng() % 20);
      graph<ll> g(n);
      vector<array<int, 3>> es;  // 参照実装用（両向き）
      for (int i = 0; i < m; i++) {
        int u = (int)(rng() % n), v = (int)(rng() % n);
        ll w = (ll)(rng() % 50);
        int id = g.add_edge(u, v, w);
        check(id == i, "add_edge が辺番号を順に返す");
        es.push_back({u, v, (int)w});
        es.push_back({v, u, (int)w});
      }
      check(g.edge_count() == m, "edge_count");
      check(g.size() == n, "size");

      int s = (int)(rng() % n);
      dijkstra d(g, s);
      auto want = bellman(n, es, {s});
      for (int v = 0; v < n; v++) {
        check(d.dist[v] == want[v], "dist");
        check(d.reachable(v) == (want[v] < INF), "reachable");
      }
      // 経路の妥当性: 頂点列がつながっていて、距離の合計が dist と一致する
      for (int t = 0; t < n; t++) {
        auto p = d.path(t);
        if (!d.reachable(t)) {
          check(p.empty(), "未到達なら path は空");
          continue;
        }
        check(!p.empty() && p.front() == s && p.back() == t, "path の両端");
        ll sum = 0;
        bool ok = true;
        for (size_t i = 0; i + 1 < p.size(); i++) {
          ll best = INF;
          for (auto& e : g[p[i]])
            if (e.to == p[i + 1]) best = min(best, e.w);
          if (best == INF)
            ok = false;
          else
            sum += best;
        }
        check(ok, "path が実在する辺でつながっている");
        check(sum == d.dist[t], "path の重みの合計が dist と一致");
      }
    }
    report("無向グラフ");
  }

  {  // ---- 有向グラフ ----
    ng = 0;
    for (int iter = 0; iter < 400; iter++) {
      int n = 1 + (int)(rng() % 12);
      int m = (int)(rng() % 25);
      graph<ll> g(n, true);
      vector<array<int, 3>> es;
      for (int i = 0; i < m; i++) {
        int u = (int)(rng() % n), v = (int)(rng() % n);
        ll w = (ll)(rng() % 50);
        g.add_edge(u, v, w);
        es.push_back({u, v, (int)w});
      }
      check(g.is_directed(), "is_directed");
      int s = (int)(rng() % n);
      dijkstra d(g, s);
      auto want = bellman(n, es, {s});
      for (int v = 0; v < n; v++)
        check(d.dist[v] == want[v], "有向グラフの dist");
    }
    report("有向グラフ");
  }

  {  // ---- 多始点 ----
    ng = 0;
    for (int iter = 0; iter < 300; iter++) {
      int n = 1 + (int)(rng() % 12);
      int m = (int)(rng() % 20);
      graph<ll> g(n, true);
      vector<array<int, 3>> es;
      for (int i = 0; i < m; i++) {
        int u = (int)(rng() % n), v = (int)(rng() % n);
        ll w = (ll)(rng() % 50);
        g.add_edge(u, v, w);
        es.push_back({u, v, (int)w});
      }
      int k = 1 + (int)(rng() % n);
      vector<int> src;
      for (int i = 0; i < k; i++) src.push_back((int)(rng() % n));
      dijkstra d(g, src);
      auto want = bellman(n, es, src);
      for (int v = 0; v < n; v++) check(d.dist[v] == want[v], "多始点の dist");
      for (int s : src) check(d.dist[s] == 0, "始点の距離は 0");
    }
    report("多始点");
  }

  {  // ---- 重みなし / 重み 0 / 自己ループ / 多重辺 ----
    ng = 0;
    graph<int> g(4);
    g.add_edge(0, 1);  // 既定の重み 1
    g.add_edge(1, 2);
    g.add_edge(2, 3);
    dijkstra d(g, 0);
    check(d.dist[3] == 3, "重みなしグラフ");

    graph<ll> z(3, true);
    z.add_edge(0, 0, 5);  // 自己ループ
    z.add_edge(0, 1, 0);  // 重み 0
    z.add_edge(0, 1, 7);  // 多重辺（長いほう）
    z.add_edge(1, 2, 10);
    z.add_edge(1, 2, 3);  // 多重辺（短いほう）
    dijkstra dz(z, 0);
    check(dz.dist[0] == 0 && dz.dist[1] == 0 && dz.dist[2] == 3,
          "自己ループ / 重み 0 / 多重辺");
    check(dz.par_edge[2] == 4, "短いほうの多重辺を選ぶ");

    graph<ll> iso(3, true);
    dijkstra di(iso, 0);
    check(di.dist[1] == di.inf && !di.reachable(1), "孤立点は未到達");
    check(di.path(1).empty(), "未到達の path は空");
    check(di.path(0) == vector<int>{0}, "始点自身の path");

    graph<ll> one(1);
    dijkstra d1(one, 0);
    check(d1.dist[0] == 0, "頂点 1 個");

    // 既定の inf はテンプレートの LINF と同じ値
    check(di.inf == 2002003004005006007LL, "ll の既定 inf は LINF と同じ");
    check(di.dist[1] == 2002003004005006007LL, "未到達は LINF");
    graph<int> gi(2, true);
    dijkstra dii(gi, 0);
    check(dii.inf == numeric_limits<int>::max() / 2,
          "int では max() / 2 に落ちる");
    check(!dii.reachable(1), "int でも未到達を判定できる");

    // inf を明示的に渡す
    dijkstra dx(iso, 0, 12345LL);
    check(dx.inf == 12345 && dx.dist[1] == 12345, "inf を指定できる");
    report("端のケース");
  }

  {  // ---- graph::clear ----
    ng = 0;
    graph<ll> g(3);
    g.add_edge(0, 1, 5);
    g.add_edge(1, 2, 5);
    check(g.edge_count() == 2 && dijkstra(g, 0).dist[2] == 10, "clear 前");
    g.clear();
    check(g.edge_count() == 0 && g.size() == 3, "clear 後の状態");
    check(g[0].empty() && !dijkstra(g, 0).reachable(2), "clear で辺が消える");
    g.add_edge(0, 2, 1);
    check(g.edge_count() == 1 && dijkstra(g, 0).dist[2] == 1,
          "clear 後に足し直せる");
    report("graph::clear");
  }

#ifdef _GLIBCXX_DEBUG
  puts("速度計測                       : _GLIBCXX_DEBUG のため省略");
#else
  {  // ---- 速度 ----
    const int N = 200000, M = 400000;
    graph<ll> g(N, true);
    for (int i = 0; i < M; i++)
      g.add_edge((int)(rng() % N), (int)(rng() % N), (ll)(rng() % 1000000000));
    auto st = chrono::steady_clock::now();
    dijkstra d(g, 0);
    ll cnt = 0;
    for (int v = 0; v < N; v++) cnt += d.reachable(v);
    printf("%-30s : %lld ms  (到達 %lld / %d)\n", "速度 N=2e5 M=4e5",
           (long long)chrono::duration_cast<chrono::milliseconds>(
               chrono::steady_clock::now() - st)
               .count(),
           cnt, N);
  }
#endif

  printf(ng_total ? "\nNG %d 件\n" : "\nすべて OK\n", ng_total);
  return ng_total ? 1 : 0;
}
