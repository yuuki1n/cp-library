#include <algorithm>
#include <cstddef>
#include <numeric>
#include <utility>
#include <vector>

/*
 * 素数判定・素因数分解・約数列挙
 *
 *   n が小さいときは SPF（最小素因数）テーブル、大きいときは
 *   Miller-Rabin + Pollard の ρ に切り替える。
 *
 *     n <= SPF_MAX (10^6)   SPF テーブルを引く      O(log n)
 *     n >  SPF_MAX          Pollard の ρ            O(n^(1/4)) 程度
 *
 *   テーブルは初回に必要になった時点で構築してキャッシュする。使わなければ
 *   確保もされない。サイズは 2 の冪で必要なぶんだけ拡張する（上限 SPF_MAX、
 *   全体で約 2 MB / 構築 1 ms）。Pollard が分割した部分因数も同じ判定を通る
 *   ので、10^18 の数が 10^6 以下に割れた時点で即座に解決される。
 *
 *   is_prime(n)   決定的 Miller-Rabin。n < 2^64 で確実
 *   factorize(n)  vector<pair<long long, int>> を素数の昇順で返す。n <= 1 は空
 *   divisors(n)   約数を昇順で全列挙。n <= 0 は空。n = 1 なら {1}
 *
 *   __int128 を使うので 64bit 環境専用。スレッドセーフではない。
 *
 * 使用例:
 *   for (auto [p, e] : factorize(360)) print(p, e);   // 2 3 / 3 2 / 5 1
 *   auto ds = divisors(360);                          // 1 2 3 4 5 6 8 ...
 *   if (is_prime(1000000007)) ...
 *
 * verify:
 *   (未 verify)
 */

namespace prime_internal {

// これ以下は SPF テーブル、超えたら Pollard の ρ に切り替える
constexpr long long SPF_MAX = 1000000;

inline unsigned long long mul_mod(unsigned long long a, unsigned long long b,
                                  unsigned long long m) {
  return (unsigned long long)((__uint128_t)a * b % m);
}

inline unsigned long long pow_mod(unsigned long long a, unsigned long long e,
                                  unsigned long long m) {
  unsigned long long r = 1 % m;
  a %= m;
  while (e) {
    if (e & 1) r = mul_mod(r, a, m);
    a = mul_mod(a, a, m);
    e >>= 1;
  }
  return r;
}

// 奇数 x (<= n) の最小素因数を t[x >> 1] で引けるテーブルを返す。
// 構築済みの上限が n に満たなければ 2 の冪で拡張する。
//
// 前提: n <= SPF_MAX。これを破ると n をカバーしないテーブルが返る
// （上限まで構築済みなら再構築はしないので、無駄には走らない）。
inline const std::vector<int>& spf_table(long long n) {
  static std::vector<int> t;
  static long long lim = 0;  // 構築済みの上限
  if (n > lim && lim < SPF_MAX) {
    long long nl = lim ? lim * 2 : 1024;
    while (nl < n && nl < SPF_MAX) nl *= 2;  // 上限を条件に含めて溢れを防ぐ
    nl = std::min(nl, SPF_MAX);
    t.assign(std::size_t(nl / 2 + 1), 0);
    for (std::size_t i = 0; i < t.size(); i++) t[i] = int(i * 2 + 1);
    for (long long p = 3; p * p <= nl; p += 2)
      if (t[std::size_t(p >> 1)] == p)
        for (long long q = p * p; q <= nl; q += p * 2)
          if (t[std::size_t(q >> 1)] == q) t[std::size_t(q >> 1)] = int(p);
    lim = nl;
  }
  return t;
}

}  // namespace prime_internal

// 素数判定。SPF_MAX 以下はテーブル、超えたら決定的 Miller-Rabin。
inline bool is_prime(long long n_) {
  if (n_ < 2) return false;
  if (!(n_ & 1)) return n_ == 2;
  if (n_ <= prime_internal::SPF_MAX)
    return prime_internal::spf_table(n_)[std::size_t(n_ >> 1)] == n_;

  unsigned long long n = (unsigned long long)n_;
  for (unsigned long long p : {3ULL, 5ULL, 7ULL, 11ULL, 13ULL, 17ULL, 19ULL,
                               23ULL, 29ULL, 31ULL, 37ULL}) {
    if (n % p == 0) return n == p;
  }
  unsigned long long d = n - 1;
  int s = 0;
  while (!(d & 1)) d >>= 1, s++;
  for (unsigned long long a : {2ULL, 325ULL, 9375ULL, 28178ULL, 450775ULL,
                               9780504ULL, 1795265022ULL}) {
    if (a % n == 0) continue;
    unsigned long long x = prime_internal::pow_mod(a % n, d, n);
    if (x == 1 || x == n - 1) continue;
    bool witness = true;
    for (int i = 1; i < s; i++) {
      x = prime_internal::mul_mod(x, x, n);
      if (x == n - 1) {
        witness = false;
        break;
      }
    }
    if (witness) return false;
  }
  return true;
}

namespace prime_internal {

// 合成数 n の 1 < d < n なる約数を 1 つ返す（Brent 版 Pollard の ρ）
inline unsigned long long pollard(unsigned long long n) {
  if (!(n & 1)) return 2;
  for (unsigned long long c = 1;; c++) {
    auto f = [&](unsigned long long x) { return (mul_mod(x, x, n) + c) % n; };
    unsigned long long x = 0, y = 0, ys = 0, q = 1, g = 1;
    long long m = 128, r = 1;
    while (g == 1) {
      x = y;
      for (long long i = 0; i < r; i++) y = f(y);
      for (long long k = 0; k < r && g == 1; k += m) {
        ys = y;
        long long lim = std::min(m, r - k);
        for (long long i = 0; i < lim; i++) {
          y = f(y);
          q = mul_mod(q, x > y ? x - y : y - x, n);
        }
        g = std::gcd(q, n);
      }
      r <<= 1;
    }
    if (g == n) {
      g = 1;
      y = ys;
      while (g == 1) {
        y = f(y);
        g = std::gcd(x > y ? x - y : y - x, n);
      }
    }
    if (g != n) return g;
  }
}

}  // namespace prime_internal

// 素因数分解。素数の昇順に (素数, 指数) を返す。
inline std::vector<std::pair<long long, int>> factorize(long long n) {
  std::vector<std::pair<long long, int>> res;
  if (n <= 1) return res;

  // SPF テーブルだけで完結する場合。小さい素数から順に出るのでソート不要
  if (n <= prime_internal::SPF_MAX) {
    const std::vector<int>& t = prime_internal::spf_table(n);
    if (!(n & 1)) {
      int e = 0;
      while (!(n & 1)) n >>= 1, e++;
      res.emplace_back(2, e);
    }
    while (n > 1) {
      long long p = t[std::size_t(n >> 1)];
      int e = 0;
      while (n % p == 0) n /= p, e++;
      res.emplace_back(p, e);
    }
    return res;
  }

  std::vector<long long> ps;
  // 小さい素数を剥がしておく（Pollard の呼び出しを減らす）
  for (long long p : {2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37}) {
    if (p * p > n) break;
    while (n % p == 0) n /= p, ps.push_back(p);
  }

  std::vector<long long> stk;
  if (n > 1) stk.push_back(n);
  while (!stk.empty()) {
    long long m = stk.back();
    stk.pop_back();
    if (m == 1) continue;
    if (m <= prime_internal::SPF_MAX) {  // SPF テーブルで一気に分解
      const std::vector<int>& t = prime_internal::spf_table(m);
      while (m > 1) {
        if (!(m & 1)) {
          ps.push_back(2);
          m >>= 1;
          continue;
        }
        long long p = t[std::size_t(m >> 1)];
        ps.push_back(p);
        m /= p;
      }
      continue;
    }
    if (is_prime(m)) {
      ps.push_back(m);
      continue;
    }
    long long d = (long long)prime_internal::pollard((unsigned long long)m);
    stk.push_back(d);
    stk.push_back(m / d);
  }

  std::sort(ps.begin(), ps.end());
  for (std::size_t i = 0; i < ps.size();) {
    std::size_t j = i;
    while (j < ps.size() && ps[j] == ps[i]) j++;
    res.emplace_back(ps[i], int(j - i));
    i = j;
  }
  return res;
}

// 約数を昇順で全列挙する。
inline std::vector<long long> divisors(long long n) {
  if (n <= 0) return {};
  std::vector<long long> res{1};
  for (const auto& pe : factorize(n)) {
    std::size_t sz = res.size();
    long long pk = 1;
    for (int i = 0; i < pe.second; i++) {
      pk *= pe.first;
      for (std::size_t j = 0; j < sz; j++) res.push_back(res[j] * pk);
    }
  }
  std::sort(res.begin(), res.end());
  return res;
}
