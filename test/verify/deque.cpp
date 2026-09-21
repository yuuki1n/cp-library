// avl_segtree の verify 用（両端の挿入削除と 1 点取得）。Library Checker に提出するコード。
// https://judge.yosupo.jp/problem/deque
//   g++ -std=gnu++20 -O2 -I../.. FILE.cpp
//
// insert(0, x) / insert(size(), x) / erase(0) / erase(size() - 1) / get(i) / size() を突く。
// 端ばかり触るので、平衡が偏らないことの確認にもなる。
#include <cstdio>
#include <vector>

#include "../../data_structure/avl_segtree.hpp"

struct S {
  long long v = 0;
};
S op(S a, S) { return a; }  // 区間積は使わない
S e() { return S{}; }

// 作用も反転も使わないので F 以降は省く
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
  int q = (int)read_uint();
  tree t;  // 空から始める

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
    if (type == 0) t.insert(0, S{read_uint()});
    else if (type == 1) t.insert(t.size(), S{read_uint()});
    else if (type == 2) t.erase(0);
    else if (type == 3) t.erase(t.size() - 1);
    else put(t.get(read_uint()).v);
  }
  fwrite(out.data(), 1, out.size(), stdout);
  return 0;
}
