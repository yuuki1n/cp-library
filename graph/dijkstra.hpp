#include <algorithm>
#include <cassert>
#include <limits>
#include <queue>
#include <type_traits>
#include <utility>
#include <vector>

namespace dijkstra_internal {
// 未到達を表す既定値。テンプレートの LINF と同じ値にしてある。
// W に入りきらない型（int など）では max() / 2 に落とす。
constexpr long long LINF_ = 2002003004005006007LL;
template <class W>
constexpr W default_inf() {
  if constexpr (std::is_integral_v<W>) {
    if constexpr (std::numeric_limits<W>::max() / 2 >= LINF_)
      return W(LINF_);
    else
      return std::numeric_limits<W>::max() / 2;
  } else {
    return W(LINF_);
  }
}
}  // namespace dijkstra_internal

/*
 * dijkstra : 単一始点最短路（重みが非負のとき）
 *
 *   O((n + m) log n)。負の重みがあると正しく求まらない。
 *   graph.hpp の graph を渡す前提だが、include はしない。次を満たす型なら
 *   何でも渡せる。
 *     - 距離の型を weight_type という名前で公開している
 *     - size() が頂点数を返す
 *     - g[u] が u から出る辺の範囲を返し、辺が to / w / id を持つ
 *
 *   dijkstra(g, s)           始点 s から
 *   dijkstra(g, src)         多始点（src は vector<int>）。どれかからの最短
 *   dijkstra(g, s, inf)      未到達を表す値を指定する
 *
 *   dist[v]                  s から v への最短距離。未到達なら inf
 *   reachable(v)             到達できるか
 *   par[v]                   最短路木での直前の頂点。始点と未到達は -1
 *   par_edge[v]              そこへ来るのに使った辺番号。始点と未到達は -1
 *   path(t)                  s から t への頂点列。未到達なら空
 *
 *   inf の既定はテンプレートの LINF と同じ 2002003004005006007。W がこれを
 *   入れられない型（int など）では numeric_limits<W>::max() / 2 に落ちる。
 *   距離の総和が inf を超える問題では明示的に渡すこと。
 *
 *   reachable(v) の中身は dist[v] < inf。inf に -1 のような小さい値を渡すと
 *   常に false になるので、そういう使い方をするなら reachable は使わない。
 *
 * 使用例:
 *   graph<ll> g(N);
 *   rep(M) { INT(u, v); LL(w); g.add_edge(--u, --v, w); }
 *
 *   dijkstra d(g, 0);
 *   print(d.dist[N - 1]);
 *   if (d.reachable(N - 1)) print(d.path(N - 1));   // 頂点列
 *   else print(-1);
 *
 *   // 多始点。複数のマスから同時に広げる
 *   dijkstra d2(g, vi{0, 3, 5});
 *
 *   // 通った辺の番号を並べる
 *   vi es;
 *   for (int v = t; d.par[v] != -1; v = d.par[v]) es.pb(d.par_edge[v]);
 *   rg::reverse(es);
 *
 * verify:
 *   (未 verify)
 *   予定: https://judge.yosupo.jp/problem/shortest_path
 */
template <class G>
struct dijkstra {
  using W = typename G::weight_type;

  std::vector<W> dist;
  std::vector<int> par;       // 直前の頂点
  std::vector<int> par_edge;  // そこへ来るのに使った辺番号
  W inf;

  dijkstra(const G& g, int s, W inf_ = dijkstra_internal::default_inf<W>())
      : dijkstra(g, std::vector<int>{s}, inf_) {}

  dijkstra(const G& g, const std::vector<int>& src,
           W inf_ = dijkstra_internal::default_inf<W>())
      : dist(g.size(), inf_),
        par(g.size(), -1),
        par_edge(g.size(), -1),
        inf(inf_) {
    using P = std::pair<W, int>;
    std::priority_queue<P, std::vector<P>, std::greater<P>> pq;
    for (int s : src) {
      assert(0 <= s && s < g.size());
      dist[s] = W(0);
      pq.emplace(W(0), s);
    }
    while (!pq.empty()) {
      auto [d, u] = pq.top();
      pq.pop();
      if (d > dist[u]) continue;  // 古くなった要素は捨てる
      for (const auto& e : g[u]) {
        W nd = d + e.w;
        if (nd < dist[e.to]) {
          dist[e.to] = nd;
          par[e.to] = u;
          par_edge[e.to] = e.id;
          pq.emplace(nd, e.to);
        }
      }
    }
  }

  bool reachable(int v) const { return dist[v] < inf; }

  // s から t への頂点列。未到達なら空
  std::vector<int> path(int t) const {
    if (!reachable(t)) return {};
    std::vector<int> ret;
    for (int v = t; v != -1; v = par[v]) ret.push_back(v);
    std::reverse(ret.begin(), ret.end());
    return ret;
  }
};
