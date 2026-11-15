#include <iterator>
#include <type_traits>

// https://github.com/yuuki1n/cp-library/blob/main/util/set_search.hpp

/*
 * set_search : set / multiset を Java の TreeSet と同じ名前で探す
 *
 *   ceiling(s, x)  x 以上の最小      無ければ +∞
 *   higher(s, x)   x より大きい最小  無ければ +∞
 *   floor(s, x)    x 以下の最大      無ければ -∞
 *   lower(s, x)    x より小さい最大  無ければ -∞
 *
 *   どれも O(log n)。第 3 引数 def を渡すと、無いときはそれを返す。
 *   ∞ は int（4 バイト以下）なら 1001001001、それより大きい整数なら
 *   2002003004005006007（テンプレートの INF / LINF と同じ値で、2 つ足しても溢れない）。
 *   整数以外（pair など）は def を必ず渡す。省くとコンパイルエラーになる。
 *   unsigned の set で floor / lower を使うときも def を渡すこと（-∞ が回り込むため）。
 *
 *   floor は std::floor と同じ名前だが、引数の数が違うので floor(2.7) はそのまま使える。
 *
 * 使用例:
 *   set<ll> s{1, 3, 5};
 *   higher(s, 3);     // 5
 *   higher(s, 5);     // LINF
 *   higher(s, 5, N);  // N
 *   lower(s, 1);      // -LINF
 */
namespace set_search_internal {
template <class T> constexpr T inf() {
  static_assert(std::is_integral_v<T>, "整数以外は def を渡す");
  if constexpr (sizeof(T) <= 4) return (T)1001001001;
  else return (T)2002003004005006007LL;
}
template <class C> using key = typename C::key_type;
}  // namespace set_search_internal

template <class C>
set_search_internal::key<C> ceiling(const C& s, const set_search_internal::key<C>& x,
                                    const set_search_internal::key<C>& def = set_search_internal::inf<set_search_internal::key<C>>()) {
  auto it = s.lower_bound(x);
  return it == s.end() ? def : *it;
}
template <class C>
set_search_internal::key<C> higher(const C& s, const set_search_internal::key<C>& x,
                                   const set_search_internal::key<C>& def = set_search_internal::inf<set_search_internal::key<C>>()) {
  auto it = s.upper_bound(x);
  return it == s.end() ? def : *it;
}
template <class C>
set_search_internal::key<C> floor(const C& s, const set_search_internal::key<C>& x,
                                  const set_search_internal::key<C>& def = -set_search_internal::inf<set_search_internal::key<C>>()) {
  auto it = s.upper_bound(x);
  return it == s.begin() ? def : *std::prev(it);
}
template <class C>
set_search_internal::key<C> lower(const C& s, const set_search_internal::key<C>& x,
                                  const set_search_internal::key<C>& def = -set_search_internal::inf<set_search_internal::key<C>>()) {
  auto it = s.lower_bound(x);
  return it == s.begin() ? def : *std::prev(it);
}
