#include <atcoder/dsu>
#include <functional>
#include <utility>
#include <vector>

/*
 * monoid_dsu : 連結成分ごとに可換モノイドの総積を持つ dsu
 *
 *   atcoder::dsu を使って、成分ごとの畳み込みを追加したもの。併合のたびに
 *   2 つの成分の値を合成するので、prod(x) は O(1)（leader の分だけ α(n)）。
 *   op は可換かつ結合的であること。どちらの成分が根になるかは
 *   union by size 任せで、合成順を制御できないため。
 *
 *   monoid_dsu(v)        頂点 i の初期値を v[i] とする
 *   monoid_dsu(n, e)     頂点 0 .. n-1 の初期値をすべて e とする
 *   merge(a, b)         併合する。実際に併合したら true
 *   prod(x)             x の属する成分の総積
 *   apply(x, val)       x の属する成分の値に val を合成する
 *
 *   leader / same / size / groups は atcoder::dsu のものがそのまま使える。
 *   継承は private。公開継承だと atcoder::dsu& 経由で基底の merge を呼べて
 *   しまい、値を更新しないまま併合されるため。
 *
 *   clear(v) / clear(n, e)  構築直後の状態に戻す（値は入れ直す）
 *
 * 使用例:
 *   // 成分ごとの頂点の重みの合計
 *   monoid_dsu<long long> uf(W);            // W は vector<long long>
 *   uf.merge(0, 1);
 *   print(uf.prod(0));
 *
 *   // 成分ごとの辺の本数（自己ループ・多重辺も数える）
 *   monoid_dsu<long long> es(N, 0);
 *   fore(e, edges) { es.merge(e.fi, e.se); es.apply(e.fi, 1); }
 *   // 辺の本数 == 頂点数 なら、その成分はちょうど 1 つ閉路を持つ
 *
 *   // 成分ごとの最大値
 *   monoid_dsu<long long, decltype([](auto a, auto b) { return max(a, b); })> mx(A);
 *
 * verify:
 *   (未 verify)
 *   予定: https://judge.yosupo.jp/problem/dynamic_graph_vertex_add_component_sum
 *         （rollback_dsu と組み合わせて Offline Dynamic Connectivity で）
 */
template <class V, class Op = std::plus<V>>
struct monoid_dsu : private atcoder::dsu {
 private:
  std::vector<V> val;  // 根に対してのみ意味を持つ
  Op op;

 public:
  // 基底のうち、そのまま使えるものだけを公開する。
  // 公開継承にすると atcoder::dsu& 経由で基底の merge を呼べてしまい、
  // val が更新されないまま併合されてしまうため。
  using atcoder::dsu::groups;
  using atcoder::dsu::leader;
  using atcoder::dsu::same;
  using atcoder::dsu::size;

  monoid_dsu() : monoid_dsu(0) {}
  explicit monoid_dsu(const std::vector<V>& v, Op op_ = Op())
      : atcoder::dsu((int)v.size()), val(v), op(op_) {}
  explicit monoid_dsu(int n, V e = V(), Op op_ = Op())
      : atcoder::dsu(n), val(n, e), op(op_) {}

  // 構築直後の状態に戻す。値は復元できないので入れ直す
  void clear(const std::vector<V>& v) { *this = monoid_dsu(v, op); }
  void clear(int n, V e = V()) { *this = monoid_dsu(n, e, op); }

  // 併合する。もともと別の成分だったら true
  bool merge(int a, int b) {
    int x = leader(a), y = leader(b);
    if (x == y) return false;
    V m = op(val[x], val[y]);
    atcoder::dsu::merge(x, y);
    val[leader(x)] = std::move(m);
    return true;
  }

  // x の属する成分の総積
  const V& prod(int x) { return val[leader(x)]; }

  // x の属する成分の値に val_ を合成する
  void apply(int x, const V& val_) {
    int r = leader(x);
    val[r] = op(val[r], val_);
  }
};
