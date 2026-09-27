#include <cassert>
#include <utility>
#include <vector>

/*
 * hld : HL 分解（Heavy-Light Decomposition）。構築 O(n)
 *
 *   hld(n)              頂点 0 .. n-1
 *   add_edge(u, v)      辺を足す
 *   build(root)         根を決めて構築する。既定の根は 0
 *   clear()             構築直後に戻す（頂点数はそのまま、辺は捨てる）
 *
 *   lca(u, v)           最小共通祖先                       O(log n)
 *   dist(u, v)          辺の本数での距離                   O(log n)
 *   la(v, k)            v から k 個上の祖先。無ければ -1   O(log n)
 *   jump(u, v, k)       u から v へ k 進んだ頂点。無ければ -1
 *   path(u, v, edge)    パスを区間の列に分ける。u から v の順 O(log n) 個
 *   in(v) / out(v)      v の位置と部分木の終わり（半開 [in, out)）
 *   at(i)               位置 i にある頂点。in の逆
 *   size()              頂点の個数。size(v) は v の部分木の頂点数
 *   depth(v)            v の深さ（根は 0）
 *
 *   区間はすべて半開 [l, r)。セグメント木にそのまま渡せる。
 *   path(u, v, false) は頂点に値を持つとき、true は辺に値を持つときに使う。
 *   辺の値は子側の頂点に置く。位置は in(深いほうの端点) で、辺と「根でない
 *   頂点」が 1 対 1 に対応する。true だと LCA を除くのでちょうど辺だけになる。
 *
 *   path が返す区間は { l, r, fwd } で、u から v へ進む順に並ぶ。
 *   fwd が false の区間は右から左に読む（登る向きなので順序が逆になる）。
 *   op が可換なら fwd は無視してよい。非可換なら、引数を入れ替えた op で
 *   もう 1 本セグメント木を作り、fwd が false のときそちらを引く。
 *   木であること（連結で辺が n-1 本）を前提とする。build を呼ぶまで他は使えない。
 *   build のあとに辺は足せない。根を変えるだけなら build(別の根) を呼び直す。
 *
 * 使用例:
 *   hld t(N);
 *   rep(N - 1) { INT(u, v); t.add_edge(--u, --v); }
 *   t.build();
 *   print(t.lca(0, 1), t.dist(0, 1));
 *
 *   // 頂点に値を持たせて、パスの総和を取る
 *   segtree<ll, op_add, e_zero> seg(N);
 *   rep(v, N) seg.set(t.in(v), A[v]);
 *   ll s = 0;
 *   fore(p, t.path(u, v)) s += seg.prod(p.l, p.r);   // 和なので fwd は見なくてよい
 *
 *   // 部分木の総和。部分木は [in(v), out(v)) の 1 区間に収まる
 *   print(seg.prod(t.in(v), t.out(v)));
 */
struct hld {
 private:
  int n;
  std::vector<std::vector<int>> adj;
  std::vector<int> par_, dep_, sz_, hd_;   // 親 / 深さ / 部分木の大きさ / 連の先頭
  std::vector<int> in_, ord_;              // 頂点 -> 位置 / 位置 -> 頂点
  bool built = false;

 public:
  hld() : hld(0) {}
  explicit hld(int n_) : n(n_), adj(n_) {}

  void add_edge(int u, int v) {
    assert(!built && "build の後に辺は足せない。作り直すなら clear()");
    adj[u].push_back(v), adj[v].push_back(u);
  }

  // 構築直後の状態に戻す（頂点数はそのまま、辺は捨てる）
  void clear() {
    for (auto& a : adj) a.clear();
    built = false;
  }

  // 根を決めて構築する
  void build(int root = 0) {
    if (n == 0) return;
    par_.assign(n, -1), dep_.assign(n, 0), sz_.assign(n, 1);
    hd_.assign(n, root), in_.assign(n, 0), ord_.assign(n, 0);

    std::vector<int> stk{root};
    std::vector<int> pre;  // 行きがけ順。逆に見ると子が先に来る
    pre.reserve(n);
    par_[root] = -1;
    while (!stk.empty()) {
      int v = stk.back();
      stk.pop_back();
      pre.push_back(v);
      for (int w : adj[v])
        if (w != par_[v]) par_[w] = v, dep_[w] = dep_[v] + 1, stk.push_back(w);
    }
    assert((int)pre.size() == n && "木でない（連結でないか辺が足りない）");

    // 子が先に来る順で部分木の大きさを出し、重い子を adj の先頭へ持ってくる
    for (int i = n - 1; i >= 0; i--) {
      int v = pre[i], heavy = -1, best = 0;
      for (int w : adj[v])
        if (w != par_[v]) {
          sz_[v] += sz_[w];
          if (sz_[w] > best) best = sz_[w], heavy = w;
        }
      if (heavy >= 0)
        for (auto& w : adj[v])
          if (w == heavy) {
            std::swap(w, adj[v][0]);
            break;
          }
    }

    // 重い子を最後に積むと次に処理されるので、連が連続した位置に並ぶ
    int t = 0;
    stk.assign(1, root);
    while (!stk.empty()) {
      int v = stk.back();
      stk.pop_back();
      in_[v] = t, ord_[t++] = v;
      for (int i = (int)adj[v].size() - 1; i >= 1; i--) {
        int w = adj[v][i];
        if (w == par_[v]) continue;
        hd_[w] = w;
        stk.push_back(w);
      }
      if (!adj[v].empty() && adj[v][0] != par_[v]) {
        hd_[adj[v][0]] = hd_[v];
        stk.push_back(adj[v][0]);
      }
    }
    built = true;
  }

  int size() const { return n; }
  int in(int v) const { return assert(built), in_[v]; }
  int out(int v) const { return assert(built), in_[v] + sz_[v]; }
  int at(int i) const { return assert(built), ord_[i]; }
  int depth(int v) const { return assert(built), dep_[v]; }
  int size(int v) const { return assert(built), sz_[v]; }

  int lca(int u, int v) const {
    assert(built);
    while (hd_[u] != hd_[v]) {
      if (in_[hd_[u]] > in_[hd_[v]]) std::swap(u, v);
      v = par_[hd_[v]];
    }
    return in_[u] < in_[v] ? u : v;
  }
  int dist(int u, int v) const {
    // dep_ を先に読まれても落ちるよう、lca 任せにせず自分でも確かめる
    assert(built);
    return dep_[u] + dep_[v] - 2 * dep_[lca(u, v)];
  }

  // v から k 個上の祖先。無ければ -1
  int la(int v, int k) const {
    assert(built);
    if (k < 0 || dep_[v] < k) return -1;
    while (true) {
      int h = hd_[v];
      if (in_[v] - in_[h] >= k) return ord_[in_[v] - k];
      k -= in_[v] - in_[h] + 1;
      v = par_[h];
    }
  }

  // u から v へ k 進んだ頂点。届かなければ -1
  int jump(int u, int v, int k) const {
    int l = lca(u, v), du = dep_[u] - dep_[l], dv = dep_[v] - dep_[l];
    if (k < 0 || k > du + dv) return -1;
    return k <= du ? la(u, k) : la(v, du + dv - k);
  }

  // パスの一部を覆う半開区間。fwd が false なら右から左に読む
  struct part {
    int l, r;
    bool fwd;
  };

  // u-v パスを覆う区間の列。u から v への順に並ぶ。
  // edge = true なら LCA を除く（辺に値を持つとき）
  std::vector<part> path(int u, int v, bool edge = false) const {
    assert(built);
    std::vector<part> lft, rgt;
    // 登る側を分けて持つ。u 側は登る向き（葉→根）なので逆読みになる
    while (hd_[u] != hd_[v]) {
      if (in_[hd_[u]] > in_[hd_[v]]) lft.push_back({in_[hd_[u]], in_[u] + 1, false}), u = par_[hd_[u]];
      else rgt.push_back({in_[hd_[v]], in_[v] + 1, true}), v = par_[hd_[v]];
    }
    int e = edge ? 1 : 0;
    if (in_[u] > in_[v]) {
      if (in_[v] + e < in_[u] + 1) lft.push_back({in_[v] + e, in_[u] + 1, false});
    } else if (in_[u] + e < in_[v] + 1) {
      rgt.push_back({in_[u] + e, in_[v] + 1, true});
    }
    lft.insert(lft.end(), rgt.rbegin(), rgt.rend());
    return lft;
  }
};
