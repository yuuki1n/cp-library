// util/set_search.hpp の検証。ソート済み配列を素朴に走査した結果と突き合わせる。
//   g++ -std=gnu++20 -O2 -Wall -Wextra -I.. set_search_test.cpp -o set_search_test
#include "../../util/set_search.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <random>
#include <set>
#include <string>
#include <utility>
#include <vector>

using ll = long long;
using namespace std;

int ng = 0;        // 今のブロックの NG 件数（report のたびに 0 に戻す）
int ng_total = 0;  // 全体の NG 件数
void check(bool ok, const string& msg) {
  if (!ok && ng < 5) printf("  NG: %s\n", msg.c_str());
  if (!ok) ng++, ng_total++;
}
void report(const string& name) {
  printf("%-30s : %s\n", name.c_str(), ng ? "NG" : "OK");
  ng = 0;
}

const ll LINF = 2002003004005006007LL;
const int INF = 1001001001;

// 素朴版。v は昇順（重複あり）
ll naive_ceiling(const vector<ll>& v, ll x, ll def) {
  for (ll a : v)
    if (a >= x) return a;
  return def;
}
ll naive_higher(const vector<ll>& v, ll x, ll def) {
  for (ll a : v)
    if (a > x) return a;
  return def;
}
ll naive_floor(const vector<ll>& v, ll x, ll def) {
  ll r = def;
  for (ll a : v)
    if (a <= x) r = a;
  return r;
}
ll naive_lower(const vector<ll>& v, ll x, ll def) {
  ll r = def;
  for (ll a : v)
    if (a < x) r = a;
  return r;
}

int main() {
  mt19937 rng(20261115);

  {  // ---- set<ll> / multiset<ll>：既定値と def ----
    for (int it = 0; it < 3000; it++) {
      int n = (int)(rng() % 8);
      set<ll> s;
      multiset<ll> m;
      vector<ll> v;
      for (int i = 0; i < n; i++) {
        ll a = (ll)(rng() % 11) - 5;
        s.insert(a), m.insert(a), v.push_back(a);
      }
      vector<ll> u(s.begin(), s.end());
      sort(v.begin(), v.end());
      for (ll x = -7; x <= 7; x++) {
        check(ceiling(s, x) == naive_ceiling(u, x, LINF), "set ceiling");
        check(higher(s, x) == naive_higher(u, x, LINF), "set higher");
        check(floor(s, x) == naive_floor(u, x, -LINF), "set floor");
        check(lower(s, x) == naive_lower(u, x, -LINF), "set lower");
        check(ceiling(m, x, 99LL) == naive_ceiling(v, x, 99), "multiset ceiling def");
        check(higher(m, x, 99LL) == naive_higher(v, x, 99), "multiset higher def");
        check(floor(m, x, -99LL) == naive_floor(v, x, -99), "multiset floor def");
        check(lower(m, x, -99LL) == naive_lower(v, x, -99), "multiset lower def");
      }
    }
    report("set / multiset<ll>");
  }

  {  // ---- 型ごとの既定値と、いろいろな型 ----
    set<int> si{1, 3};
    check(higher(si, 3) == INF, "int は INF");
    check(lower(si, 1) == -INF, "int は -INF");
    check(higher(si, 1) == 3, "int に値があるとき");
    set<ll> sl{1, 3};
    check(higher(sl, 3) == LINF, "ll は LINF");
    check(floor(sl, 0) == -LINF, "ll は -LINF");
    check(higher(sl, 1, 7) == 3 && higher(sl, 3, 7) == 7, "int リテラルの def");
    set<ll> empty;
    check(ceiling(empty, 0) == LINF && floor(empty, 0) == -LINF, "空の set");
    set<pair<int, int>> sp{{1, 2}, {3, 4}};
    check(ceiling(sp, {2, 0}, {-1, -1}) == make_pair(3, 4), "pair（def を渡す）");
    check(lower(sp, {1, 2}, {-1, -1}) == make_pair(-1, -1), "pair の無いとき");
    set<string> ss{"b", "d"};
    check(higher(ss, string("b"), string("")) == "d", "string");
    check(std::floor(2.7) == 2.0 && floor(2.7) == 2.0, "std::floor と衝突しない");
    report("既定値 / いろいろな型");
  }

  printf(ng_total ? "\nNG %d 件\n" : "\nすべて OK\n", ng_total);
  return ng_total ? 1 : 0;
}
