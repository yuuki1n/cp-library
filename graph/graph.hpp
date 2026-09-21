#include <cassert>
#include <vector>

/*
 * graph<W> : グラフの入れ物（隣接リスト）
 *
 *   graph(n, directed)  頂点 0 .. n-1。directed の既定は false
 *   add_edge(u, v, w)   辺番号（0 から順）を返す。w の既定は 1
 *   g[u]                u から出る辺。辺は { id, from, to, w }
 *   size() / edge_count() / is_directed() / clear()
 *
 *   無向は往復ぶんを入れる（往復で辺番号は同じ）。自己ループは g[u] に 2 本
 *   入る（次数の慣習に合わせた）。
 *   有向の「入ってくる辺」は持たない。要るときは逆向きのグラフを別に作る。
 *
 * 使用例:
 *   graph<ll> g(N);
 *   rep(M) { INT(u, v); LL(w); g.add_edge(--u, --v, w); }
 *   fore(e, g[0]) print(e.to, e.w);
 */
template <class W = long long> struct graph {
  using weight_type = W;

  struct edge {
    int id;  // 何本目に足した辺か（無向なら往復で同じ）
    int from, to;
    W w;
  };

  explicit graph(int n_ = 0, bool directed_ = false) : n(n_), directed(directed_), adj(n_) {}

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
