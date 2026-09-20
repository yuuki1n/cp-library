#include <algorithm>
#include <cassert>
#include <utility>
#include <vector>

namespace avl_segtree_internal {
// S が sz / fail を持つか。持たない型でも載せられるようにする
template <class S>
concept HasSz = requires(S x) { x.sz; };
template <class S>
concept HasFail = requires(S x) { x.fail; };

// 作用を使わないときの既定。F を指定したら mapping 以降も全部指定する
struct no_lazy {};
template <class S> S map_(no_lazy, S x) { return x; }
inline no_lazy comp_(no_lazy, no_lazy) { return {}; }
inline no_lazy id_() { return {}; }

// 既定の rev。和や min のように向きに依らない集約なら何もしなくてよい
template <class S> S rev_(S x) { return x; }
}  // namespace avl_segtree_internal

// avl_segtree に載せる値の基底。sz と fail はライブラリが維持する
struct avl_value {
  int sz = 1;         // この値が集約している要素数
  bool fail = false;  // Beats: まとめて作用を適用できなかった
};

/*
 * avl_segtree<S, op, e, F, mapping, composition, id, rev> : AVL 木で列を持つ
 *
 *   avl_segtree(v)     列 v から構築                O(n)
 *   avl_segtree(n)     e() を n 個                  O(1)
 *   avl_segtree(n, x)  x を n 個                    O(1)
 *   size()
 *   insert(i, x)       i 番目の手前に x を挿す      O(log n)
 *   insert(i, x, k)    x を k 個まとめて挿す        O(log n)
 *   erase(i)           i 番目を消す                 O(log n)
 *   erase(l, r)        [l, r) を消す                O((log n) + 捨てる節点数)
 *   set(i, x)          i 番目を x にする            O(log n)
 *   apply(i, f)        i 番目に f を作用させる      O(log n)
 *   apply(l, r, f)     [l, r) に f を作用させる     O(log n)
 *   reverse(l, r)      [l, r) を逆順にする          O(log n)
 *   rotate(l, r, k)    [l, r) を左へ k 巡回させる   O(log n)
 *   get(i)             i 番目                       O(log n)
 *   prod(l, r)         [l, r) の総積                O(log n)
 *   all_prod()         全体の総積                   O(1)
 *   to_vec()           列に戻す                     O(n)
 *   clear()            空にする
 *
 * 区間の約束:
 *   - 添字は 0 始まりの半開区間 [l, r)
 *   - l >= r は prod が e()、他（erase / apply / reverse / rotate）は何もしない
 *   - rotate の k は |k| < r - l であること。負を渡すと右へ回る
 *
 * S に求めること:
 *   - avl_value を継承すると x.sz に要素数が入る。op や mapping で維持しない
 *   - mapping を呼ぶ前に x.sz へ要素数が入っている
 *   - rev(x) は逆順にしたときの集約値。和や min なら既定のまま。
 *     文字列の連結など向きで変わるものだけ指定する
 *
 * F に求めること:
 *   - composition(f, g) は「g を適用してから f」。ACL の lazy_segtree と同じ
 *   - 要らなければ F 以降を省ける。省くと apply は何もしない
 *   - 区間 chmin のようにまとめて適用できないことがあるなら（Beats）、
 *     mapping の中で x.fail = true を立てる。ライブラリが子へ降りて作り直す。
 *     1 要素への作用は必ず適用できること（降りる先が無いため）
 *
 * 葉のまとめ方:
 *   - 葉は「同じ値が k 個」を持つ。avl_segtree(n) や insert(i, x, k) が節点 1 個
 *     で済み、触られるまで分かれない
 *   - まとまった葉の総積を読むときだけ op を O(log k) 回呼ぶ
 *   - 節点は使い回すので、メモリは同時に存在する葉の数に比例する
 *   - erase(l, r) は捨てる部分木をたどるので、その節点数ぶんかかる。挿入した
 *     ぶんしか消せないのでならし計算量は O(log n) のまま
 *   - 要素数は int に収まる範囲まで。to_vec() は要素の数ぶん作るので、
 *     まとまった葉を大きく取ったまま呼ぶとメモリを食う
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
 *   t.reverse(1, 4);
 *   t.rotate(0, 5, 2);            // 添字 2 の要素が先頭に来る
 *   print(t.prod(0, 5).sum);
 *
 * verify:
 *   https://judge.yosupo.jp/problem/dynamic_sequence_range_affine_range_sum
 */
template <class S, S (*op)(S, S), S (*e)(), class F = avl_segtree_internal::no_lazy, S (*mapping)(F, S) = avl_segtree_internal::map_<S>,
          F (*composition)(F, F) = avl_segtree_internal::comp_, F (*id)() = avl_segtree_internal::id_,
          S (*rev)(S) = avl_segtree_internal::rev_<S>>
struct avl_segtree {
 private:
  struct node {
    int lft = -1, rht = -1;  // 子の添字。葉は -1
    int sz = 1;              // 内部節点は部分木の要素数、葉はまとめた個数
    int rnk = 1;             // 部分木の高さ
    S val;                   // 内部節点は総積、葉は 1 要素ぶんの値
    F laz = id();            // 子へまだ配っていない作用
    bool has_laz = false;
    bool has_rev = false;  // 子へまだ配っていない反転
  };

  // 節点はここに置き、必ず int の添字で辿る。
  // ポインタや参照で持つと push_back の再確保で無効になる
  std::vector<node> pool;
  std::vector<int> free_;  // 使い終わった節点の添字
  int root = -1;

  /* ---- S の sz / fail を触る。持たない型なら何もしない ---- */

  static constexpr bool has_sz_ = avl_segtree_internal::HasSz<S>;
  static constexpr bool has_fail_ = avl_segtree_internal::HasFail<S>;

  static void set_sz(S& x, int n) {
    if constexpr (has_sz_) x.sz = n;
  }
  static void set_fail(S& x, bool b) {
    if constexpr (has_fail_) x.fail = b;
  }
  static bool failed(const S& x) {
    if constexpr (has_fail_) return x.fail;
    return false;
  }

  /* ---- 確保と解放 ---- */

  int alloc() {
    int i = (int)pool.size();
    if (free_.empty()) {
      pool.push_back(node{});
    } else {
      i = free_.back();
      free_.pop_back();
    }
    // val と laz は呼び出し側が入れ直すか、フラグが false の間は読まれない
    node& n = pool[i];
    n.lft = n.rht = -1;
    n.sz = n.rnk = 1;
    n.has_laz = n.has_rev = false;
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

  // 値 v が k 個ぶん並ぶ葉を作る
  int make(const S& v, int k = 1) {
    int i = alloc();
    pool[i].val = v;
    pool[i].sz = k;
    set_sz(pool[i].val, 1);
    return i;
  }

  // 空の値。e() は sz を 1 のまま返しうるので 0 に直す
  static S zero_() {
    S x = e();
    set_sz(x, 0);
    return x;
  }

  // a が n 個並んだときの総積。O(log n)
  static S pow_(const S& a, int n) {
    if (n <= 0) return zero_();
    S x = a;
    set_sz(x, 1);
    if (n == 1) return x;
    S r = zero_();
    int rn = 0, xn = 1;
    while (n) {
      if (n & 1) {
        r = rn == 0 ? x : op(r, x);
        set_sz(r, rn += xn);
      }
      n >>= 1;
      if (n) {
        x = op(x, x);
        set_sz(x, xn *= 2);
      }
    }
    return r;
  }

  // d < 0 で左の子、d > 0 で右の子。確保をまたいで持ち越さないこと
  int& child_(int i, int d) { return d < 0 ? pool[i].lft : pool[i].rht; }

  int sz_(int i) const { return i < 0 ? 0 : pool[i].sz; }
  int rnk_(int i) const { return i < 0 ? 0 : pool[i].rnk; }

  // 節点の総積。まとまった葉だけ冪を計算する
  S agg_(int i) const {
    const node& n = pool[i];
    if (n.lft >= 0 || n.sz == 1) return n.val;
    return pow_(n.val, n.sz);
  }

  // まとまった葉を左 k 個・右 (sz - k) 個に割る。i は内部節点になる
  void split_leaf(int i, int k) {
    int sz = pool[i].sz;
    S v = pool[i].val;  // 確保で pool が動くので先に控える
    int a = make(v, k), b = make(v, sz - k);
    pool[i].lft = a;
    pool[i].rht = b;
    update(i);
  }

  /* ---- 遅延の伝播。子を見る前に必ず push を通す ---- */

  // 部分木ぜんぶに f を作用させる
  void apply_all(int i, const F& f) {
    if (i < 0) return;
    // 葉は 1 要素ぶんの値を持つので、作用もその単位でかける
    int m = pool[i].lft < 0 ? 1 : pool[i].sz;
    set_sz(pool[i].val, m);
    set_fail(pool[i].val, false);
    pool[i].val = mapping(f, pool[i].val);
    set_sz(pool[i].val, m);
    if (pool[i].lft < 0) {
      // 1 要素への作用は必ず適用できること。降りる先が無いため
      assert(!failed(pool[i].val));
      return;
    }
    pool[i].laz = pool[i].has_laz ? composition(f, pool[i].laz) : f;
    pool[i].has_laz = true;
    if (failed(pool[i].val)) {
      // まとめて適用できなかった。子へ配ってから作り直す
      push(i);
      update(i);
    }
  }

  // 部分木ぜんぶを反転させる
  void reverse_all(int i) {
    if (i < 0) return;
    node& n = pool[i];
    if (n.lft < 0) {  // 同じ値が並ぶだけなので並び順は変わらない
      set_sz(n.val, 1);
      n.val = rev(n.val);
      set_sz(n.val, 1);
      return;
    }
    std::swap(n.lft, n.rht);
    set_sz(n.val, n.sz);
    n.val = rev(n.val);
    set_sz(n.val, n.sz);
    n.has_rev = !n.has_rev;
  }

  // 溜めた作用と反転を子へ配る
  void push(int i) {
    if (pool[i].has_laz) {
      F f = pool[i].laz;
      pool[i].has_laz = false;
      pool[i].laz = id();
      apply_all(pool[i].lft, f);
      apply_all(pool[i].rht, f);
    }
    if (pool[i].has_rev) {
      pool[i].has_rev = false;
      reverse_all(pool[i].lft);
      reverse_all(pool[i].rht);
    }
  }

  // 子から自分を作り直す
  int update(int i) {
    node& n = pool[i];
    if (n.lft < 0) return i;
    n.sz = sz_(n.lft) + sz_(n.rht);
    n.rnk = std::max(rnk_(n.lft), rnk_(n.rht)) + 1;
    n.val = op(agg_(n.lft), agg_(n.rht));
    set_sz(n.val, n.sz);
    // 節点に持つ値は必ず fail を下ろしておく。op が入力を使い回しても残らない
    set_fail(n.val, false);
    return i;
  }

  /* ---- 平衡 ---- */

  // d 方向へ回す。d > 0 なら左回転（右の子を持ち上げる）
  int rot_(int i, int d) {
    int c = child_(i, d);
    push(c);
    child_(i, d) = child_(c, -d);
    child_(c, -d) = update(i);
    return update(c);
  }

  // 高さの差を 1 以下に戻す
  int balance(int i) {
    update(i);
    int d = rnk_(pool[i].rht) - rnk_(pool[i].lft);
    if (-1 <= d && d <= 1) return i;
    int s = d > 0 ? 1 : -1, c = child_(i, s);  // s は深い側
    push(c);
    // 深い側の子が逆向きに伸びていたら、先にそちらを回す
    if (rnk_(child_(c, -s)) > rnk_(child_(c, s))) child_(i, s) = rot_(c, -s);
    return rot_(i, s);
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
    if (pool[i].lft < 0) split_leaf(i, k);  // まとまった葉の途中で切る
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

  // [l, r) を切り出して f に渡し、返ってきた部分木をつなぎ直す。
  // f は受け取った部分木を好きにしてよく、要らなければ -1 を返す
  template <class Fn> void on_range_(int l, int r, Fn f) {
    auto [a, b] = split_(root, l);
    auto [c, d] = split_(b, r - l);
    root = merge_(merge_(a, f(c)), d);
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

  // k 番目の手前に「x が cnt 個」を挿し、戻りながら平衡を直す
  int insert_(int i, int k, const S& x, int cnt) {
    if (pool[i].lft < 0) {
      if (0 < k && k < pool[i].sz) {
        split_leaf(i, k);  // まとまった葉の途中なら先に割る
      } else {             // 葉の端なので、葉と新しい親を 1 つずつ作る
        int leaf = make(x, cnt), p = alloc();
        pool[p].lft = k == 0 ? leaf : i;
        pool[p].rht = k == 0 ? i : leaf;
        return update(p);
      }
    }
    push(i);
    int ls = sz_(pool[i].lft), d = k < ls ? -1 : 1;
    child_(i, d) = insert_(child_(i, d), d < 0 ? k : k - ls, x, cnt);
    return balance(i);
  }

  // k 番目を消す。部分木が空になったら -1 を返す
  int erase_(int i, int k) {
    if (pool[i].lft < 0) {
      if (pool[i].sz > 1) {  // 同じ値の並びなので 1 つ減らすだけ
        pool[i].sz--;
        return i;
      }
      kill(i);
      return -1;
    }
    push(i);
    int ls = sz_(pool[i].lft), d = k < ls ? -1 : 1;
    int t = erase_(child_(i, d), d < 0 ? k : k - ls);
    if (t < 0) {  // 片方が消えたので、もう片方を親の位置へ繰り上げる
      int sib = child_(i, -d);
      kill(i);
      return sib;
    }
    child_(i, d) = t;
    return balance(i);
  }

  // k 番目を x にする。まとまった葉なら 1 個だけ切り出してから書き換える
  int set_(int i, int k, const S& x) {
    if (pool[i].lft < 0) {
      if (pool[i].sz == 1) {
        pool[i].val = x;
        set_sz(pool[i].val, 1);
        return i;
      }
      split_leaf(i, k > 0 ? k : 1);
    }
    push(i);
    int ls = sz_(pool[i].lft), d = k < ls ? -1 : 1;
    child_(i, d) = set_(child_(i, d), d < 0 ? k : k - ls, x);
    return balance(i);
  }

  int apply_(int i, int l, int r, const F& f) {
    if (i < 0 || l >= r) return i;
    if (l == 0 && r == pool[i].sz) {
      apply_all(i, f);
      return i;
    }
    // まとまった葉の一部だけに作用させるので、境目で割る
    if (pool[i].lft < 0) split_leaf(i, l > 0 ? l : r);
    push(i);
    int ls = sz_(pool[i].lft);
    if (l < ls) pool[i].lft = apply_(pool[i].lft, l, std::min(ls, r), f);
    if (ls < r) pool[i].rht = apply_(pool[i].rht, std::max(0, l - ls), r - ls, f);
    return balance(i);
  }

  S prod_(int i, int l, int r) {
    if (i < 0 || l >= r) return zero_();
    if (pool[i].lft < 0) return pow_(pool[i].val, r - l);  // 葉
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
    set_sz(x, r - l);
    return x;
  }

  void to_vec_(int i, std::vector<S>& out) {
    if (i < 0) return;
    if (pool[i].lft < 0) {
      S v = pool[i].val;
      set_sz(v, 1);
      out.insert(out.end(), (size_t)pool[i].sz, v);
      return;
    }
    push(i);
    to_vec_(pool[i].lft, out);
    to_vec_(pool[i].rht, out);
  }

 public:
  avl_segtree() = default;
  // e() を n 個。葉 1 つで持つので O(1)
  explicit avl_segtree(int n) : avl_segtree(n, e()) {}
  // x を n 個。葉 1 つで持つので O(1)
  avl_segtree(int n, const S& x) {
    assert(0 <= n);
    if (n > 0) root = make(x, n);
  }
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

  // i 番目の手前に x を k 個挿す。i == size() なら末尾に足す
  void insert(int i, const S& x, int k = 1) {
    assert(0 <= i && i <= size());
    assert(0 < k);
    root = root < 0 ? make(x, k) : insert_(root, i, x, k);
  }

  void erase(int i) {
    assert(0 <= i && i < size());
    root = erase_(root, i);
  }

  // [l, r) を消す。l >= r なら何もしない
  void erase(int l, int r) {
    assert(0 <= l && r <= size());
    if (l >= r) return;
    on_range_(l, r, [&](int c) {
      kill_tree(c);
      return -1;
    });
  }

  // i 番目を x にする
  void set(int i, const S& x) {
    assert(0 <= i && i < size());
    root = set_(root, i, x);
  }

  void apply(int i, const F& f) {
    assert(0 <= i && i < size());
    apply(i, i + 1, f);
  }

  // [l, r) に f を作用させる。l >= r なら何もしない
  void apply(int l, int r, const F& f) {
    assert(0 <= l && r <= size());
    if (l >= r) return;
    root = apply_(root, l, r, f);
  }

  // [l, r) を逆順にする。l >= r なら何もしない
  void reverse(int l, int r) {
    assert(0 <= l && r <= size());
    if (l >= r) return;
    on_range_(l, r, [&](int c) {
      reverse_all(c);
      return c;
    });
  }

  // [l, r) を左へ k だけ巡回させる。l + k 番目が l 番目に来る。
  // 負を渡すと右へ回る。長さ 1 以下なら何もしない
  void rotate(int l, int r, int k) {
    assert(0 <= l && r <= size());
    int n = r - l;
    if (n <= 1) return;
    assert(-n < k && k < n);  // 丸めない。範囲外は呼び出し側の間違い
    if (k < 0) k += n;
    if (k == 0) return;
    on_range_(l, r, [&](int c) {
      auto [head, tail] = split_(c, k);  // 前 k 個を後ろへ回す
      return merge_(tail, head);
    });
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

  S all_prod() const { return root < 0 ? zero_() : agg_(root); }

  std::vector<S> to_vec() {
    std::vector<S> out;
    out.reserve(size());
    to_vec_(root, out);
    return out;
  }
};
