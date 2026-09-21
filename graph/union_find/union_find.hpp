#include <algorithm>
#include <array>
#include <numeric>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

namespace union_find_internal {
// 値を持たないときの印。既定の op を実体化するためだけに演算を持たせてある
struct none {
  friend none operator+(none, none) { return {}; }
};
template <class S> S add(S a, S b) { return a + b; }
template <class S> S zero() { return S(); }
}  // namespace union_find_internal

/*
 * union_find<S, op, e, Undoable> : 1 つの成分を O(|成分|) で取り出せる Union-Find
 *
 *   merge(a, b)     併合する。実際に併合したら true
 *   merge(a, b, f)  併合したとき f(残る根, 消える根) を呼ぶ
 *   leader(x)       成分の代表頂点
 *   same(a, b) / size(x)
 *   group_count()   連結成分の個数
 *   group(x)        x と同じ成分の頂点      O(a(n) + |成分|)
 *   groups()        連結成分ごとの頂点      O(n)
 *   clear()         構築直後に戻す（大きさはそのまま）
 *
 *   S を渡すと連結成分ごとに可換モノイドの総積を持つ。union_find<ll> なら和。
 *   union_find(v)   頂点 i の初期値を v[i] とする
 *   prod(x)         x の属する成分の総積
 *   apply(x, val)   x の属する成分の値に val を合成する
 *   op は可換かつ結合的であること。どちらが根になるかは union by size 任せで
 *   合成順を制御できないため。
 *
 *   Undoable = true にすると巻き戻せる。rollback_union_find がその別名。
 *   merge / leader などが O(log n) になる。
 *   undo()        直前の merge を 1 回取り消す              O(1)
 *   undo(f)       取り消す直前に f(残る根, 消える根) を呼ぶ
 *   snapshot()    今までに呼んだ merge の回数
 *   rollback(t)   merge を t 回呼んだ時点まで戻す
 *   rollback(t, f)
 *
 *   成分ごとに巡回リスト nxt を持つ。merge では 2 つの環の next を交換するだけで
 *   1 つの環になる。全成分が要るなら groups()、1 成分だけなら group()。
 *   groups() は成分の大きさによらず O(n) かかる。
 *   group() の中は環をたどった順、groups() の中は昇順。どちらも成分そのものの
 *   並び順は決めていない。
 *
 *   巻き戻す場合は経路圧縮できないので、代わりに union by size だけで高さを
 *   O(log n) に抑える。速度はおよそ 2 倍かかり、履歴に merge 1 回あたり
 *   12 + sizeof(S) バイト使う。巻き戻さないなら既定のまま（履歴の分は消える）。
 *   併合しなかった merge も 1 回と数えるので merge と undo は 1 対 1 で対応する。
 *   履歴が空の undo は何もしない。
 *   値を載せた場合、op に逆元が無くても巻き戻せる（残る根の元の値を控えるため）。
 *
 * 使用例:
 *   union_find uf(N);
 *   rep(M) { INT(u, v); uf.merge(--u, --v); }
 *   print(uf.group(0));
 *
 *   union_find<ll> uf(w);                    // 成分ごとの重みの合計
 *   uf.merge(0, 1);
 *   print(uf.prod(0));
 *
 *   ll op_max(ll a, ll b) { return max(a, b); }
 *   ll e_min() { return -LINF; }
 *   rollback_union_find<ll, op_max, e_min> uf(a);   // 巻き戻せる成分ごとの最大値
 *   int t = uf.snapshot();
 *   uf.merge(0, 1);
 *   uf.rollback(t);
 *
 *   // 値を 1 つ載せるだけでは足りないなら merge / undo にコールバックを渡す
 *   vector<ll> sum(N);
 *   uf.merge(a, b, [&](int to, int from) { sum[to] += sum[from]; });
 *   uf.undo(      [&](int to, int from) { sum[to] -= sum[from]; });
 */
template <class S = union_find_internal::none, S (*op)(S, S) = union_find_internal::add<S>,
          S (*e)() = union_find_internal::zero<S>, bool Undoable = false>
struct union_find {
 private:
  static constexpr bool has_val = !std::is_same_v<S, union_find_internal::none>;
  // 巻き戻すには、消える根だけでなく残る根の元の値も要る（op に逆元が無いため）
  using hist = std::conditional_t<has_val, std::pair<std::array<int, 3>, S>, std::array<int, 3>>;

  mutable std::vector<int> dat;  // 負なら -(成分の大きさ)、非負なら親。経路圧縮で書き換わる
  std::vector<int> nxt;          // 成分ごとの巡回リスト
  int num;                       // 連結成分の個数
  [[no_unique_address]] std::conditional_t<has_val, std::vector<S>, std::tuple<>> val;  // 根に対してのみ意味を持つ
  // {子になった根, その根の元の dat, 親になった根}。併合しなかったときは {-1,0,0}
  [[no_unique_address]] std::conditional_t<Undoable, std::vector<hist>, std::tuple<>> hst;

  static const std::array<int, 3>& rec(const hist& h) {
    if constexpr (has_val) return h.first;
    else return h;
  }
  // keyed_union_find だけが使う。頂点を 1 つ増やして、その番号を返す
  int extend() {
    int i = (int)dat.size();
    dat.push_back(-1), nxt.push_back(i), num++;
    if constexpr (has_val) val.push_back(e());
    return i;
  }
  template <class K, class UF, class Map> friend struct keyed_union_find;

 public:
  union_find() : union_find(0) {}
  explicit union_find(int n) : dat(n, -1), nxt(n), num(n) {
    std::iota(nxt.begin(), nxt.end(), 0);
    if constexpr (has_val) val.assign(n, e());
  }
  // 頂点 i の初期値を v[i] とする
  explicit union_find(const std::vector<S>& v)
    requires has_val
      : dat(v.size(), -1), nxt(v.size()), num((int)v.size()), val(v) {
    std::iota(nxt.begin(), nxt.end(), 0);
  }

  int leader(int x) const {
    if constexpr (Undoable) {
      while (dat[x] >= 0) x = dat[x];  // 巻き戻すので経路は畳まない
    } else {
      while (dat[x] >= 0) {
        if (dat[dat[x]] >= 0) dat[x] = dat[dat[x]];  // 経路を半分に畳む
        x = dat[x];
      }
    }
    return x;
  }
  bool same(int a, int b) const { return leader(a) == leader(b); }
  int size(int x) const { return -dat[leader(x)]; }
  int group_count() const { return num; }

  // x の属する成分の総積
  const S& prod(int x) const
    requires has_val
  {
    return val[leader(x)];
  }
  // x の属する成分の値に v を合成する
  void apply(int x, const S& v)
    requires has_val
  {
    int r = leader(x);
    val[r] = op(val[r], v);
  }

  // 併合する。もともと別の成分だったら true
  bool merge(int a, int b) {
    return merge(a, b, [](int, int) {});
  }
  // 併合したとき f(残る根, 消える根) を呼ぶ。値を 1 つ載せるだけでは足りないときに使う
  template <class F> bool merge(int a, int b, F f) {
    int x = leader(a), y = leader(b);
    if (x == y) {
      if constexpr (Undoable) {  // undo の回数を merge と合わせるための空記録
        if constexpr (has_val) hst.push_back({{-1, 0, 0}, e()});
        else hst.push_back({-1, 0, 0});
      }
      return false;
    }
    if (-dat[x] < -dat[y]) std::swap(x, y);  // x を大きい方に
    if constexpr (Undoable) {
      if constexpr (has_val) hst.push_back({{y, dat[y], x}, val[x]});
      else hst.push_back({y, dat[y], x});
    }
    if constexpr (has_val) val[x] = op(val[x], val[y]);
    std::swap(nxt[x], nxt[y]);  // 2 つの環の next を交換すると 1 つの環になる
    dat[x] += dat[y];
    dat[y] = x;
    num--;
    f(x, y);
    return true;
  }

  // 直前の merge を 1 回取り消す。履歴が空なら何もしない
  void undo() { undo([](int, int) {}); }
  // 取り消す直前に f(残る根, 消える根) を呼ぶ。merge に渡したものを打ち消す関数を渡す
  template <class F> void undo(F f) {
    if (hst.empty()) return;
    hist h = hst.back();
    hst.pop_back();
    auto [y, dy, x] = rec(h);
    if (y < 0) return;
    f(x, y);
    if constexpr (has_val) val[x] = h.second;
    dat[y] = dy;
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
  template <class F> void rollback(int t, F f) {
    while (!hst.empty() && (int)hst.size() > t) undo(f);
  }

  // 構築直後の状態に戻す（大きさはそのまま）。値も e() に戻り、履歴も捨てる
  void clear() {
    std::fill(dat.begin(), dat.end(), -1);
    std::iota(nxt.begin(), nxt.end(), 0);
    num = (int)dat.size();
    if constexpr (has_val) std::fill(val.begin(), val.end(), e());
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
template <class S = union_find_internal::none, S (*op)(S, S) = union_find_internal::add<S>,
          S (*e)() = union_find_internal::zero<S>>
using rollback_union_find = union_find<S, op, e, true>;
