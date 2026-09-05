#include <cassert>
#include <vector>

/*
 * graph<W> : グラフの入れ物（隣接リスト）
 *
 *   辺に重み W を持つ。無向グラフは add_edge が往復ぶんを入れる。往復には
 *   同じ辺番号が付くので、辺ごとの情報を配列で持ちたいときに使える。
 *
 *   graph(n, directed)  頂点 0 .. n-1。directed の既定は false（無向）
 *   add_edge(u, v, w)   辺を足す。辺番号（0 から順）を返す。w の既定は 1
 *   size()              頂点数
 *   edge_count()        add_edge を呼んだ回数
 *   g[u]                u から出る辺の一覧（const 参照）
 *   clear()             辺をすべて捨てる（頂点数はそのまま）
 *
 *   辺は { id, from, to, w }。
 *   有向グラフの「入ってくる辺」は持たない。要るときは向きを逆にした
 *   グラフをもう 1 つ作る。
 *
 *   無向グラフの自己ループは g[u] に 2 本入る（edge_count() は 1 のまま）。
 *   自己ループが次数に 2 を足すという慣習に合わせてある。g[u].size() を
 *   次数として使うときは意識すること。
 *
 * 使用例:
 *   INT(N, M);
 *   graph<ll> g(N);
 *   rep(M) {
 *     INT(u, v);
 *     LL(w);
 *     g.add_edge(--u, --v, w);
 *   }
 *   fore(e, g[0]) print(e.to, e.w);
 *
 *   graph<ll> h(N, true);   // 有向
 *   graph<int> t(N);        // 重みなし。w は 1 が入る
 *
 * verify:
 *   (未 verify)
 */
template <class W = long long>
struct graph {
  using weight_type = W;

  struct edge {
    int id;  // 何本目に足した辺か（無向なら往復で同じ）
    int from, to;
    W w;
  };

  explicit graph(int n_ = 0, bool directed_ = false)
      : n(n_), directed(directed_), adj(n_) {}

  int size() const { return n; }
  int edge_count() const { return m; }
  bool is_directed() const { return directed; }

  // 辺を足す。辺番号を返す
  int add_edge(int u, int v, W w = W(1)) {
    assert(0 <= u && u < n);
    assert(0 <= v && v < n);
    int id = m++;
    adj[u].push_back({id, u, v, w});
    if (!directed) adj[v].push_back({id, v, u, w});
    return id;
  }

  const std::vector<edge>& operator[](int u) const {
    assert(0 <= u && u < n);
    return adj[u];
  }

  // 辺をすべて捨てる（頂点数はそのまま）
  void clear() {
    for (auto& v : adj) v.clear();
    m = 0;
  }

 private:
  int n;
  int m = 0;
  bool directed;
  std::vector<std::vector<edge>> adj;
};
