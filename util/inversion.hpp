#pragma once

#include <utility>
#include <vector>

/*
 * inversion_count(a) : 転倒数
 *
 *   マージソートで数える。O(N log N) / 追加メモリ O(N)。
 *   同値は転倒とみなさない（狭義 a[i] > a[j] のみ数える）。
 *
 * 使用例:
 *   long long inv = inversion_count(A);
 *
 *   隣接swapで昇順にする最小回数、バブルソートの交換回数と一致する。
 *
 * verify:
 *   (未 verify)
 */
template <class T>
long long inversion_count(std::vector<T> a) {
  std::size_t n = a.size();
  if (n < 2) return 0;
  std::vector<T> buf(n);
  long long cnt = 0;

  // [l, r) を整列しつつ転倒数を加算する
  auto rec = [&](auto&& self, std::size_t l, std::size_t r) -> void {
    if (r - l <= 1) return;
    std::size_t m = l + (r - l) / 2;
    self(self, l, m);
    self(self, m, r);
    std::size_t i = l, j = m, p = l;
    while (i < m || j < r) {
      if (j >= r || (i < m && !(a[j] < a[i]))) {
        buf[p++] = std::move(a[i++]);
      } else {
        // a[i..m) はすべて a[j] より大きい
        cnt += static_cast<long long>(m - i);
        buf[p++] = std::move(a[j++]);
      }
    }
    for (std::size_t t = l; t < r; t++) a[t] = std::move(buf[t]);
  };
  rec(rec, 0, n);
  return cnt;
}
