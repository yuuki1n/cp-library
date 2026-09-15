#include <algorithm>
#include <cassert>
#include <functional>
#include <utility>
#include <vector>

/*
 * relational_dsu<F, Op, Inv> : 重み付き（ポテンシャル付き）dsu。ならし O(a(n))
 *
 *   「v の値 - u の値 は f」という制約を入れ、矛盾を検出しつつ差を答える。
 *
 *   relational_dsu(n, e, op, inv)
 *   merge(u, v, f)      diff(u, v) == f を入れる。矛盾したら false
 *   consistent(u, v, f) 入れずに矛盾しないかだけ調べる
 *   diff(u, v)          v の値 - u の値。same(u, v) が前提
 *   leader / same / size / group_count / clear
 *
 *   F は群であること（op は結合、inv は逆元、e は単位元）。既定は
 *   long long / 足し算 / 符号反転 なので、ふつうの差の制約なら
 *   relational_dsu<> uf(N); でよい。
 *   atcoder::dsu は継承していない。経路圧縮のたびに関係を合成し直す必要が
 *   あるが、dsu の leader は非仮想で parent_or_size も private のため。
 *
 * 使用例:
 *   relational_dsu<> uf(N);
 *   rep(M) { INT0(u, v); LL(w); if (!uf.merge(u, v, w)) { print("No"); return; } }
 *   if (uf.same(0, 1)) print(uf.diff(0, 1));
 *
 *   // 二部グラフ判定。xor は自分自身が逆元なので inv は恒等
 *   relational_dsu<int, bit_xor<int>, identity> uf(N);
 *
 * verify:
 *   (未 verify)
 *   予定: https://judge.yosupo.jp/problem/unionfind_with_potential
 *         https://judge.yosupo.jp/problem/unionfind_with_potential_non_commutative_group
 *         後者は非可換な群での検証。今のテストは可換な演算しか使っていない
 */
template <class F = long long, class Op = std::plus<F>,
          class Inv = std::negate<F>>
struct relational_dsu {
 private:
  std::vector<int> dat;  // 負なら -(成分の大きさ)、非負なら親
  std::vector<F> rel;    // 根から自分への関係。根は必ず e
  int num;
  F e;  // 単位元。clear で rel を戻すために持っておく
  Op op;
  Inv inv;

 public:
  relational_dsu() : relational_dsu(0) {}
  explicit relational_dsu(int n, F e_ = F(), Op op_ = Op(), Inv inv_ = Inv())
      : dat(n, -1), rel(n, e_), num(n), e(e_), op(op_), inv(inv_) {}

  // 構築直後の状態に戻す（大きさはそのまま）
  void clear() {
    std::fill(dat.begin(), dat.end(), -1);
    std::fill(rel.begin(), rel.end(), e);
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
};
