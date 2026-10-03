// tree_mo の verify 用。
// https://atcoder.jp/contests/abc477/tasks/abc477_g
//   g++ -std=gnu++20 -O2 -I../../../.. FILE.cpp
//
// s-t パス上で、現れる回数が a 回以上 b 回以下の値の種類数を数える。
// 頻度ごとの個数 fcnt[f] を持ち、その区間和を平方分割で取る。
// BIT にすると更新が O(log N) になり、Mo の O((2N+Q)sqrt(Q)) 回の
// 更新に log が乗って間に合わない。更新 O(1) / 取得 O(sqrt(N)) に寄せる。
#include <cmath>
#include <cstdio>
#include <vector>

#include "../../../../graph/tree/hld.hpp"
#include "../../../../util/mo.hpp"

#include "../../../../graph/tree/tree_mo.hpp"

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
  std::vector<int> x(n);
  for (int i = 0; i < n; i++) x[i] = (int)read_uint() - 1;  // 0 .. n-1

  tree_mo tm(n);
  for (int i = 0; i < n - 1; i++) {
    int u = (int)read_uint() - 1, v = (int)read_uint() - 1;
    tm.add_edge(u, v);
  }
  tm.build();

  std::vector<int> qa(q), qb(q);
  for (int i = 0; i < q; i++) {
    int s = (int)read_uint() - 1, t = (int)read_uint() - 1;
    qa[i] = (int)read_uint(), qb[i] = (int)read_uint();
    tm.add_query(s, t);
  }

  // fcnt[f] = ちょうど f 回現れる値の個数。blk は幅 B ごとの和
  int B = (int)std::sqrt((double)n) + 1;
  std::vector<int> cnt(n, 0), fcnt(n + 2, 0), blk(n / B + 2, 0);

  auto add = [&](int v) {
    int c = cnt[x[v]]++;
    if (c) fcnt[c]--, blk[c / B]--;
    fcnt[c + 1]++, blk[(c + 1) / B]++;
  };
  auto rem = [&](int v) {
    int c = cnt[x[v]]--;
    fcnt[c]--, blk[c / B]--;
    if (c > 1) fcnt[c - 1]++, blk[(c - 1) / B]++;
  };
  auto get = [&](int i) {
    int a = qa[i], b = qb[i];
    if (b > n) b = n;
    long long s = 0;
    int ba = a / B, bb = b / B;
    if (ba == bb) {
      for (int f = a; f <= b; f++) s += fcnt[f];
    } else {
      for (int f = a; f < (ba + 1) * B; f++) s += fcnt[f];
      for (int k = ba + 1; k < bb; k++) s += blk[k];
      for (int f = bb * B; f <= b; f++) s += fcnt[f];
    }
    return s;
  };

  auto ans = tm.solve(add, rem, get);
  out.reserve(1 << 22);
  for (long long v : ans) put(v);
  fwrite(out.data(), 1, out.size(), stdout);
  return 0;
}
