// prime.hpp の primes() の verify 用。
// https://judge.yosupo.jp/problem/enumerate_primes
//   g++ -std=gnu++20 -O2 -I../../.. FILE.cpp
//
// N 以下の素数を全部作り、B 番目から A 個おきに出す。
#include <cstdio>
#include <vector>

#include "../../../math/prime.hpp"

namespace {
std::vector<char> out;
void put(long long x, char sep) {
  char tmp[20];
  int k = 0;
  do tmp[k++] = char('0' + x % 10), x /= 10;
  while (x);
  while (k) out.push_back(tmp[--k]);
  out.push_back(sep);
}
}  // namespace

int main() {
  long long n, a, b;
  if (scanf("%lld %lld %lld", &n, &a, &b) != 3) return 1;

  std::vector<long long> ps = primes(n);
  long long cnt = (long long)ps.size();
  long long x = b < cnt ? (cnt - b + a - 1) / a : 0;  // 取り出す個数

  out.reserve(1 << 24);
  put(cnt, ' '), put(x, '\n');
  for (long long i = 0; i < x; i++) put(ps[b + a * i], i + 1 == x ? '\n' : ' ');
  if (x == 0) out.push_back('\n');
  fwrite(out.data(), 1, out.size(), stdout);
  return 0;
}
