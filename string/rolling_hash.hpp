#include <algorithm>
#include <cassert>
#include <chrono>
#include <random>
#include <string>
#include <vector>

/*
 * rolling_hash<Updatable> : 列の部分列のハッシュを引く
 *
 *   rolling_hash(a)        文字列でも vector<整数> でも作れる
 *   size()
 *   get(l, r)              [l, r) のハッシュ
 *   get(i)                 1 要素ぶん
 *   rev(l, r)              [l, r) を逆から読んだハッシュ
 *   same(l1, r1, l2, r2)   2 つの部分列が一致するか
 *   palindrome(l, r)       [l, r) が回文か
 *   lcp(i, j)              位置 i と j から始まる部分列の最長共通接頭辞の長さ
 *   lcp(o, i, j)           別の列 o の位置 j との最長共通接頭辞
 *   set(i, v)              Updatable のときだけ。1 点更新
 *   concat(x, y, len_y)    長さ len_y の y を x の後ろに繋いだハッシュ
 *
 *   rolling_hash<>        静的。構築 O(n)、get は O(1)
 *   dynamic_rolling_hash  更新あり。BIT を使うので get も set も O(log n)
 *
 *   法は 2^61 - 1、基数は実行のたびにランダム。基数と冪の表は全インスタンスで
 *   共有するので、別々に作った列どうしでもハッシュを比べられる。
 *   逆から読む用の表は rev / palindrome を最初に呼んだときに作る（メモリ 2 倍）。
 *
 *   要素は 0 以上 2^61 - 3 以下であること。負の値は入れない。
 *   中では 1 を足して持つ。そうしないと 0 だけの列で長さの違いが出ない。
 *   ハッシュの一致は確率的で、同じ実行の中でしか比べられない。
 *
 * 使用例:
 *   // S の中に T と一致する場所があるか
 *   rolling_hash s(S), t(T);
 *   ll m = t.size();
 *   rep(i, s.size() - m + 1) if (s.get(i, i + m) == t.get(0, m)) print(i);
 *
 *   // 回文判定
 *   if (s.palindrome(l, r)) print("Yes");
 */
namespace rolling_hash_internal {
using u64 = unsigned long long;
constexpr u64 MOD = (1ULL << 61) - 1;

inline u64 add_(u64 a, u64 b) {
  a += b;
  return a >= MOD ? a - MOD : a;
}
inline u64 sub_(u64 a, u64 b) { return add_(a, MOD - b); }
inline u64 mul_(u64 a, u64 b) {
  __uint128_t t = (__uint128_t)a * b;
  return add_((u64)(t >> 61), (u64)(t & MOD));
}

// 基数と冪の表は全インスタンスで共有する。別の列と比べられるようにするため
inline u64 base_ = [] {
  std::mt19937_64 rng((u64)std::chrono::steady_clock::now().time_since_epoch().count());
  return std::uniform_int_distribution<u64>(1 << 30, MOD - 2)(rng);
}();
inline std::vector<u64> pow_{1};

// 値はそのままでなく 1 足して持つ。0 のままだと {0} と {0, 0} と空列が
// 同じハッシュになり、長さの違いが出ないため
inline u64 enc(u64 v) {
  assert(v < MOD - 1 && "要素は 0 以上 2^61 - 3 以下");
  return v + 1;
}

// vector の再確保がならしてくれるので、ここで倍々にはしない
inline void grow(std::size_t n) {
  std::size_t m = pow_.size();
  if (n < m) return;
  pow_.resize(n + 1);
  for (std::size_t i = m; i <= n; i++) pow_[i] = mul_(pow_[i - 1], base_);
}

// 列 1 本ぶんの累積。Updatable なら BIT で持つ
template <bool Updatable> struct core {
  int n = 0;
  std::vector<u64> s, h;

  void init(const std::vector<u64>& a) {
    n = (int)a.size();
    grow(n);
    s.assign(n, 0), h.assign(n + 1, 0);
    if constexpr (Updatable) {
      for (int i = 0; i < n; i++) set(i, a[i]);
    } else {
      for (int i = 0; i < n; i++) s[i] = a[i], h[i + 1] = add_(mul_(h[i], base_), a[i]);
    }
  }

  void set(int i, u64 v) {
    static_assert(Updatable, "更新できるのは BIT で持っているときだけ");
    // BIT の枝 x は「位置 i の寄与を pow[x-i-1] 倍して」持つ
    for (int x = i + 1; x <= n; x += x & -x) h[x] = add_(h[x], mul_(sub_(v, s[i]), pow_[x - i - 1]));
    s[i] = v;
  }

  // [0, i) のハッシュ
  u64 pref(int i) const {
    if constexpr (Updatable) {
      u64 r = 0;
      for (int x = i; x > 0; x -= x & -x) r = add_(r, mul_(h[x], pow_[i - x]));
      return r;
    } else {
      return h[i];
    }
  }

  u64 get(int l, int r) const { return sub_(pref(r), mul_(pref(l), pow_[r - l])); }
};
}  // namespace rolling_hash_internal

template <bool Updatable = false> struct rolling_hash {
  using u64 = rolling_hash_internal::u64;

 private:
  int n = 0;
  rolling_hash_internal::core<Updatable> f;
  mutable rolling_hash_internal::core<Updatable> b;  // 逆から読んだ列。要るまで作らない
  mutable bool has_b = false;

  void build_rev() const {
    if (has_b) return;
    std::vector<u64> t(n);
    for (int i = 0; i < n; i++) t[i] = f.s[n - 1 - i];
    b.init(t), has_b = true;
  }

 public:
  rolling_hash() = default;

  template <class C> explicit rolling_hash(const C& a) : n((int)a.size()) {
    std::vector<u64> t(n);
    int i = 0;
    for (auto&& x : a) t[i++] = rolling_hash_internal::enc((u64)x);
    f.init(t);
  }

  int size() const { return n; }

  // [l, r) のハッシュ
  u64 get(int l, int r) const {
    assert(0 <= l && l <= r && r <= n);
    return f.get(l, r);
  }
  u64 get(int i) const { return get(i, i + 1); }

  // [l, r) を逆から読んだハッシュ
  u64 rev(int l, int r) const {
    assert(0 <= l && l <= r && r <= n);
    return build_rev(), b.get(n - r, n - l);
  }

  // 1 点更新。Updatable のときだけ
  void set(int i, u64 v) {
    static_assert(Updatable, "更新するなら dynamic_rolling_hash を使う");
    assert(0 <= i && i < n);
    u64 e = rolling_hash_internal::enc(v);
    f.set(i, e);
    if (has_b) b.set(n - 1 - i, e);
  }

  bool same(int l1, int r1, int l2, int r2) const {
    return r1 - l1 == r2 - l2 && get(l1, r1) == get(l2, r2);
  }
  bool palindrome(int l, int r) const { return get(l, r) == rev(l, r); }

  // 位置 i と j から始まる部分列の、最長共通接頭辞の長さ
  int lcp(int i, int j) const { return lcp(*this, i, j); }

  // 別の列 o の位置 j から始まる部分列との、最長共通接頭辞の長さ
  template <bool U> int lcp(const rolling_hash<U>& o, int i, int j) const {
    assert(0 <= i && i <= n);
    assert(0 <= j && j <= o.size());
    int lo = 0, hi = std::min(n - i, o.size() - j) + 1;
    while (hi - lo > 1) {
      int mid = (lo + hi) / 2;
      (get(i, i + mid) == o.get(j, j + mid) ? lo : hi) = mid;
    }
    return lo;
  }

  // 長さ len_y の y を x の後ろに繋いだハッシュ
  static u64 concat(u64 x, u64 y, int len_y) {
    rolling_hash_internal::grow(len_y);
    return rolling_hash_internal::add_(rolling_hash_internal::mul_(x, rolling_hash_internal::pow_[len_y]), y);
  }
};

using dynamic_rolling_hash = rolling_hash<true>;
