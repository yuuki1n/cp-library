#include <algorithm>
#include <array>
#include <cassert>
#include <numeric>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

namespace relational_union_find_internal {
template <class F> F add(F a, F b) { return a + b; }
template <class F> F zero() { return F(); }
template <class F> F neg(F a) { return -a; }
}  // namespace relational_union_find_internal

/*
 * relational_union_find<F, op, e, inv, Undoable> : 重み付き（ポテンシャル付き）
 * Union-Find。ならし O(a(n))
 *
 *   「v の値 - u の値 は f」という制約を入れ、矛盾を検出しつつ差を答える。
 *
 *   relational_union_find(n)
 *   merge(u, v, f)      diff(u, v) == f を入れる。矛盾したら false
 *   merge(u, v, f, cb)  併合したとき cb(残る根, 消える根) を呼ぶ
 *   consistent(u, v, f) 入れずに矛盾しないかだけ調べる
 *   diff(u, v)          v の値 - u の値。same(u, v) が前提
 *   pot(x)              x の値 - 根の値
 *   group(x)            x と同じ成分の頂点   O(a(n) + |成分|)
 *   groups()            連結成分ごとの頂点   O(n)
 *   leader / same / size / group_count / clear
 *
 *   F は群であること（op は結合、inv は逆元、e は単位元）。既定は
 *   long long / 足し算 / 0 / 符号反転 なので、ふつうの差の制約なら
 *   relational_union_find<> uf(N); でよい。
 *
 *   Undoable = true にすると巻き戻せる。rollback_relational_union_find が
 *   その別名。undo / snapshot / rollback が増え、1 操作 O(log n) になる。
 *   経路圧縮できない代わりに union by size だけで高さを抑える。詳しくは
 *   union_find と同じ。
 *
 *   rel は「自分の値 - 親の値」を持つ。圧縮する場合は leader() のついでに
 *   「自分の値 - 根の値」へ畳み直すので、pot(x) は leader を呼ぶだけで済む。
 *   圧縮しない場合は根まで登って合成する。
 *
 * 使用例:
 *   relational_union_find<> uf(N);
 *   rep(M) {
 *     INT(u, v);
 *     LL(w);
 *     if (!uf.merge(--u, --v, w)) { print("No"); return; }
 *   }
 *   if (uf.same(0, 1)) print(uf.diff(0, 1));
 *
 *   // 二部グラフ判定。xor は自分自身が逆元なので inv は恒等
 *   int op_xor(int a, int b) { return a ^ b; }
 *   int e_zero() { return 0; }
 *   int inv_id(int a) { return a; }
 *   relational_union_find<int, op_xor, e_zero, inv_id> uf(N);
 *
 *   rollback_relational_union_find<> uf(N);   // 巻き戻せる版
 *   uf.merge(0, 1, 5);
 *   int t = uf.snapshot();
 *   uf.merge(1, 2, 3);
 *   uf.rollback(t);
 */
template <class F = long long, F (*op)(F, F) = relational_union_find_internal::add<F>, F (*e)() = relational_union_find_internal::zero<F>,
          F (*inv)(F) = relational_union_find_internal::neg<F>, bool Undoable = false>
struct relational_union_find {
 private:
  mutable std::vector<int> dat;  // 負なら -(成分の大きさ)、非負なら親。経路圧縮で書き換わる
  mutable std::vector<F> rel;    // 自分の値 - 親の値。根は必ず e()
  std::vector<int> nxt;          // 成分ごとの巡回リスト
  int num;
  // {子になった根, その根の元の dat, 親になった根}。併合しなかったときは {-1,0,0}
  [[no_unique_address]] std::conditional_t<Undoable, std::vector<std::array<int, 3>>, std::tuple<>> hst;

  // keyed_union_find だけが使う。頂点を 1 つ増やして、その番号を返す
  int extend() {
    int i = (int)dat.size();
    dat.push_back(-1), rel.push_back(e()), nxt.push_back(i), num++;
    return i;
  }
  template <class K, class UF, class Map> friend struct keyed_union_find;

 public:
  relational_union_find() : relational_union_find(0) {}
  explicit relational_union_find(int n) : dat(n, -1), rel(n, e()), nxt(n), num(n) { std::iota(nxt.begin(), nxt.end(), 0); }

  int leader(int x) const {
    if constexpr (Undoable) {
      while (dat[x] >= 0) x = dat[x];  // 巻き戻すので経路は畳まない
      return x;
    } else {
      if (dat[x] < 0) return x;
      int r = leader(dat[x]);
      // 圧縮で親が根に変わるので、自分の値 - 根の値 に付け替える
      rel[x] = op(rel[dat[x]], rel[x]);
      return dat[x] = r;
    }
  }
  bool same(int u, int v) const { return leader(u) == leader(v); }
  int size(int x) const { return -dat[leader(x)]; }
  int group_count() const { return num; }

  // x の値 - 根の値
  F pot(int x) const {
    if constexpr (Undoable) {
      F r = e();
      for (; dat[x] >= 0; x = dat[x]) r = op(rel[x], r);
      return r;
    } else {
      leader(x);  // 畳んでおけば rel[x] がそのまま「x の値 - 根の値」になる
      return rel[x];
    }
  }
  // v の値 - u の値。同じ成分にいることが前提
  F diff(int u, int v) const {
    assert(same(u, v));
    return op(inv(pot(u)), pot(v));
  }
  // 制約を入れずに、矛盾しないかだけ調べる
  bool consistent(int u, int v, F f) const { return !same(u, v) || diff(u, v) == f; }

  // diff(u, v) == f という制約を入れる。既存の制約と矛盾したら false
  bool merge(int u, int v, F f) {
    return merge(u, v, f, [](int, int) {});
  }
  // 併合したとき cb(残る根, 消える根) を呼ぶ。成分ごとの値を自分で持ちたいときに使う
  template <class Cb> bool merge(int u, int v, F f, Cb cb) {
    int x = leader(u), y = leader(v);
    if (x == y) {
      if constexpr (Undoable) hst.push_back({-1, 0, 0});  // undo の回数を merge と合わせる
      return diff(u, v) == f;
    }
    // 根 y の値 - 根 x の値 = (u - x) + (v - u) + (y - v)
    F g = op(op(pot(u), f), inv(pot(v)));
    if (-dat[x] < -dat[y]) std::swap(x, y), g = inv(g);  // 小さい方をぶら下げる。向きが逆になる
    if constexpr (Undoable) hst.push_back({y, dat[y], x});
    std::swap(nxt[x], nxt[y]);  // 2 つの環の next を交換すると 1 つの環になる
    dat[x] += dat[y];
    dat[y] = x;
    rel[y] = g;
    num--;
    cb(x, y);
    return true;
  }

  // 直前の merge を 1 回取り消す。履歴が空なら何もしない
  void undo() { undo([](int, int) {}); }
  // 取り消す直前に cb(残る根, 消える根) を呼ぶ。merge に渡したものを打ち消す関数を渡す
  template <class Cb> void undo(Cb cb) {
    if (hst.empty()) return;
    auto [y, dy, x] = hst.back();
    hst.pop_back();
    if (y < 0) return;
    cb(x, y);
    dat[y] = dy, rel[y] = e();
    dat[x] -= dy;
    std::swap(nxt[x], nxt[y]);  // 交換の取り消しはもう一度交換するだけ
    num++;
  }

  // 今までに呼んだ merge の回数
  int snapshot() const { return (int)hst.size(); }
  // merge を t 回呼んだ時点まで戻す
  void rollback(int t) {
    rollback(t, [](int, int) {});
  }
  template <class Cb> void rollback(int t, Cb cb) {
    while (!hst.empty() && (int)hst.size() > t) undo(cb);
  }

  // 構築直後の状態に戻す（大きさはそのまま）。履歴も捨てる
  void clear() {
    std::fill(dat.begin(), dat.end(), -1);
    std::fill(rel.begin(), rel.end(), e());
    std::iota(nxt.begin(), nxt.end(), 0);
    num = (int)dat.size();
    if constexpr (Undoable) hst.clear();
  }

  // x と同じ成分の頂点を返す（x 自身も含む）
  std::vector<int> group(int x) const {
    int r = leader(x), n = -dat[r], c = r;
    std::vector<int> ret;
    ret.reserve(n);
    for (int i = 0; i < n; i++) ret.push_back(c = nxt[c]);
    return ret;
  }

  // 連結成分ごとの頂点。各成分の中は昇順。成分そのものの並び順は決めていない
  std::vector<std::vector<int>> groups() const {
    int n = (int)dat.size();
    std::vector<std::vector<int>> buf(n), ret;
    for (int i = 0; i < n; i++) buf[leader(i)].push_back(i);
    for (auto& g : buf)
      if (!g.empty()) ret.push_back(std::move(g));
    return ret;
  }
};

// 巻き戻せる版。経路圧縮しない代わりに merge を取り消せる
template <class F = long long, F (*op)(F, F) = relational_union_find_internal::add<F>, F (*e)() = relational_union_find_internal::zero<F>,
          F (*inv)(F) = relational_union_find_internal::neg<F>>
using rollback_relational_union_find = relational_union_find<F, op, e, inv, true>;
