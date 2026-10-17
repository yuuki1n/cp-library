#include <cassert>
#include <string>
#include <type_traits>
#include <vector>

// https://github.com/yuuki1n/cp-library/blob/main/util/digit_dp.hpp

/*
 * digit_dp : 上限以下の整数を、桁を左から読む状態機械として数える
 *
 *   digit_dp<T>(n, m, s0, nxt, add, base)   [0, n]。add は既定 +=、base は既定 10
 *   digit_dp<T>(l, r, m, s0, nxt, ...)      [l, r]。f(r) - f(l-1) なので T に -= が要る
 *
 *   n / l / r は桁を並べた文字列。返るのは長さ m の列で res[s] = 最後に状態 s にいた合計。
 *
 *   【状態】 0 .. m-1 の整数で、s0 が「まだ数が始まっていない」。
 *   先頭のゼロの間は nxt も add も呼ばれず、状態は s0 のまま進む。
 *   つまり nxt が呼ばれるのは必ず実際の桁で、その数の 1 桁目なら first が立つ。
 *   数 0 は最後まで始まらないので res[s0] に 1 個ぶん残る。
 *
 *   nxt(s, d, first, k)         状態 s で桁 d を置いた次の状態。-1 なら置けない
 *   add(dst, src, s, d, first, k)  dst に src を足し込む。総和が要るときだけ書く
 *
 *   nxt は引数を後ろから削ってよい（(s, d) / (s, d, first) / (s, d, first, k)）。
 *   first なら s は s0 で、直前の桁は無い。k は右から数えた位置で d の重みは base^k。
 *   T は T() が零、T(1) が 1 通り、+= で合流できること。計算量 O(桁数 * m * base)。
 *
 *   k は上限の桁数が基準。n = 54321 の 5 は 00005 と見るので k = 0 になる。
 *
 * 使用例:
 *   // N 以下で 3 の倍数の個数。状態は桁和 mod 3。s0 = 0 は数 0 とも整合する
 *   auto r = digit_dp<mint>(S, 3, 0, [](int s, int d) { return (s + d) % 3; });
 *   print(r[0]);
 *
 *   // N 以下で隣り合う桁が同じでないものの個数。状態は直前の桁
 *   auto r = digit_dp<mint>(S, 10, 0, [](int s, int d, bool first) {
 *     return first ? d : (s == d ? -1 : d);
 *   });
 *
 *   // 隣り合う桁の差の総和。直前の桁が要るので add で s を見る
 *   struct V {
 *     mint cnt, sum;
 *     V() = default;
 *     explicit V(int o) : cnt(o) {}
 *     V& operator+=(const V& o) { return cnt += o.cnt, sum += o.sum, *this; }
 *   };
 *   auto r = digit_dp<V>(S, 10, 0, [](int, int d) { return d; },
 *                        [](V& a, const V& b, int s, int d, bool first, int) {
 *                          a.cnt += b.cnt, a.sum += b.sum;
 *                          if (!first) a.sum += b.cnt * abs(s - d);
 *                        });
 */
namespace digit_dp_internal {
// 既定の足し込み。桁は見ずにただ合流する
struct plus_ {
  template <class T> void operator()(T& a, const T& b, int, int, bool, int) const { a += b; }
};

template <class F> int call(F& f, int s, int d, bool first, int k) {
  if constexpr (std::is_invocable_v<F, int, int, bool, int>) return f(s, d, first, k);
  else if constexpr (std::is_invocable_v<F, int, int, bool>) return f(s, d, first);
  else return f(s, d);
}

// 文字列を 1 減らす。桁数は変えない（"100" -> "099"）。"0" は渡さないこと
inline std::string dec(std::string s, int base) {
  int i = (int)s.size() - 1;
  while (i >= 0 && s[i] == '0') s[i--] = char('0' + base - 1);
  assert(i >= 0 && "0 より小さくはできない");
  s[i]--;
  return s;
}
}  // namespace digit_dp_internal

template <class T = long long, class F, class G = digit_dp_internal::plus_>
std::vector<T> digit_dp(const std::string& n, int m, int s0, F nxt, G add = {}, int base = 10) {
  assert(2 <= base && base <= 10);
  assert(0 <= s0 && s0 < m);
  int len = (int)n.size();
  // cur[tight][lead][s]。tight は上限に張り付いているか、lead はまだ先頭のゼロか
  std::vector<std::vector<T>> cur(4, std::vector<T>(m)), nx(4, std::vector<T>(m));
  cur[3][s0] = T(1);  // 3 = tight かつ lead
  for (int i = 0; i < len; i++) {
    int t = n[i] - '0', k = len - 1 - i;
    assert(0 <= t && t < base && "桁が基数に収まっていない");
    for (auto& v : nx) std::fill(v.begin(), v.end(), T());
    for (int b = 0; b < 4; b++) {
      bool tight = b & 2, lead = b & 1;
      for (int s = 0; s < m; s++) {
        const T& v = cur[b][s];
        int hi = tight ? t : base - 1;
        for (int d = 0; d <= hi; d++) {
          int nt = tight && d == t ? 2 : 0;
          // 先頭のゼロの間は数がまだ始まっていない。状態も値もそのまま運ぶ
          if (lead && d == 0) {
            nx[nt | 1][s] += v;
            continue;
          }
          // ここまで来たら d は実際の桁。lead が立っていればこの数の 1 桁目
          int ns = digit_dp_internal::call(nxt, s, d, lead, k);
          if (ns < 0) continue;
          add(nx[nt][ns], v, s, d, lead, k);
        }
      }
    }
    cur.swap(nx);
  }
  std::vector<T> res(m);
  for (int b = 0; b < 4; b++)
    for (int s = 0; s < m; s++) res[s] += cur[b][s];
  return res;
}

// [l, r] を数える。f(r) - f(l-1) なので T に -= が要る
template <class T = long long, class F, class G = digit_dp_internal::plus_>
std::vector<T> digit_dp(const std::string& l, const std::string& r, int m, int s0, F nxt, G add = {}, int base = 10) {
  assert(l.size() <= r.size() && "l <= r であること");
  std::vector<T> res = digit_dp<T>(r, m, s0, nxt, add, base);
  if (l == "0") return res;
  std::vector<T> sub = digit_dp<T>(digit_dp_internal::dec(l, base), m, s0, nxt, add, base);
  for (int s = 0; s < m; s++) res[s] -= sub[s];
  return res;
}
