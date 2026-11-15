#include <concepts>
#include <iterator>
#include <type_traits>

// https://github.com/yuuki1n/cp-library/blob/main/util/set_search.hpp

/*
 * set_search : set / map を Java の TreeSet / TreeMap と同じ名前で探す
 *
 *   set / multiset                  map / multimap（キーを返す）
 *   ceiling(s, x)  ceiling_key(m, x)  x 以上の最小      無ければ +∞
 *   higher(s, x)   higher_key(m, x)   x より大きい最小  無ければ +∞
 *   floor(s, x)    floor_key(m, x)    x 以下の最大      無ければ -∞
 *   lower(s, x)    lower_key(m, x)    x より小さい最大  無ければ -∞
 *
 *   どれも O(log n)。第 3 引数 def を渡すと、無いときはそれを返す。
 *   ∞ は 4 バイトの整数（int）なら 1001001001、8 バイトの整数（ll）なら
 *   2002003004005006007（テンプレートの INF / LINF と同じ値で、2 つ足しても溢れない）。
 *
 *   def を必ず渡すとき:
 *     - キーが整数でない（pair / string など）。省くとコンパイルエラーになる
 *     - キーが char / bool など 4 バイト未満の整数。省くとコンパイルエラーになる。
 *       ∞ にあたる値が無く、切り詰めると実在する値と区別できないため。例: higher(sc, c, '\0')
 *     - unsigned で floor / lower を使う（-∞ が回り込んで大きな正の数になる）
 *     - 降順の set / map（greater<> など）。探す向きは比較関数の順なので
 *       ceiling は「x 以下の最大」になるが、無いときの既定値は +∞ のまま
 *
 *   set 用の関数に map を渡す（逆も）と、呼んだ行で「合う関数が無い」エラーになる。
 *   値が欲しいときは m.lower_bound(x)->second などを使う。
 *   floor は std::floor と同じ名前だが、引数の数が違うので floor(2.7) はそのまま使える。
 *
 * 使用例:
 *   set<ll> s{1, 3, 5};
 *   higher(s, 3);     // 5
 *   higher(s, 5);     // LINF
 *   higher(s, 5, N);  // N
 *   lower(s, 1);      // -LINF
 *   map<ll, int> m{{2, 10}, {4, 20}};
 *   floor_key(m, 3);  // 2
 */
namespace set_search_internal {
template <class T> constexpr T inf() {
  // char / bool は ∞ にあたる値が無い（切り詰めると実在する値と区別できない）ので作らない
  static_assert(std::is_integral_v<T> && sizeof(T) >= 4, "整数以外と、char / bool など 4 バイト未満の整数は def を渡す");
  if constexpr (sizeof(T) == 4) return (T)1001001001;
  else return (T)2002003004005006007LL;
}
template <class C> using key = typename C::key_type;

// 順序つきで二分探索できるもの
template <class C>
concept ordered = requires(const C& s, const key<C>& x) {
  s.lower_bound(x);
  s.upper_bound(x);
};
// set / multiset：要素そのものがキー
template <class C>
concept set_like = ordered<C> && std::same_as<key<C>, typename C::value_type>;
// map / multimap：要素は (キー, 値)
template <class C>
concept map_like = ordered<C> && requires { typename C::mapped_type; };

// it が指す要素のキー。end なら def
template <class C> key<C> key_at(const C& s, typename C::const_iterator it, const key<C>& def) {
  if (it == s.end()) return def;
  if constexpr (map_like<C>) return it->first;
  else return *it;
}
// it の 1 つ手前。先頭なら end を返して「無い」を表す
template <class C> typename C::const_iterator before(const C& s, typename C::const_iterator it) {
  return it == s.begin() ? s.end() : std::prev(it);
}

template <class C> key<C> ceiling_(const C& s, const key<C>& x, const key<C>& def) { return key_at(s, s.lower_bound(x), def); }
template <class C> key<C> higher_(const C& s, const key<C>& x, const key<C>& def) { return key_at(s, s.upper_bound(x), def); }
template <class C> key<C> floor_(const C& s, const key<C>& x, const key<C>& def) { return key_at(s, before(s, s.upper_bound(x)), def); }
template <class C> key<C> lower_(const C& s, const key<C>& x, const key<C>& def) { return key_at(s, before(s, s.lower_bound(x)), def); }
}  // namespace set_search_internal

/* ---- set / multiset ---- */

template <set_search_internal::set_like C>
set_search_internal::key<C> ceiling(const C& s, const set_search_internal::key<C>& x,
                                    const set_search_internal::key<C>& def = set_search_internal::inf<set_search_internal::key<C>>()) {
  return set_search_internal::ceiling_(s, x, def);
}
template <set_search_internal::set_like C>
set_search_internal::key<C> higher(const C& s, const set_search_internal::key<C>& x,
                                   const set_search_internal::key<C>& def = set_search_internal::inf<set_search_internal::key<C>>()) {
  return set_search_internal::higher_(s, x, def);
}
template <set_search_internal::set_like C>
set_search_internal::key<C> floor(const C& s, const set_search_internal::key<C>& x,
                                  const set_search_internal::key<C>& def = -set_search_internal::inf<set_search_internal::key<C>>()) {
  return set_search_internal::floor_(s, x, def);
}
template <set_search_internal::set_like C>
set_search_internal::key<C> lower(const C& s, const set_search_internal::key<C>& x,
                                  const set_search_internal::key<C>& def = -set_search_internal::inf<set_search_internal::key<C>>()) {
  return set_search_internal::lower_(s, x, def);
}

/* ---- map / multimap（キーを返す）---- */

template <set_search_internal::map_like C>
set_search_internal::key<C> ceiling_key(const C& m, const set_search_internal::key<C>& x,
                                        const set_search_internal::key<C>& def = set_search_internal::inf<set_search_internal::key<C>>()) {
  return set_search_internal::ceiling_(m, x, def);
}
template <set_search_internal::map_like C>
set_search_internal::key<C> higher_key(const C& m, const set_search_internal::key<C>& x,
                                       const set_search_internal::key<C>& def = set_search_internal::inf<set_search_internal::key<C>>()) {
  return set_search_internal::higher_(m, x, def);
}
template <set_search_internal::map_like C>
set_search_internal::key<C> floor_key(const C& m, const set_search_internal::key<C>& x,
                                      const set_search_internal::key<C>& def = -set_search_internal::inf<set_search_internal::key<C>>()) {
  return set_search_internal::floor_(m, x, def);
}
template <set_search_internal::map_like C>
set_search_internal::key<C> lower_key(const C& m, const set_search_internal::key<C>& x,
                                      const set_search_internal::key<C>& def = -set_search_internal::inf<set_search_internal::key<C>>()) {
  return set_search_internal::lower_(m, x, def);
}
