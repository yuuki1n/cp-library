// math/combin.hpp の検証。パスカルの三角形など素朴な実装と突き合わせる。
//   g++ -std=gnu++20 -O2 -Wall -Wextra -I.. -IC:/acl combin_test.cpp -o combin_test
#include "../../math/combin.hpp"

#include <atcoder/modint>
#include <chrono>
#include <cstdio>
#include <random>
#include <string>
#include <vector>

using ll = long long;
using namespace std;
using mint = atcoder::modint998244353;
using mint7 = atcoder::modint1000000007;

int ng = 0;        // 今のブロックの NG 件数（report のたびに 0 に戻す）
int ng_total = 0;  // 全体の NG 件数
void check(bool ok, const string& msg) {
  if (!ok && ng < 5) printf("  NG: %s\n", msg.c_str());
  if (!ok) ng++, ng_total++;
}
void report(const string& name) {
  printf("%-34s : %s\n", name.c_str(), ng ? "NG" : "OK");
  ng = 0;
}

// パスカルの三角形。pas[n][r] = C(n, r)
vector<vector<mint>> pascal(int n) {
  vector<vector<mint>> p(n + 1, vector<mint>(n + 1, 0));
  for (int i = 0; i <= n; i++) {
    p[i][0] = 1;
    for (int j = 1; j <= i; j++) p[i][j] = p[i - 1][j - 1] + p[i - 1][j];
  }
  return p;
}

int main() {
  const int N = 60;
  auto pas = pascal(N);

  {  // 二項係数がパスカルの三角形と一致する
    combin<mint> C;
    for (int n = 0; n <= N; n++)
      for (int r = 0; r <= N; r++) {
        mint want = r <= n ? pas[n][r] : mint(0);
        check(C.C(n, r) == want, "C(" + to_string(n) + "," + to_string(r) + ")");
      }
    report("C がパスカルの三角形と一致");
  }

  {  // 範囲外は 0
    combin<mint> C;
    check(C.C(5, 6) == 0, "C(5,6)");
    check(C.C(5, -1) == 0, "C(5,-1)");
    check(C.C(-1, 0) == 0, "C(-1,0)");
    check(C.C(-3, -2) == 0, "C(-3,-2)");
    check(C.C(0, 0) == 1, "C(0,0)");
    check(C.P(5, 6) == 0, "P(5,6)");
    check(C.P(5, -1) == 0, "P(5,-1)");
    check(C.P(-1, 0) == 0, "P(-1,0)");
    check(C.P(0, 0) == 1, "P(0,0)");
    check(C.H(5, -1) == 0, "H(5,-1)");
    check(C.H(-1, 3) == 0, "H(-1,3)");
    check(C.H(0, 0) == 1, "H(0,0)");
    check(C.H(0, 3) == 0, "H(0,3)");
    check(C.H(5, 0) == 1, "H(5,0)");
    check(C.catalan(-1) == 0, "catalan(-1)");
    report("範囲外は 0");
  }

  {  // 順列 P(n,r) = n (n-1) ... (n-r+1)
    combin<mint> C;
    for (int n = 0; n <= N; n++)
      for (int r = 0; r <= n; r++) {
        mint want = 1;
        for (int i = 0; i < r; i++) want *= n - i;
        check(C.P(n, r) == want, "P(" + to_string(n) + "," + to_string(r) + ")");
      }
    report("P が素朴な積と一致");
  }

  {  // 重複組合せ。小さい範囲は全列挙で数える
    combin<mint> C;
    for (int n = 1; n <= 6; n++)
      for (int r = 0; r <= 6; r++) {
        // n 種類から r 個。非減少列 a[0] <= ... <= a[r-1] (< n) の個数
        ll cnt = 0;
        vector<int> a(r, 0);
        while (true) {
          cnt++;
          int i = r - 1;
          while (i >= 0 && a[i] == n - 1) i--;
          if (i < 0) break;
          int v = a[i] + 1;
          for (int j = i; j < r; j++) a[j] = v;
        }
        if (r == 0) cnt = 1;
        check(C.H(n, r) == mint(cnt), "H(" + to_string(n) + "," + to_string(r) + ")");
      }
    report("H が全列挙と一致");
  }

  {  // カタラン数
    combin<mint> C;
    ll want[] = {1, 1, 2, 5, 14, 42, 132, 429, 1430, 4862, 16796, 58786, 208012};
    for (int n = 0; n < 13; n++) check(C.catalan(n) == mint(want[n]), "catalan(" + to_string(n) + ")");
    // 漸化式 cat(n+1) = sum cat(i) cat(n-i) でも確かめる
    for (int n = 0; n < 40; n++) {
      mint s = 0;
      for (int i = 0; i <= n; i++) s += C.catalan(i) * C.catalan(n - i);
      check(C.catalan(n + 1) == s, "catalan 漸化式 n=" + to_string(n));
    }
    report("catalan");
  }

  {  // fact / finv / inv の整合
    combin<mint> C;
    for (int n = 0; n <= 300; n++) {
      check(C.fact(n) * C.finv(n) == 1, "fact*finv n=" + to_string(n));
      if (n >= 1) check(C.inv(n) * mint(n) == 1, "inv n=" + to_string(n));
      if (n >= 1) check(C.fact(n) == C.fact(n - 1) * mint(n), "fact 漸化式 n=" + to_string(n));
    }
    report("fact / finv / inv の整合");
  }

  {  // 問い合わせる順番で結果が変わらない（表の伸ばし方の検証）
    combin<mint> asc, desc, jump, pre(100000);
    for (int n = 0; n <= 1000; n++) asc.C(n, n / 2);
    for (int n = 1000; n >= 0; n--) desc.C(n, n / 2);
    jump.C(100000, 50000), jump.C(3, 1), jump.C(99999, 2);
    for (int n = 0; n <= 1000; n++) {
      mint v = asc.C(n, n / 2);
      check(desc.C(n, n / 2) == v, "降順 n=" + to_string(n));
      check(jump.C(n, n / 2) == v, "飛び石 n=" + to_string(n));
      check(pre.C(n, n / 2) == v, "先に確保 n=" + to_string(n));
    }
    combin<mint> fresh;
    check(fresh.C(100000, 50000) == jump.C(100000, 50000), "C(100000,50000)");
    check(fresh.C(99999, 2) == jump.C(99999, 2), "C(99999,2)");
    report("問い合わせ順に依らない");
  }

  {  // 大きい n でも漸化式が成り立つ
    combin<mint> C;
    mt19937 rng(20261001);
    for (int t = 0; t < 200; t++) {
      ll n = 1 + rng() % 2000000, r = rng() % (unsigned)n;
      check(C.C(n, r) == C.C(n - 1, r - 1) + C.C(n - 1, r), "大きい n の漸化式 n=" + to_string(n));
      check(C.C(n, r) == C.C(n, n - r), "対称性 n=" + to_string(n));
      check(C.P(n, r) == C.C(n, r) * C.fact(r), "P = C * r! n=" + to_string(n));
    }
    report("大きい n");
  }

  {  // 法を変えても同じように動く
    combin<mint7> C;
    vector<vector<mint7>> p(N + 1, vector<mint7>(N + 1, 0));
    for (int i = 0; i <= N; i++) {
      p[i][0] = 1;
      for (int j = 1; j <= i; j++) p[i][j] = p[i - 1][j - 1] + p[i - 1][j];
    }
    for (int n = 0; n <= N; n++)
      for (int r = 0; r <= n; r++) check(C.C(n, r) == p[n][r], "mod 1e9+7 C(" + to_string(n) + "," + to_string(r) + ")");
    report("別の法でも動く");
  }

  {  // 昇順に引いても伸ばし直しが積み上がらない（表は 2 倍ずつ伸びる）
    auto st = chrono::steady_clock::now();
    combin<mint> C;
    mint s = 0;
    for (int n = 0; n <= 2000000; n++) s += C.C(n, 2);
    auto ms = chrono::duration_cast<chrono::milliseconds>(chrono::steady_clock::now() - st).count();
    printf("%-34s : %lld ms (s=%u)\n", "昇順に 2e6 回", (ll)ms, s.val());
    check(ms < 60, "昇順 2e6 回で 60 ミリ秒以内");
    report("伸ばし直しが積み上がらない");
  }

  {  // 速度。2e6 まで伸ばしながら 1e6 回引く
    auto st = chrono::steady_clock::now();
    combin<mint> C;
    mint s = 0;
    mt19937 rng(1);
    for (int t = 0; t < 1000000; t++) {
      ll n = rng() % 2000000;
      s += C.C(n, n / 3);
    }
    auto ms = chrono::duration_cast<chrono::milliseconds>(chrono::steady_clock::now() - st).count();
    printf("%-34s : %lld ms (s=%u)\n", "1e6 回の C（表は 2e6 まで）", (ll)ms, s.val());
    check(ms < 1000, "1e6 回で 1 秒以内");
    report("速度");
  }

  printf("\n合計 NG: %d\n", ng_total);
  return ng_total ? 1 : 0;
}
