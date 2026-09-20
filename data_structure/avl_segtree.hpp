#include <algorithm>
#include <cassert>
#include <utility>
#include <vector>

namespace avl_segtree_internal {
// S が sz / fail を持つか。持たない型でも載せられるようにする
template <class S>
concept HasSz = requires(S x) { x.sz; };

template <class S> void set_sz(S& x, int n) {
  if constexpr (HasSz<S>) x.sz = n;
}

// 作用を使わないときの既定。F を指定したら mapping 以降も全部指定する
struct no_lazy {};
template <class S> S map_(no_lazy, S x) { return x; }
inline no_lazy comp_(no_lazy, no_lazy) { return {}; }
inline no_lazy id_() { return {}; }
}  // namespace avl_segtree_internal

// avl_segtree に載せる値の基底。sz と fail はライブラリが維持する
struct avl_value {
  int sz = 1;         // この値が集約している要素数
  bool fail = false;  // Beats: まとめて作用を適用できなかった
};

/*
 * avl_segtree<S, op, e, F, mapping, composition, id> : AVL 木で列を持つ
 *
 *   avl_segtree(v)     列 v から構築                O(n)
 *   avl_segtree(n)     e() を n 個                  O(n)
 *   size()
 *   insert(i, x)       i 番目の手前に x を挿す      O(log n)
 *   erase(i)           i 番目を消す                 O(log n)
 *   erase(l, r)        [l, r) を消す                O((log n) + 捨てる節点数)
 *   set(i, x)          i 番目を x にする            O(log n)
 *   apply(i, f)        i 番目に f を作用させる      O(log n)
 *   apply(l, r, f)     [l, r) に f を作用させる     O(log n)
 *   get(i)             i 番目                       O(log n)
 *   prod(l, r)         [l, r) の総積                O(log n)
 *   all_prod()         全体の総積                   O(1)
 *   to_vec()           列に戻す                     O(n)
 *   clear()            空にする
 *
 *   添字は 0 始まりの半開区間。l >= r の区間は prod なら e()、erase と apply は
 *   何もしない。
 *   S は avl_value を継承すると x.sz に要素数が入る（op や mapping の中で
 *   維持しなくてよい）。mapping を呼ぶ前に x.sz へ節点の要素数が入る。
 *   composition(f, g) は「g を適用してから f」。ACL の lazy_segtree と同じ。
 *   作用が要らないなら F 以降は省ける。省いた場合 apply は何もしない。
 *   節点は使い回すので、使うメモリは同時に存在する要素数に比例する。
 *   erase(l, r) は捨てる部分木をたどって節点を回収するので、その節点数ぶん
 *   かかる。今は 1 要素 1 葉なので 2 (r - l) 個。挿入したぶんしか消せないので
 *   ならし計算量は O(log n) のまま。
 *
 * 使用例:
 *   struct S : avl_value { ll sum = 0; };
 *   S op(S a, S b) { S r; r.sum = a.sum + b.sum; return r; }
 *   S e() { return S{}; }
 *   using F = ll;                                  // 区間加算
 *   S mapping(F f, S x) { x.sum += f * x.sz; return x; }
 *   F composition(F f, F g) { return f + g; }
 *   F id() { return 0; }
 *
 *   avl_segtree<S, op, e, F, mapping, composition, id> t(v);
 *   t.insert(3, x);
 *   t.apply(0, 5, 10);
 *   print(t.prod(0, 5).sum);
 *
 * verify:
 *   (未 verify)
 *   予定: https://judge.yosupo.jp/problem/dynamic_sequence_range_affine_range_sum
 */
template <class S, S (*op)(S, S), S (*e)(),
          class F = avl_segtree_internal::no_lazy,
          S (*mapping)(F, S) = avl_segtree_internal::map_<S>,
          F (*composition)(F, F) = avl_segtree_internal::comp_,
          F (*id)() = avl_segtree_internal::id_>
struct avl_segtree {
 private:
  struct node {
    int lft = -1, rht = -1;  // 子の添字。葉は -1
    int sz = 1;              // 部分木の要素数
    int rnk = 1;             // 部分木の高さ
    S val;                   // 葉なら自身の値、内部節点なら部分木の総積
    F laz = id();            // 子へまだ配っていない作用
    bool has_laz = false;
  };

  // 節点はここに置き、必ず int の添字で辿る。
  // ポインタや参照で持つと push_back の再確保で無効になる
  std::vector<node> pool;
  std::vector<int> free_;  // 使い終わった節点の添字
  int root = -1;

  /* ---- 確保と解放 ---- */

  int alloc() {
    if (free_.empty()) {
      pool.push_back(node{});
      return (int)pool.size() - 1;
    }
    int i = free_.back();
    free_.pop_back();
    pool[i] = node{};
    return i;
  }
  void kill(int i) {
    if (i >= 0) free_.push_back(i);
  }
  void kill_tree(int i) {
    if (i < 0) return;
    kill_tree(pool[i].lft);
    kill_tree(pool[i].rht);
    free_.push_back(i);
  }

  int make(const S& v) {
    int i = alloc();
    pool[i].val = v;
    avl_segtree_internal::set_sz(pool[i].val, 1);
    return i;
  }

  // 空の値。e() は sz を 1 のまま返しうるので 0 に直す
  static S zero_() {
    S x = e();
    avl_segtree_internal::set_sz(x, 0);
    return x;
  }

  int sz_(int i) const { return i < 0 ? 0 : pool[i].sz; }
  int rnk_(int i) const { return i < 0 ? 0 : pool[i].rnk; }

  /* ---- 遅延の伝播。子を見る前に必ず push を通す ---- */

  // 部分木ぜんぶに f を作用させる
  void apply_all(int i, const F& f) {
    if (i < 0) return;
    node& n = pool[i];
    avl_segtree_internal::set_sz(n.val, n.sz);
    n.val = mapping(f, n.val);
    avl_segtree_internal::set_sz(n.val, n.sz);
    if (n.lft < 0) return;  // 葉は配る先が無い
    n.laz = n.has_laz ? composition(f, n.laz) : f;
    n.has_laz = true;
  }

  // 溜めた作用を子へ配る
  void push(int i) {
    if (!pool[i].has_laz) return;
    F f = pool[i].laz;
    pool[i].has_laz = false;
    pool[i].laz = id();
    apply_all(pool[i].lft, f);
    apply_all(pool[i].rht, f);
  }

  // 子から自分を作り直す
  int update(int i) {
    node& n = pool[i];
    if (n.lft < 0) return i;
    n.sz = sz_(n.lft) + sz_(n.rht);
    n.rnk = std::max(rnk_(n.lft), rnk_(n.rht)) + 1;
    n.val = op(pool[n.lft].val, pool[n.rht].val);
    avl_segtree_internal::set_sz(n.val, n.sz);
    return i;
  }

  /* ---- 平衡 ---- */

  int rotate_l(int i) {
    int r = pool[i].rht;
    push(r);
    pool[i].rht = pool[r].lft;
    pool[r].lft = update(i);
    return update(r);
  }
  int rotate_r(int i) {
    int l = pool[i].lft;
    push(l);
    pool[i].lft = pool[l].rht;
    pool[l].rht = update(i);
    return update(l);
  }

  // 高さの差を 1 以下に戻す
  int balance(int i) {
    update(i);
    int d = rnk_(pool[i].rht) - rnk_(pool[i].lft);
    if (d > 1) {
      int r = pool[i].rht;
      push(r);
      if (rnk_(pool[r].lft) > rnk_(pool[r].rht)) pool[i].rht = rotate_r(r);
      return rotate_l(i);
    }
    if (d < -1) {
      int l = pool[i].lft;
      push(l);
      if (rnk_(pool[l].rht) > rnk_(pool[l].lft)) pool[i].lft = rotate_l(l);
      return rotate_r(i);
    }
    return i;
  }

  /* ---- つなぐ・切る ---- */

  // 2 つの木をつなぐ。高さの高い方の縁をたどって浅い方を吊るす
  int merge_(int a, int b) {
    if (a < 0) return b;
    if (b < 0) return a;
    if (rnk_(a) > rnk_(b) + 1) {
      push(a);
      pool[a].rht = merge_(pool[a].rht, b);
      return balance(a);
    }
    if (rnk_(b) > rnk_(a) + 1) {
      push(b);
      pool[b].lft = merge_(a, pool[b].lft);
      return balance(b);
    }
    int i = alloc();
    pool[i].lft = a;
    pool[i].rht = b;
    return update(i);
  }

  // 左から k 個とそれ以降に分ける
  std::pair<int, int> split_(int i, int k) {
    if (i < 0) return {-1, -1};
    if (k == 0) return {-1, i};
    if (k == sz_(i)) return {i, -1};
    push(i);
    int l = pool[i].lft, r = pool[i].rht, ls = sz_(l);
    kill(i);  // i はどちらの木にも残らない
    if (k < ls) {
      auto [a, b] = split_(l, k);
      return {a, merge_(b, r)};
    }
    if (k > ls) {
      auto [a, b] = split_(r, k - ls);
      return {merge_(l, a), b};
    }
    return {l, r};
  }

  /* ---- 木をたどる ---- */

  int build_(const std::vector<S>& v, int l, int r) {
    if (r - l == 1) return make(v[l]);
    int m = (l + r) / 2;
    int i = alloc();
    int a = build_(v, l, m), b = build_(v, m, r);
    pool[i].lft = a;
    pool[i].rht = b;
    return update(i);
  }

  // 葉を書き換えて、戻りながら祖先を作り直す
  void set_(int i, int k, const S& x) {
    if (pool[i].lft < 0) {
      pool[i].val = x;
      avl_segtree_internal::set_sz(pool[i].val, 1);
      return;
    }
    push(i);
    int ls = sz_(pool[i].lft);
    if (k < ls)
      set_(pool[i].lft, k, x);
    else
      set_(pool[i].rht, k - ls, x);
    update(i);
  }

  void apply_(int i, int l, int r, const F& f) {
    if (i < 0 || l >= r) return;
    if (l == 0 && r == pool[i].sz) {
      apply_all(i, f);
      return;
    }
    push(i);
    int ls = sz_(pool[i].lft);
    if (l < ls) apply_(pool[i].lft, l, std::min(ls, r), f);
    if (ls < r) apply_(pool[i].rht, std::max(0, l - ls), r - ls, f);
    update(i);
  }

  S prod_(int i, int l, int r) {
    if (i < 0 || l >= r) return zero_();
    if (l == 0 && r == pool[i].sz) return pool[i].val;
    push(i);
    int ls = sz_(pool[i].lft);
    S x;
    if (r <= ls)
      x = prod_(pool[i].lft, l, r);
    else if (ls <= l)
      x = prod_(pool[i].rht, l - ls, r - ls);
    else
      x = op(prod_(pool[i].lft, l, ls), prod_(pool[i].rht, 0, r - ls));
    avl_segtree_internal::set_sz(x, r - l);
    return x;
  }

  void to_vec_(int i, std::vector<S>& out) {
    if (i < 0) return;
    if (pool[i].lft < 0) {
      out.push_back(pool[i].val);
      return;
    }
    push(i);
    to_vec_(pool[i].lft, out);
    to_vec_(pool[i].rht, out);
  }

 public:
  avl_segtree() = default;
  explicit avl_segtree(int n) : avl_segtree(std::vector<S>(n, e())) {}
  explicit avl_segtree(const std::vector<S>& v) {
    if (v.empty()) return;
    pool.reserve(2 * v.size());
    root = build_(v, 0, (int)v.size());
  }

  void clear() {
    pool.clear();
    free_.clear();
    root = -1;
  }

  int size() const { return sz_(root); }

  // i 番目の手前に x を挿す。i == size() なら末尾に足す
  void insert(int i, const S& x) {
    assert(0 <= i && i <= size());
    auto [a, b] = split_(root, i);
    root = merge_(merge_(a, make(x)), b);
  }

  void erase(int i) {
    assert(0 <= i && i < size());
    erase(i, i + 1);
  }

  // [l, r) を消す。l >= r なら何もしない
  void erase(int l, int r) {
    assert(0 <= l && r <= size());
    if (l >= r) return;
    auto [a, b] = split_(root, l);
    auto [c, d] = split_(b, r - l);
    kill_tree(c);
    root = merge_(a, d);
  }

  // i 番目を x にする
  void set(int i, const S& x) {
    assert(0 <= i && i < size());
    set_(root, i, x);
  }

  void apply(int i, const F& f) {
    assert(0 <= i && i < size());
    apply(i, i + 1, f);
  }

  // [l, r) に f を作用させる。l >= r なら何もしない
  void apply(int l, int r, const F& f) {
    assert(0 <= l && r <= size());
    if (l >= r) return;
    apply_(root, l, r, f);
  }

  S get(int i) {
    assert(0 <= i && i < size());
    return prod_(root, i, i + 1);
  }

  // [l, r) の総積。l >= r なら e()
  S prod(int l, int r) {
    assert(0 <= l && r <= size());
    return prod_(root, l, std::max(l, r));
  }

  S all_prod() const { return root < 0 ? zero_() : pool[root].val; }

  std::vector<S> to_vec() {
    std::vector<S> out;
    out.reserve(size());
    to_vec_(root, out);
    return out;
  }
};
