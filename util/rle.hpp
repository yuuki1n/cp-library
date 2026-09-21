#include <iterator>
#include <type_traits>
#include <utility>
#include <vector>

/*
 * rle(v) : ランレングス圧縮
 *
 *   隣り合う等しい要素をまとめて (値, 連続する個数) の列にする。O(N)。
 *   範囲 for が回る型なら何でも渡せる（string / vector / deque / array /
 *   views など）。空なら空を返す。
 *
 *   rle_decode(r)  圧縮された列を vector<T> に戻す
 *
 *   要素の比較に == を使う。
 *
 * 使用例:
 *   rle(string("aaabbc"));      // {('a',3), ('b',2), ('c',1)}
 *   rle(vi{1, 1, 2, 3, 3, 3});  // {(1,2), (2,1), (3,3)}
 *   fore(t, rle(S)) print(t.fi, t.se);
 */
template <class C> auto rle(const C& v) {
  using T = std::remove_cvref_t<decltype(*std::begin(v))>;
  std::vector<std::pair<T, long long>> r;
  for (const auto& x : v)
    if (!r.empty() && r.back().first == x) r.back().second++;
    else r.emplace_back(x, 1);
  return r;
}

template <class T> std::vector<T> rle_decode(const std::vector<std::pair<T, long long>>& r) {
  std::vector<T> v;
  for (const auto& [x, c] : r)
    for (long long i = 0; i < c; i++) v.push_back(x);
  return v;
}
