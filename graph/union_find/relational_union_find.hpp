#include <algorithm>
#include <cassert>
#include <numeric>
#include <utility>
#include <vector>

namespace relational_union_find_internal {
template <class F> F add(F a, F b) { return a + b; }
template <class F> F zero() { return F(); }
template <class F> F neg(F a) { return -a; }
}  // namespace relational_union_find_internal

/*
 * relational_union_find<F, op, e, inv> : 重み付き（ポテンシャル付き）
 * Union-Find。ならし O(a(n))
 *
 *   「v の値 - u の値 は f」という制約を入れ、矛盾を検出しつつ差を答える。
 *
 *   relational_union_find(n)
 *   merge(u, v, f)      diff(u, v) == f を入れる。矛盾したら false
 *   consistent(u, v, f) 入れずに矛盾しないかだけ調べる
 *   diff(u, v)          v の値 - u の値。same(u, v) が前提
 *   group(x)            x と同じ成分の頂点   O(a(n) + |成分|)
 *   groups()            連結成分ごとの頂点   O(n)
 *   leader / same / size / group_count / clear
 *
 *   F は群であること（op は結合、inv は逆元、e は単位元）。既定は
 *   long long / 足し算 / 0 / 符号反転 なので、ふつうの差の制約なら
 *   relational_union_find<> uf(N); でよい。
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
 * verify:
 *   (未 verify)
 *   予定: https://judge.yosupo.jp/problem/unionfind_with_potential
 *         https://judge.yosupo.jp/problem/unionfind_with_potential_non_commutative_group
 *         後者は非可換な群での検証。今のテストは可換な演算しか使っていない
 */
template <class F = long long, F (*op)(F, F) = relational_union_find_internal::add<F>, F (*e)() = relational_union_find_internal::zero<F>,
          F (*inv)(F) = relational_union_find_internal::neg<F>>
struct relational_union_find {
 private:
  std::vector<int> dat;  // 負なら -(成分の大きさ)、非負なら親
  std::vector<F> rel;    // 根から自分への関係。根は必ず e()
  std::vector<int> nxt;  // 成分ごとの巡回リスト
  int num;

 public:
  relational_union_find() : relational_union_find(0) {}
  explicit relational_union_find(int n) : dat(n, -1), rel(n, e()), nxt(n), num(n) { std::iota(nxt.begin(), nxt.end(), 0); }

  // 構築直後の状態に戻す（大きさはそのまま）
  void clear() {
    std::fill(dat.begin(), dat.end(), -1);
    std::fill(rel.begin(), rel.end(), e());
    std::iota(nxt.begin(), nxt.end(), 0);
    num = (int)dat.size();
  }

  int leader(int x) {
    if (dat[x] < 0) return x;
    int r = leader(dat[x]);
    // 圧縮で親が根に変わるので、根から自分への関係に付け替える
    rel[x] = op(rel[dat[x]], rel[x]);
    return dat[x] = r;
  }

  bool same(int u, int v) { return leader(u) == leader(v); }
  int size(int x) { return -dat[leader(x)]; }
  int group_count() const { return num; }

  // diff(u, v) == f という制約を入れる。既存の制約と矛盾したら false
  bool merge(int u, int v, F f) {
    int x = leader(u), y = leader(v);
    if (x == y) return diff(u, v) == f;
    // 根 x から根 y への関係 = (x→u) + (u→v) + (v→y)
    F g = op(op(rel[u], f), inv(rel[v]));
    if (-dat[x] < -dat[y]) {  // 小さい方をぶら下げる。向きが逆になる
      std::swap(x, y);
      g = inv(g);
    }
    std::swap(nxt[x], nxt[y]);  // 2 つの環の next を交換すると 1 つの環になる
    dat[x] += dat[y];
    dat[y] = x;
    rel[y] = g;
    num--;
    return true;
  }

  // 制約を入れずに、矛盾しないかだけ調べる
  bool consistent(int u, int v, F f) { return !same(u, v) || diff(u, v) == f; }

  // v の値 - u の値。同じ成分にいることが前提
  F diff(int u, int v) {
    assert(same(u, v));
    return op(inv(rel[u]), rel[v]);
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
