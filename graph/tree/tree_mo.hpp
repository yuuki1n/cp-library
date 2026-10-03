#include <cassert>
#include <type_traits>
#include <utility>
#include <vector>

/*
 * tree_mo : 木のパスに対する Mo's algorithm。オイラーツアーで 1 本の列に落とす
 *
 *   tree_mo(n)            頂点 0 .. n-1
 *   add_edge(u, v)        辺を足す
 *   build(root)           根を決めて構築する。既定の根は 0
 *   add_query(s, t)       頂点 s から t へのパスを問う
 *   solve(add, rem, get)  答えを add_query した順で返す
 *
 *   add(v) / rem(v) は頂点 v をパスに入れる / 外す。get() は今の答えを返す。
 *   get(i) と書くと、何番目のクエリを答えているかを受け取れる。
 *   クエリごとに追加の条件（しきい値など）があるときはこれで引く。
 *   状態は呼び出し側で持つ（ラムダで捕まえる）。
 *
 *   オイラーツアーでは各頂点が 2 回現れ、窓の中に奇数回あるものがパス上にいる。
 *   そのため add と rem は「入れたか外したか」ではなく「奇数回になったか
 *   偶数回になったか」で呼ばれる。窓を広げたときに rem が、縮めたときに
 *   add が呼ばれることがあるので、この 2 つは対称に書くこと。
 *   LCA はライブラリ側で足し引きするので、呼び出し側は気にしなくてよい。
 *
 *   計算量は O((2n + q) sqrt(q))。列の長さが 2n になるぶん 1 次元より重い。
 *   add / rem は O(1)、get は O(sqrt(n)) までに収める。
 *
 *   hld.hpp の hld と mo.hpp の mo を使うが include はしない。
 *   貼るときはこの 2 つを先に置くこと。
 *   木（連結で辺が n-1 本）であること。森には使えない。
 *
 * 使用例:
 *   // パス上に現れる値の種類数
 *   tree_mo t(N);
 *   rep(N - 1) { INT(u, v); t.add_edge(--u, --v); }
 *   t.build();
 *   rep(Q) { INT(s, e); t.add_query(--s, --e); }
 *
 *   vll cnt(200001);
 *   ll kind = 0;
 *   auto add = [&](int v) { if (cnt[X[v]]++ == 0) kind++; };
 *   auto rem = [&](int v) { if (--cnt[X[v]] == 0) kind--; };
 *   fore(x, t.solve(add, rem, [&] { return kind; })) print(x);
 */
struct tree_mo {
 private:
  int n;
  hld tree;
  std::vector<int> tour, tin, tout;  // 長さ 2n の列と、各頂点が出入りする位置
  std::vector<int> qs, qt;           // クエリの両端
  bool built = false;

  // get は get() でも get(i) でも書ける。戻り値は参照でも値で受け取る
  template <class Get> static auto call_get(Get& g, int i) {
    if constexpr (std::is_invocable_v<Get, int>) return g(i);
    else return g();
  }

 public:
  tree_mo() : tree_mo(0) {}
  explicit tree_mo(int n_) : n(n_), tree(n_) {}

  void add_edge(int u, int v) {
    assert(!built && "build の後に辺は足せない");
    tree.add_edge(u, v);
  }

  // 根を決めて構築する
  void build(int root = 0) {
    built = true;
    if (n == 0) return;
    tree.build(root);
    tour.clear(), tour.reserve(2 * n);
    tin.assign(n, -1), tout.assign(n, -1);
    // 行きがけ順に並んでいるので、部分木が終わった祖先から閉じていけばよい
    std::vector<int> stk;
    for (int i = 0; i < n; i++) {
      while (!stk.empty() && tree.out(stk.back()) <= i) {
        tout[stk.back()] = (int)tour.size(), tour.push_back(stk.back()), stk.pop_back();
      }
      int v = tree.at(i);
      tin[v] = (int)tour.size(), tour.push_back(v), stk.push_back(v);
    }
    while (!stk.empty()) {
      tout[stk.back()] = (int)tour.size(), tour.push_back(stk.back()), stk.pop_back();
    }
  }

  // 頂点 s から t へのパスを問う
  void add_query(int s, int t) {
    assert(built && "build してからクエリを足す");
    qs.push_back(s), qt.push_back(t);
  }

  int size() const { return n; }

  // add(v) / rem(v) でパスを動かし、各クエリで get() を呼ぶ
  template <class Add, class Rem, class Get> auto solve(Add add, Rem rem, Get get) const {
    assert(built && "build してから solve する");
    int q = (int)qs.size();
    std::vector<decltype(call_get(get, 0))> ans(q);
    if (q == 0) return ans;

    // s を先に出る方とする。LCA が s なら [tin[s], tin[t]]、
    // そうでなければ [tout[s], tin[t]] が、奇数回 = パス上（ただし LCA が漏れる）
    mo m(2 * n);
    std::vector<int> extra(q, -1);  // 区間から漏れる LCA。無ければ -1
    for (int i = 0; i < q; i++) {
      int s = qs[i], t = qt[i];
      if (tin[s] > tin[t]) std::swap(s, t);
      int l = tree.lca(s, t);
      if (l == s) m.add_query(tin[s], tin[t] + 1);
      else m.add_query(tout[s], tin[t] + 1), extra[i] = l;
    }

    std::vector<char> odd(n, 0);
    // 窓に入れても外しても、やることは「その頂点の偶奇を反転する」だけ
    auto toggle = [&](int i) {
      int v = tour[i];
      if (odd[v] ^= 1) add(v);
      else rem(v);
    };
    // 漏れた LCA は答えを取り出す直前に足して、取り出したら戻す
    return m.solve(toggle, toggle, [&](int i) {
      int e = extra[i];
      if (e < 0) return call_get(get, i);
      add(e);
      auto r = call_get(get, i);
      rem(e);
      return r;
    });
  }
};
