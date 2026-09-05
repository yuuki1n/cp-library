#include <cstddef>
#include <vector>

/*
 * cumsum2d<T> : 2 次元累積和
 *
 *   構築    O(HW)
 *   矩形和  O(1)
 *
 * 使用例:
 *   cumsum2d<long long> cs(G);        // vector<vector<int>> など
 *   cs(i1, j1, i2, j2);               // [i1, i2) x [j1, j2) の和
 *   cs(i2, j2);                       // [0, i2) x [0, j2) の和
 *   cs.all_sum();
 *   cs.h();  cs.w();
 *
 * verify:
 *   (未 verify)
 */
template <class T = long long>
struct cumsum2d {
  std::vector<std::vector<T>> s;  // s[i][j] = 左上 i x j の和

  cumsum2d() : s(1, std::vector<T>(1, T())) {}

  template <class U>
  explicit cumsum2d(const std::vector<std::vector<U>>& a) {
    std::size_t h = a.size(), w = h == 0 ? 0 : a[0].size();
    s.assign(h + 1, std::vector<T>(w + 1, T()));
    for (std::size_t i = 0; i < h; i++)
      for (std::size_t j = 0; j < w; j++)
        s[i + 1][j + 1] = s[i][j + 1] + s[i + 1][j] - s[i][j] + T(a[i][j]);
  }

  std::size_t h() const { return s.size() - 1; }
  std::size_t w() const { return s[0].size() - 1; }

  // [i1, i2) x [j1, j2)
  T operator()(long long i1, long long j1, long long i2, long long j2) const {
    return s[i2][j2] - s[i1][j2] - s[i2][j1] + s[i1][j1];
  }
  // [0, i2) x [0, j2)
  T operator()(long long i2, long long j2) const { return s[i2][j2]; }
  T all_sum() const { return s.back().back(); }
};
