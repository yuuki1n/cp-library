// mo の verify 用（add / rem が 2 つで済む形）。
// https://judge.yosupo.jp/problem/static_range_count_distinct
//   g++ -std=gnu++20 -O2 -I../../.. FILE.cpp
//
// 値が 10^9 まで来るので、先に座標圧縮してから数える。
#include <algorithm>
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

  mo m(n);
  for (int i = 0; i < q; i++) {
    int l = (int)read_uint(), r = (int)read_uint();
    m.add_query(l, r);
  }

  std::vector<int> cnt(s.size(), 0);
  long long kind = 0;
  auto ans = m.solve([&](int i) { if (cnt[a[i]]++ == 0) kind++; },
                     [&](int i) { if (--cnt[a[i]] == 0) kind--; },
                     [&] { return kind; });

  out.reserve(1 << 22);
  for (long long x : ans) put(x);
  fwrite(out.data(), 1, out.size(), stdout);
  return 0;
}
