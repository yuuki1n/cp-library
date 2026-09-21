// relational_union_find の verify 用（非可換な群）。Library Checker に提出するコード。
// https://judge.yosupo.jp/problem/unionfind_with_potential_non_commutative_group
//   g++ -std=gnu++20 -O2 -I../.. -I/path/to/ac-library FILE.cpp
//
// ポテンシャルが 2x2 行列なので op が非可換。合成順を間違えると落ちる。
#include <array>
#include <atcoder/modint>
#include <cstdio>
#include <utility>
#include <vector>

#include "../../../../graph/union_find/relational_union_find.hpp"

using mint = atcoder::modint998244353;
using M = std::array<std::array<mint, 2>, 2>;

M op(M a, M b) {
  M c{};
  for (int i : {0, 1})
    for (int k : {0, 1})
      for (int j : {0, 1}) c[i][j] += a[i][k] * b[k][j];
  return c;
}
M e() {
  M m{};
  m[0][0] = m[1][1] = 1;
  return m;
}
// 入力は det = 1 が保証されており、積も逆行列も det = 1 のまま。
// よって逆行列は対角を入れ替えて非対角の符号を反転するだけでよい
M inv(M a) {
  std::swap(a[0][0], a[1][1]);
  a[0][1] = -a[0][1], a[1][0] = -a[1][0];
  return a;
}

using ruf = relational_union_find<M, op, e, inv>;

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
void put(long long x, char tail) {
  if (x < 0) out.push_back('-'), x = -x;
  char tmp[12];
  int k = 0;
  do tmp[k++] = char('0' + x % 10), x /= 10;
  while (x);
  while (k) out.push_back(tmp[--k]);
  out.push_back(tail);
}
}  // namespace

int main() {
  int n = (int)read_uint(), q = (int)read_uint();
  ruf uf(n);
  out.reserve(1 << 22);

  while (q--) {
    unsigned t = read_uint();
    int u = (int)read_uint(), v = (int)read_uint();
    if (t == 0) {
      M x;
      for (int i : {0, 1})
        for (int j : {0, 1}) x[i][j] = mint::raw((int)read_uint());
      // a_u = a_v x すなわち a_v^{-1} a_u = x。ライブラリの diff(v, u) がそれ
      put(uf.merge(v, u, x) ? 1 : 0, '\n');
    } else if (!uf.same(u, v)) {
      put(-1, '\n');
    } else {
      M m = uf.diff(v, u);
      put(m[0][0].val(), ' '), put(m[0][1].val(), ' ');
      put(m[1][0].val(), ' '), put(m[1][1].val(), '\n');
    }
  }
  fwrite(out.data(), 1, out.size(), stdout);
  return 0;
}
