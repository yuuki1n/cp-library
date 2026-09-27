// hld の verify 用（非可換な演算をパス上で合成する）。
// https://judge.yosupo.jp/problem/vertex_set_path_composite
//   g++ -std=gnu++20 -O2 -I../../../.. -I/path/to/ac-library FILE.cpp
//
// 1 次関数の合成は非可換なので、path が返す fwd を見て引くセグメント木を変える。
// 登る向き（fwd が false）の区間は、op の引数を入れ替えたセグメント木から取る。
#include <atcoder/modint>
#include <atcoder/segtree>
#include <cstdio>
#include <vector>

#include "../../../../graph/tree/hld.hpp"

using mint = atcoder::modint998244353;

// f(x) = a x + b
struct F {
  mint a = 1, b = 0;
};
// f を通してから g
F op(F f, F g) { return F{f.a * g.a, g.a * f.b + g.b}; }
F op_rev(F f, F g) { return op(g, f); }
F e() { return F{}; }

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
void put(unsigned x) {
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
  std::vector<F> f(n);
  for (int i = 0; i < n; i++) f[i].a = mint::raw((int)read_uint()), f[i].b = mint::raw((int)read_uint());

  hld t(n);
  for (int i = 0; i < n - 1; i++) {
    int u = (int)read_uint(), v = (int)read_uint();
    t.add_edge(u, v);
  }
  t.build();

  // 同じ値を 2 本に入れる。rev は引数を入れ替えた op を持つ
  atcoder::segtree<F, op, e> seg(n);
  atcoder::segtree<F, op_rev, e> rev(n);
  for (int i = 0; i < n; i++) seg.set(t.in(i), f[i]), rev.set(t.in(i), f[i]);

  out.reserve(1 << 21);
  while (q--) {
    unsigned ty = read_uint();
    if (ty == 0) {
      int p = (int)read_uint();
      F g;
      g.a = mint::raw((int)read_uint()), g.b = mint::raw((int)read_uint());
      seg.set(t.in(p), g), rev.set(t.in(p), g);
    } else {
      int u = (int)read_uint(), v = (int)read_uint();
      mint x = mint::raw((int)read_uint());
      F res = e();
      for (auto p : t.path(u, v)) res = op(res, p.fwd ? seg.prod(p.l, p.r) : rev.prod(p.l, p.r));
      put((res.a * x + res.b).val());
    }
  }
  fwrite(out.data(), 1, out.size(), stdout);
  return 0;
}
