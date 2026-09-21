// avl_segtree の verify 用。Library Checker に提出するコード。
// https://judge.yosupo.jp/problem/dynamic_sequence_range_affine_range_sum
//   g++ -std=gnu++20 -O2 -I../.. -I/path/to/ac-library FILE.cpp
#include <atcoder/modint>
#include <cstdio>
#include <vector>

#include "../../../data_structure/avl_segtree.hpp"

using mint = atcoder::modint998244353;

struct S : avl_value {
  mint sum = 0;
};
S op(S a, S b) {
  S r;
  r.sum = a.sum + b.sum;
  return r;
}
S e() { return S{}; }

// x -> b x + c
struct F {
  mint b = 1, c = 0;
};
S mapping(F f, S x) {
  x.sum = f.b * x.sum + f.c * mint(x.sz);
  return x;
}
// f(g(x)) = b1 (b2 x + c2) + c1
F composition(F f, F g) { return F{f.b * g.b, f.b * g.c + f.c}; }
F id() { return F{}; }

// 和は向きに依らないので rev は既定のまま
using tree = avl_segtree<S, op, e, F, mapping, composition, id>;

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
  std::vector<S> a(n);
  for (int i = 0; i < n; i++) a[i].sum = mint::raw((int)read_uint());
  tree t(a);

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
      int i = (int)read_uint();
      S x;
      x.sum = mint::raw((int)read_uint());
      t.insert(i, x);
    } else if (type == 1) {
      t.erase((int)read_uint());
    } else if (type == 2) {
      int l = (int)read_uint(), r = (int)read_uint();
      t.reverse(l, r);
    } else if (type == 3) {
      int l = (int)read_uint(), r = (int)read_uint();
      F f;
      f.b = mint::raw((int)read_uint());
      f.c = mint::raw((int)read_uint());
      t.apply(l, r, f);
    } else {
      int l = (int)read_uint(), r = (int)read_uint();
      put(t.prod(l, r).sum.val());
    }
  }
  fwrite(out.data(), 1, out.size(), stdout);
  return 0;
}
