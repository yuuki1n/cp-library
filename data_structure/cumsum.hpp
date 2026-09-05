#include <cstddef>
#include <vector>

/*
 * cumsum<T> : 1 次元累積和
 *
 *   内部は s[i] = 先頭 i 個の和（s[0] = 0）。要素数は s.size() - 1。
 *
 *   構築        O(N)   vector から
 *               O(1)   空 / 長さ指定（容量だけ確保）
 *   push_back   ならし O(1)
 *   区間和      O(1)
 *
 *   要素型が整数なら T をそのまま使うので、int の vector を渡しても
 *   cumsum<long long> なら long long で累積される。
 *
 * 使用例:
 *   cumsum<long long> cs(A);   // vector<int> / vector<long long> から
 *   cumsum<long long> cs(N);   // 長さ N ぶんの容量を確保した空の状態
 *   cumsum<long long> cs;      // 空
 *
 *   cs.push_back(x);           // 末尾に追加
 *   cs += x;                   // 同上
 *
 *   cs(l, r);                  // [l, r) の和
 *   cs(r);                     // [0, r) の和
 *   cs.all_sum();              // 全体の和
 *   cs.at(i);                  // 元の a[i]
 *   cs.size();  cs.empty();  cs.clear();
 *
 * verify:
 *   (未 verify)
 */
template <class T = long long>
struct cumsum {
  std::vector<T> s;  // s[i] = 先頭 i 個の和

  cumsum() { s.push_back(T()); }

  // 長さ n ぶんの容量を確保した空の累積和
  explicit cumsum(std::size_t n) {
    s.reserve(n + 1);
    s.push_back(T());
  }

  template <class U>
  explicit cumsum(const std::vector<U>& a) {
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

  T operator()(long long l, long long r) const {
    return s[r] - s[l];
  }                                                 // [l, r)
  T operator()(long long r) const { return s[r]; }  // [0, r)
  T all_sum() const { return s.back(); }
  T at(long long i) const { return s[i + 1] - s[i]; }  // 元の a[i]
};
