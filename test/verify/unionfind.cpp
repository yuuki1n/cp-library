// union_find の verify 用。Library Checker に提出するコード。
// https://judge.yosupo.jp/problem/unionfind
//   g++ -std=gnu++20 -O2 -I../.. FILE.cpp
#include <cstdio>
#include <vector>

#include "../../graph/union_find/union_find.hpp"

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
}  // namespace

int main() {
  int n = (int)read_uint(), q = (int)read_uint();
  union_find uf(n);
  out.reserve(1 << 19);

  while (q--) {
    unsigned t = read_uint();
    int u = (int)read_uint(), v = (int)read_uint();
    if (t == 0) uf.merge(u, v);
    else out.push_back(char('0' + uf.same(u, v))), out.push_back('\n');
  }
  fwrite(out.data(), 1, out.size(), stdout);
  return 0;
}
