// avl_segtree の verify 用（区間反転）。Library Checker に提出するコード。
// https://judge.yosupo.jp/problem/range_reverse_range_sum
//   g++ -std=gnu++20 -O2 -I../.. FILE.cpp
//
// reverse だけを突く。N = 0 や l == r も来るので、空の木と空区間を素通りできること。
#include <cstdio>
#include <vector>

#include "../../data_structure/avl_segtree.hpp"

struct S {
  long long sum = 0;
};
S op(S a, S b) { return S{a.sum + b.sum}; }
S e() { return S{}; }

// 作用は使わない。和は向きに依らないので rev も既定のまま
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
long long read_uint() {
  int c = getc_();
  while (c < '0') c = getc_();
  long long x = 0;
  while (c >= '0') {
    x = x * 10 + (c - '0');
    c = getc_();
  }
  return x;
}
}  // namespace

int main() {
  int n = (int)read_uint(), q = (int)read_uint();
  std::vector<S> v(n);
  for (int i = 0; i < n; i++) v[i].sum = read_uint();
  tree t(v);

  std::vector<char> out;
  out.reserve(1 << 22);
  auto put = [&](long long x) {
    char tmp[20];
    int k = 0;
    do {
      tmp[k++] = char('0' + x % 10);
      x /= 10;
    } while (x);
    while (k) out.push_back(tmp[--k]);
    out.push_back('\n');
  };

  while (q--) {
    long long type = read_uint();
    long long l = read_uint(), r = read_uint();
    if (type == 0) t.reverse(l, r);
    else put(t.prod(l, r).sum);
  }
  fwrite(out.data(), 1, out.size(), stdout);
  return 0;
}
