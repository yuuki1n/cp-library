// avl_segtree の verify 用（非可換な op）。Library Checker に提出するコード。
// https://judge.yosupo.jp/problem/point_set_range_composite
//   g++ -std=gnu++20 -O2 -I../.. -I/path/to/ac-library FILE.cpp
//
// op が一次関数の合成なので順序を間違えると落ちる。作用は使わないので
// F 以降を省き、S も avl_value を継承しない（sz が要らないぶん節点が軽い）。
#include <atcoder/modint>
#include <cstdio>
#include <vector>

#include "../../data_structure/avl_segtree.hpp"

using mint = atcoder::modint998244353;

// f(x) = a x + b
struct S {
  mint a = 1, b = 0;
};
// 左を先に通してから右。つまり r(l(x))
S op(S l, S r) { return S{r.a * l.a, r.a * l.b + r.b}; }
S e() { return S{}; }

using tree = avl_segtree<S, op, e>;

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
  std::vector<S> v(n);
  for (int i = 0; i < n; i++) {
    v[i].a = mint::raw((int)read_uint());
    v[i].b = mint::raw((int)read_uint());
  }
  tree t(v);

  std::vector<char> out;
  out.reserve(1 << 22);
  auto put = [&](unsigned x) {
    char tmp[12];
    int k = 0;
    do {
      tmp[k++] = char('0' + x % 10);
      x /= 10;
    } while (x);
    while (k) out.push_back(tmp[--k]);
    out.push_back('\n');
  };

  while (q--) {
    unsigned type = read_uint();
    if (type == 0) {
      int p = (int)read_uint();
      S x;
      x.a = mint::raw((int)read_uint());
      x.b = mint::raw((int)read_uint());
      t.set(p, x);
    } else {
      int l = (int)read_uint(), r = (int)read_uint();
      mint x = mint::raw((int)read_uint());
      S g = t.prod(l, r);
      put((g.a * x + g.b).val());
    }
  }
  fwrite(out.data(), 1, out.size(), stdout);
  return 0;
}
