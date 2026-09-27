// mo の verify 用（左右を区別する形）。
// https://judge.yosupo.jp/problem/static_range_inversions_query
//   g++ -std=gnu++20 -O2 -I../../.. -I/path/to/ac-library FILE.cpp
//
// 転倒数は「どちら側から入れたか」で数えるものが変わる。
//   左に a[i] を足す -> 今ある中で a[i] より小さいものの個数だけ増える
//   右に a[i] を足す -> 今ある中で a[i] より大きいものの個数だけ増える
// 外すときはその逆を引く。個数は BIT で数える。
#include <algorithm>
#include <atcoder/fenwicktree>
#include <cstdio>
#include <vector>

#include "../../../util/mo.hpp"

namespace {
char buf[1 << 16];
int bl = 0, br = 0;
int getc_() {
  if (bl == br) {
    br = (int)fread(buf, 1, sizeof(buf), stdin);
    bl = 0;
    if (br <= 0) return -1;
  }
  return buf[bl++];
}
unsigned read_uint() {
  int c = getc_();
  while (c < '0') c = getc_();
  unsigned x = 0;
  while (c >= '0') x = x * 10 + unsigned(c - '0'), c = getc_();
  return x;
}
std::vector<char> out;
void put(long long x) {
  char tmp[20];
  int k = 0;
  do tmp[k++] = char('0' + x % 10), x /= 10;
  while (x);
  while (k) out.push_back(tmp[--k]);
  out.push_back('\n');
}
}  // namespace

int main() {
  int n = (int)read_uint(), q = (int)read_uint();
  std::vector<int> a(n);
  for (int i = 0; i < n; i++) a[i] = (int)read_uint();

  std::vector<int> s = a;
  std::sort(s.begin(), s.end());
  s.erase(std::unique(s.begin(), s.end()), s.end());
  for (int i = 0; i < n; i++) a[i] = int(std::lower_bound(s.begin(), s.end(), a[i]) - s.begin());
  int k = (int)s.size();

  mo m(n);
  for (int i = 0; i < q; i++) {
    int l = (int)read_uint(), r = (int)read_uint();
    m.add_query(l, r);
  }

  atcoder::fenwick_tree<int> bit(k);
  long long inv = 0;
  auto ans = m.solve([&](int i) { inv += bit.sum(0, a[i]); bit.add(a[i], 1); },              // 左を足す
                     [&](int i) { inv += bit.sum(a[i] + 1, k); bit.add(a[i], 1); },          // 右を足す
                     [&](int i) { bit.add(a[i], -1); inv -= bit.sum(0, a[i]); },             // 左を外す
                     [&](int i) { bit.add(a[i], -1); inv -= bit.sum(a[i] + 1, k); },         // 右を外す
                     [&] { return inv; });

  out.reserve(1 << 22);
  for (long long x : ans) put(x);
  fwrite(out.data(), 1, out.size(), stdout);
  return 0;
}
