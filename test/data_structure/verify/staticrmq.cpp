// disjoint_sparse_table の verify 用。Library Checker に提出するコード。
// https://judge.yosupo.jp/problem/staticrmq
//   g++ -std=gnu++20 -O2 -I../../.. FILE.cpp
#include <cstdio>
#include <vector>

#include "../../../data_structure/disjoint_sparse_table.hpp"

unsigned op(unsigned a, unsigned b) { return a < b ? a : b; }

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
  while (c >= '0') {
    x = x * 10 + unsigned(c - '0');
    c = getc_();
  }
  return x;
}
}  // namespace

int main() {
  int n = (int)read_uint(), q = (int)read_uint();
  std::vector<unsigned> a(n);
  for (auto& x : a) x = read_uint();
  disjoint_sparse_table<unsigned, op> d(a);

  std::vector<char> out;
  out.reserve(1 << 23);
  while (q--) {
    int l = (int)read_uint(), r = (int)read_uint();
    unsigned x = d.prod(l, r);
    char tmp[12];
    int k = 0;
    do tmp[k++] = char('0' + x % 10), x /= 10;
    while (x);
    while (k) out.push_back(tmp[--k]);
    out.push_back('\n');
  }
  fwrite(out.data(), 1, out.size(), stdout);
  return 0;
}
