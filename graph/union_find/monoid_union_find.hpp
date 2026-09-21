#include <algorithm>
#include <numeric>
#include <utility>
#include <vector>

namespace monoid_union_find_internal {
template <class S> S add(S a, S b) { return a + b; }
template <class S> S zero() { return S(); }
}  // namespace monoid_union_find_internal

/*
 * monoid_union_find<S, op, e> : 連結成分ごとに可換モノイドの総積を持つ
 * Union-Find。ならし O(a(n))
 *
 *   非推奨。同じことは union_find の merge コールバックでも書ける。
 *   値の更新を付け忘れられない利点があるので残してある
 *
 *   monoid_union_find(n)    すべて e() で初期化
 *   monoid_union_find(v)    頂点 i の初期値を v[i] とする
 *   merge(a, b)             併合する。実際に併合したら true
 *   merge(a, b, f)          値に加えて f(残る根, 消える根) も呼ぶ
 *   prod(x)                 x の属する成分の総積
 *   apply(x, val)           x の属する成分の値に val を合成する
 *   leader(x)               成分の代表頂点
 *   same(a, b) / size(x)
 *   group_count()           連結成分の個数
 *   group(x)                x と同じ成分の頂点   O(a(n) + |成分|)
 *   groups()                連結成分ごとの頂点   O(n)
 *   clear() / clear(v)      構築直後に戻す
 *
 *   op は可換かつ結合的であること。どちらが根になるかは union by size 任せで
 *   合成順を制御できないため。
 *   op / e の既定は足し算と S() なので、和を持つだけなら型引数は S だけでよい。
 *
 * 使用例:
 *   monoid_union_find<ll> uf(W);            // 成分ごとの重みの合計
 *   uf.merge(0, 1);
 *   print(uf.prod(0));
 *
 *   ll op_max(ll a, ll b) { return max(a, b); }
 *   ll e_min() { return -LINF; }
 *   monoid_union_find<ll, op_max, e_min> mx(W);
 */
template <class S, S (*op)(S, S) = monoid_union_find_internal::add<S>, S (*e)() = monoid_union_find_internal::zero<S>>
struct monoid_union_find {
 private:
  std::vector<int> dat;  // 負なら -(成分の大きさ)、非負なら親
  std::vector<int> nxt;  // 成分ごとの巡回リスト
  std::vector<S> val;    // 根に対してのみ意味を持つ
  int num;               // 連結成分の個数

 public:
  monoid_union_find() : monoid_union_find(0) {}
  explicit monoid_union_find(int n) : monoid_union_find(std::vector<S>(n, e())) {}
  explicit monoid_union_find(const std::vector<S>& v) : dat(v.size(), -1), nxt(v.size()), val(v), num((int)v.size()) {
    std::iota(nxt.begin(), nxt.end(), 0);
  }

  // 構築直後の状態に戻す。値も e() に戻る
  void clear() {
    int n = (int)dat.size();
    *this = monoid_union_find(n);
  }
  // 値を入れ直して構築直後の状態に戻す
  void clear(const std::vector<S>& v) { *this = monoid_union_find(v); }

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
    return merge(a, b, [](int, int) {});
  }
  // 値に加えて自分のデータも動かしたいとき、f(残る根, 消える根) を渡す
  template <class F> bool merge(int a, int b, F f) {
    int x = leader(a), y = leader(b);
    if (x == y) return false;
    S m = op(val[x], val[y]);
    if (-dat[x] < -dat[y]) std::swap(x, y);  // x を大きい方に
    std::swap(nxt[x], nxt[y]);               // 2 つの環の next を交換すると 1 つの環になる
    dat[x] += dat[y];
    dat[y] = x;
    val[x] = std::move(m);
    num--;
    f(x, y);
    return true;
  }

  // x の属する成分の総積
  const S& prod(int x) { return val[leader(x)]; }

  // x の属する成分の値に val_ を合成する
  void apply(int x, const S& val_) {
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

  // 連結成分ごとの頂点。各成分の中は昇順。成分そのものの並び順は決めていない
  std::vector<std::vector<int>> groups() {
    int n = (int)dat.size();
    std::vector<std::vector<int>> buf(n), ret;
    for (int i = 0; i < n; i++) buf[leader(i)].push_back(i);
    for (auto& g : buf)
      if (!g.empty()) ret.push_back(std::move(g));
    return ret;
  }
};
