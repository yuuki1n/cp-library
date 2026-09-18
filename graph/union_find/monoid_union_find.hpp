#include <algorithm>
#include <functional>
#include <numeric>
#include <utility>
#include <vector>

/*
 * monoid_union_find<V, Op> : 連結成分ごとに可換モノイドの総積を持つ
 * Union-Find。ならし O(a(n))
 *
 *   monoid_union_find(v)           頂点 i の初期値を v[i] とする
 *   monoid_union_find(n, e)        すべて e で初期化
 *   merge(a, b)             併合する。実際に併合したら true
 *   prod(x)                 x の属する成分の総積
 *   apply(x, val)           x の属する成分の値に val を合成する
 *   leader(x)               成分の代表頂点
 *   same(a, b) / size(x)
 *   group_count()           連結成分の個数
 *   group(x)                x と同じ成分の頂点   O(a(n) + |成分|)
 *   groups()                連結成分ごとの頂点   O(n)
 *   clear(v) / clear(n, e)  構築直後に戻す（値は入れ直す）
 *
 *   op は可換かつ結合的であること。どちらが根になるかは union by size 任せで
 *   合成順を制御できないため。
 *
 * 使用例:
 *   monoid_union_find<ll> uf(W);                   // 成分ごとの重みの合計
 *   uf.merge(0, 1);
 *   print(uf.prod(0));
 *
 *   monoid_union_find<ll> es(N, 0);                // 成分ごとの辺の本数
 *   fore(e, edges) { es.merge(e.fi, e.se); es.apply(e.fi, 1); }
 *
 * verify:
 *   (未 verify)
 *   予定: https://judge.yosupo.jp/problem/dynamic_graph_vertex_add_component_sum
 */
template <class V, class Op = std::plus<V>> struct monoid_union_find {
 private:
  std::vector<int> dat;  // 負なら -(成分の大きさ)、非負なら親
  std::vector<int> nxt;  // 成分ごとの巡回リスト
  std::vector<V> val;    // 根に対してのみ意味を持つ
  int num;               // 連結成分の個数
  Op op;

 public:
  monoid_union_find() : monoid_union_find(0) {}
  explicit monoid_union_find(const std::vector<V>& v, Op op_ = Op())
      : dat(v.size(), -1), nxt(v.size()), val(v), num((int)v.size()), op(op_) {
    std::iota(nxt.begin(), nxt.end(), 0);
  }
  explicit monoid_union_find(int n, V e = V(), Op op_ = Op())
      : dat(n, -1), nxt(n), val(n, e), num(n), op(op_) {
    std::iota(nxt.begin(), nxt.end(), 0);
  }

  // 構築直後の状態に戻す。値は復元できないので入れ直す
  void clear(const std::vector<V>& v) { *this = monoid_union_find(v, op); }
  void clear(int n, V e = V()) { *this = monoid_union_find(n, e, op); }

  int leader(int x) {
    while (dat[x] >= 0) {
      if (dat[dat[x]] >= 0) dat[x] = dat[dat[x]];  // 経路を半分に畳む
      x = dat[x];
    }
    return x;
  }
  bool same(int a, int b) { return leader(a) == leader(b); }
  int size(int x) { return -dat[leader(x)]; }
  int group_count() const { return num; }

  // 併合する。もともと別の成分だったら true
  bool merge(int a, int b) {
    int x = leader(a), y = leader(b);
    if (x == y) return false;
    V m = op(val[x], val[y]);
    if (-dat[x] < -dat[y]) std::swap(x, y);  // x を大きい方に
    std::swap(nxt[x], nxt[y]);  // 2 つの環の next を交換すると 1 つの環になる
    dat[x] += dat[y];
    dat[y] = x;
    val[x] = std::move(m);
    num--;
    return true;
  }

  // x の属する成分の総積
  const V& prod(int x) { return val[leader(x)]; }

  // x の属する成分の値に val_ を合成する
  void apply(int x, const V& val_) {
    int r = leader(x);
    val[r] = op(val[r], val_);
  }

  // x と同じ成分の頂点を返す（x 自身も含む）
  std::vector<int> group(int x) {
    int r = leader(x), n = -dat[r], c = r;
    std::vector<int> ret;
    ret.reserve(n);
    for (int i = 0; i < n; i++) ret.push_back(c = nxt[c]);
    return ret;
  }

  // 連結成分ごとの頂点。各成分の中は昇順
  std::vector<std::vector<int>> groups() {
    int n = (int)dat.size();
    std::vector<std::vector<int>> buf(n), ret;
    for (int i = 0; i < n; i++) buf[leader(i)].push_back(i);
    for (auto& g : buf)
      if (!g.empty()) ret.push_back(std::move(g));
    return ret;
  }
};
