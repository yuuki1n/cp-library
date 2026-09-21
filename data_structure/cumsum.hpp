#include <cstddef>
#include <vector>

/*
 * cumsum<T> : 1 次元累積和
 *
 *   cumsum(v)        vector から構築           O(N)
 *   cumsum(n)        長さ n ぶん確保した空     O(1)
 *   cumsum()         空
 *   push_back(x)     末尾に追加。cs += x も可  ならし O(1)
 *   cs(l, r)         [l, r) の和               O(1)
 *   cs(r)            [0, r) の和
 *   all_sum() / at(i) / size() / empty() / clear() / reserve(n)
 *
 *   累積は T で行う。vector<int> を渡しても cumsum<long long> なら
 *   long long で足す。範囲は検査しない。
 *
 * 使用例:
 *   cumsum<long long> cs(A);
 *   cs(l, r);
 */
template <class T = long long> struct cumsum {
  std::vector<T> s;  // s[i] = 先頭 i 個の和

  cumsum() { s.push_back(T()); }

  // 長さ n ぶんの容量を確保した空の累積和
  explicit cumsum(std::size_t n) {
    s.reserve(n + 1);
    s.push_back(T());
  }

  template <class U> explicit cumsum(const std::vector<U>& a) {
    s.reserve(a.size() + 1);
    s.push_back(T());
    for (const U& x : a) s.push_back(s.back() + T(x));
  }

  // 末尾に 1 要素追加する（vector の再確保に任せるのでならし O(1)）
  void push_back(const T& x) { s.push_back(s.back() + x); }
  cumsum& operator+=(const T& x) {
    push_back(x);
    return *this;
  }

  std::size_t size() const { return s.size() - 1; }
  bool empty() const { return s.size() == 1; }
  void clear() { s.assign(1, T()); }
  void reserve(std::size_t n) { s.reserve(n + 1); }

  T operator()(long long l, long long r) const { return s[r] - s[l]; }  // [l, r)
  T operator()(long long r) const { return s[r]; }                      // [0, r)
  T all_sum() const { return s.back(); }
  T at(long long i) const { return s[i + 1] - s[i]; }  // 元の a[i]
};
