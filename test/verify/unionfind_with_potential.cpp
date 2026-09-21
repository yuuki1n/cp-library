// relational_union_find の verify 用（mod 998244353 の加法群）。
// https://judge.yosupo.jp/problem/unionfind_with_potential
//   g++ -std=gnu++20 -O2 -I../.. FILE.cpp
//
// ライブラリの diff(u, v) は「v の値 - u の値」なので、
// 問題の a_u = a_v + x は diff(v, u) == x にあたる。
#include <cstdio>
#include <vector>

#include "../../graph/union_find/relational_union_find.hpp"

using F = unsigned;  // mod 998244353 の加法群
constexpr F MOD = 998244353;
F op_add(F a, F b) { return a + b < MOD ? a + b : a + b - MOD; }
F e_zero() { return 0; }
F inv_neg(F a) { return a ? MOD - a : 0; }

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
  if (x < 0) out.push_back('-'), x = -x;
  char tmp[12];
  int k = 0;
  do tmp[k++] = char('0' + x % 10), x /= 10;
  while (x);
  while (k) out.push_back(tmp[--k]);
  out.push_back('\n');
}
}  // namespace

int main() {
  int n = (int)read_uint(), q = (int)read_uint();
  relational_union_find<F, op_add, e_zero, inv_neg> uf(n);
  out.reserve(1 << 21);

  while (q--) {
    unsigned t = read_uint();
    int u = (int)read_uint(), v = (int)read_uint();
    if (t == 0) {
      F x = read_uint();
      put(uf.merge(v, u, x) ? 1 : 0);
    } else {
      put(uf.same(u, v) ? (long long)uf.diff(v, u) : -1);
    }
  }
  fwrite(out.data(), 1, out.size(), stdout);
  return 0;
}
