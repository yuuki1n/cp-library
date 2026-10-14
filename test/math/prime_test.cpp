// math/prime.hpp の検証。総当たり・篩と突き合わせる。
//   g++ -std=gnu++20 -O2 -Wall -Wextra -I.. prime_test.cpp -o prime_test
#include "../../math/prime.hpp"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <random>
#include <utility>
#include <vector>

using ll = long long;
using namespace std;

int failed = 0;
void check(bool ok, const char* what) {
  if (!ok) {
    printf("  NG: %s\n", what);
    failed++;
  }
}

int main() {
  mt19937_64 rng(20260901);

  // ---- エラトステネスの篩と is_prime を照合 ----
  const int LIM = 2000000;
  vector<char> sieve(LIM + 1, 1);
  sieve[0] = sieve[1] = 0;
  for (int i = 2; (ll)i * i <= LIM; i++)
    if (sieve[i])
      for (int j = i * i; j <= LIM; j += i) sieve[j] = 0;
  {
    int bad = 0;
    for (int i = 0; i <= LIM; i++)
      if (is_prime(i) != (bool)sieve[i]) bad++;
    printf("is_prime  篩と照合 (0..2e6)   : %s\n", bad == 0 ? "OK" : "NG");
    failed += bad ? 1 : 0;
  }

  // ---- 既知の大きい素数 / 合成数 ----
  {
    for (ll p : {1000000007LL, 998244353LL, 1000000009LL, 2147483647LL, 999999999989LL, 1000000000000000003LL, 9223372036854775783LL})
      check(is_prime(p), "既知の素数を素数と判定できない");
    for (ll c : {1000000007LL * 2, 999999999989LL * 3, 4LL, 561LL, 1105LL, 1729LL, 2465LL, 3215031751LL})
      check(!is_prime(c), "合成数を素数と判定した");
    check(!is_prime(0) && !is_prime(1) && !is_prime(-7), "0/1/負数");
    printf("is_prime  既知の値 / 境界     : OK\n");
  }

  // ---- factorize を試し割りと照合 ----
  {
    int bad = 0;
    for (int t = 0; t < 30000; t++) {
      ll n = ll(rng() % 1000000) + 1;
      auto f = factorize(n);
      // 積が n に戻るか
      ll prod = 1;
      for (auto& pe : f)
        for (int i = 0; i < pe.second; i++) prod *= pe.first;
      if (n == 1 ? !f.empty() : prod != n) bad++;
      // 素数の昇順で、各要素が素数か
      for (size_t i = 0; i < f.size(); i++) {
        if (f[i].first <= LIM && !sieve[f[i].first]) bad++;
        if (f[i].second <= 0) bad++;
        if (i && f[i - 1].first >= f[i].first) bad++;
      }
    }
    printf("factorize 積 / 昇順 / 素数性  : %s\n", bad == 0 ? "OK" : "NG");
    failed += bad ? 1 : 0;
  }

  // ---- factorize の既知の値 ----
  {
    auto f = factorize(360);  // 2^3 * 3^2 * 5
    check(f.size() == 3 && f[0] == make_pair(2LL, 3) && f[1] == make_pair(3LL, 2) && f[2] == make_pair(5LL, 1), "factorize(360)");
    check(factorize(1).empty(), "factorize(1) は空");
    check(factorize(0).empty(), "factorize(0) は空");
    check(factorize(-5).empty(), "factorize(負数) は空");
    auto g = factorize(1000000007LL);
    check(g.size() == 1 && g[0] == make_pair(1000000007LL, 1), "大きい素数");
    ll big = 999999937LL * 999999937LL;  // 素数の 2 乗（約 10^18）
    auto h = factorize(big);
    check(h.size() == 1 && h[0] == make_pair(999999937LL, 2), "大素数の 2 乗");
    printf("factorize 既知の値 / 境界     : OK\n");
  }

  // ---- SPF / Pollard の切り替え境界 ----
  {
    // 試し割りで求めた分解と一致するか（境界の前後をすべて）
    auto trial = [](ll n) {
      vector<pair<ll, int>> f;
      for (ll p = 2; p * p <= n; p++) {
        int e = 0;
        while (n % p == 0) n /= p, e++;
        if (e) f.emplace_back(p, e);
      }
      if (n > 1) f.emplace_back(n, 1);
      return f;
    };
    int bad = 0;
    const ll LO = 1000000 - 2000, HI = 1000000 + 2000;
    for (ll n = LO; n <= HI; n++)
      if (factorize(n) != trial(n)) bad++;
    printf("境界 10^6 +-2000 の分解      : %s\n", bad == 0 ? "OK" : "NG");
    failed += bad ? 1 : 0;

    // is_prime も境界前後で一致するか
    int bad2 = 0;
    for (ll n = LO; n <= HI; n++) {
      auto f = trial(n);
      bool t = n > 1 && f.size() == 1 && f[0].second == 1;
      if (is_prime(n) != t) bad2++;
    }
    printf("境界 10^6 +-2000 の素数判定  : %s\n", bad2 == 0 ? "OK" : "NG");
    failed += bad2 ? 1 : 0;
  }

  // ---- divisors を総当たりと照合 ----
  {
    int bad = 0;
    for (int t = 0; t < 3000; t++) {
      ll n = ll(rng() % 200000) + 1;
      auto d = divisors(n);
      vector<ll> b;
      for (ll i = 1; i * i <= n; i++)
        if (n % i == 0) {
          b.push_back(i);
          if (i != n / i) b.push_back(n / i);
        }
      sort(b.begin(), b.end());
      if (d != b) bad++;
    }
    check(divisors(1) == vector<ll>{1}, "divisors(1)");
    check(divisors(0).empty() && divisors(-4).empty(), "divisors(0 以下)");
    printf("divisors  総当たりと照合      : %s\n", bad == 0 ? "OK" : "NG");
    failed += bad ? 1 : 0;
  }

#ifdef _GLIBCXX_DEBUG
  puts("速度計測                       : _GLIBCXX_DEBUG のため省略");
#else
  // ---- 速度: 10^18 級を 1000 個 ----
  {
    vector<ll> xs(1000);
    for (auto& x : xs) x = ll(rng() % 1000000000000000000LL) + 1;
    auto t0 = chrono::steady_clock::now();
    ll s = 0;
    for (ll x : xs) s += (ll)factorize(x).size();
    auto ms = chrono::duration_cast<chrono::milliseconds>(chrono::steady_clock::now() - t0).count();
    printf("速度 factorize 10^18 x1000    : %lld ms (素因数の種類 計 %lld)\n", (ll)ms, s);
  }

  // ---- 速度: 半素数（最悪ケース）----
  {
    // 10^9 級の素数 2 つの積を 200 個
    vector<ll> ps;
    for (ll x = 1000000000LL; ps.size() < 40; x++)
      if (is_prime(x)) ps.push_back(x);
    vector<ll> xs;
    for (int i = 0; i < 20; i++)
      for (int j = 0; j < 10; j++) xs.push_back(ps[i] * ps[j + 20]);
    auto t0 = chrono::steady_clock::now();
    for (ll x : xs) {
      auto f = factorize(x);
      if (f.size() != 2 && !(f.size() == 1 && f[0].second == 2)) failed++;
    }
    auto ms = chrono::duration_cast<chrono::milliseconds>(chrono::steady_clock::now() - t0).count();
    printf("速度 半素数(10^9 x 10^9) x200 : %lld ms\n", (ll)ms);
  }

  // ---- 速度: 小さい数を大量に ----
  // ---- prime_factors（重複込み）----
  {
    int bad = 0;
    for (ll n = 1; n <= 20000; n++) {
      auto fs = prime_factors(n);
      // 積が n に戻るか / 昇順か / 全部素数か
      ll prod = 1;
      for (size_t i = 0; i < fs.size(); i++) {
        prod *= fs[i];
        if (i && fs[i - 1] > fs[i]) bad++;
        if (!is_prime(fs[i])) bad++;
      }
      if (prod != n) bad++;
      // factorize を展開したものと一致するか
      vector<ll> want;
      for (auto [q, e] : factorize(n))
        for (int i = 0; i < e; i++) want.push_back(q);
      if (fs != want) bad++;
    }
    printf("prime_factors 積 / 昇順 / 素数性 : %s\n", bad == 0 ? "OK" : "NG");
    failed += bad ? 1 : 0;
  }
  {
    // 大きい n（Pollard の rho を通る経路）でも昇順か
    int bad = 0;
    for (int t = 0; t < 2000; t++) {
      ll n = (ll)(rng() % 1000000000000000000ULL) + 1;
      auto fs = prime_factors(n);
      __int128 prod = 1;
      for (size_t i = 0; i < fs.size(); i++) {
        prod *= fs[i];
        if (i && fs[i - 1] > fs[i]) bad++;
        if (!is_prime(fs[i])) bad++;
      }
      if (prod != (__int128)n) bad++;
    }
    // 大きい素数の積。順序が入れ替わりやすい形
    for (auto [a, b] : vector<pair<ll, ll>>{{999999937LL, 1000000007LL}, {1000000007LL, 999999937LL},
                                            {2LL, 999999999989LL}, {999999999989LL, 3LL}}) {
      auto fs = prime_factors(a * b);
      for (size_t i = 1; i < fs.size(); i++)
        if (fs[i - 1] > fs[i]) bad++;
    }
    printf("prime_factors 大きい n の昇順   : %s\n", bad == 0 ? "OK" : "NG");
    failed += bad ? 1 : 0;
  }
  {
    check(prime_factors(24) == (vector<ll>{2, 2, 2, 3}), "prime_factors(24)");
    check(prime_factors(1).empty() && prime_factors(0).empty() && prime_factors(-6).empty(),
          "prime_factors は n <= 1 で空");
    check(prime_factors(2) == vector<ll>{2}, "prime_factors(2)");
    check(prime_factors(1024) == vector<ll>(10, 2), "prime_factors(1024) は 2 が 10 個");
    check((ll)prime_factors(1000000007LL).size() == 1, "大きい素数は 1 個");
    check(prime_factors(1000000007LL * 1000000009LL) ==
              (vector<ll>{1000000007LL, 1000000009LL}),
          "半素数は 2 個");
    printf("prime_factors 既知の値 / 境界    : OK\n");
  }

  // ---- primes を篩と照合 ----
  {
    int bad = 0;
    for (int n = 0; n <= 3000; n++) {
      vector<ll> want;
      for (int i = 2; i <= n; i++)
        if (sieve[i]) want.push_back(i);
      if (primes(n) != want) bad++;
    }
    printf("primes    篩と照合 (0..3000)  : %s\n", bad == 0 ? "OK" : "NG");
    failed += bad ? 1 : 0;
  }
  {
    // 端と、2 のべき / 素数ちょうど / 合成数ちょうどの境目
    check(primes(-5).empty() && primes(0).empty() && primes(1).empty(), "primes は n < 2 で空");
    check(primes(2) == vector<ll>{2}, "primes(2)");
    check(primes(3) == vector<ll>{2, 3}, "primes(3)");
    check(primes(4) == (vector<ll>{2, 3}), "primes(4) は 4 を含まない");
    check(primes(9) == (vector<ll>{2, 3, 5, 7}), "primes(9) は 9 を含まない");
    check((ll)primes(100).size() == 25, "100 以下の素数は 25 個");
    check(primes(100).back() == 97, "100 以下の最大は 97");
    check((ll)primes(1000000).size() == 78498, "10^6 以下の素数は 78498 個");
    // 上限ちょうどが素数のとき取りこぼさないか
    for (ll q : {2LL, 3LL, 5LL, 7LL, 97LL, 101LL, 999983LL})
      check(primes(q).back() == q, "上限が素数なら最後がそれ");
    printf("primes    端 / 既知の個数     : OK\n");
  }
  {
    // is_prime と突き合わせる
    auto ps = primes(500000);
    int bad = 0;
    size_t k = 0;
    for (ll i = 2; i <= 500000; i++) {
      bool in = k < ps.size() && ps[k] == i;
      if (in != is_prime(i)) bad++;
      if (in) k++;
    }
    if (k != ps.size()) bad++;
    printf("primes    is_prime と照合     : %s\n", bad == 0 ? "OK" : "NG");
    failed += bad ? 1 : 0;
  }
  {
    auto t0 = chrono::steady_clock::now();
    auto ps = primes(10000000);
    auto ms = chrono::duration_cast<chrono::milliseconds>(chrono::steady_clock::now() - t0).count();
    printf("速度 primes(1e7)              : %lld ms (%zu 個)\n", (ll)ms, ps.size());
    check(ps.size() == 664579, "10^7 以下の素数は 664579 個");
    check(ms < 1000, "primes(1e7) が 1 秒以内");
  }
  {
    auto t0 = chrono::steady_clock::now();
    ll s = 0;
    for (int i = 1; i <= 200000; i++) s += (ll)factorize(i).size();
    auto ms = chrono::duration_cast<chrono::milliseconds>(chrono::steady_clock::now() - t0).count();
    printf("速度 factorize 1..2e5         : %lld ms\n", (ll)ms);
  }

#endif
  printf("\n%s\n", failed == 0 ? "すべて OK" : "失敗あり");
  return failed == 0 ? 0 : 1;
}
