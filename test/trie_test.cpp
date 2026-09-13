// string/trie.hpp / data_structure/binary_trie.hpp の検証。
// 素朴な map / multiset と突き合わせる。
//   g++ -std=gnu++20 -O2 -Wall -Wextra -I.. trie_test.cpp -o trie_test
#include "../string/trie.hpp"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <map>
#include <random>
#include <set>
#include <string>
#include <vector>

#include "../data_structure/binary_trie.hpp"

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

int main() {
  mt19937 rng(20260912);

  {  // ---- trie: map との突き合わせ ----
    ng = 0;
    for (int iter = 0; iter < 300; iter++) {
      trie<3, 'a'> t;
      map<string, int> ref;
      for (int q = 0; q < 60; q++) {
        int len = (int)(rng() % 5);
        string s;
        for (int i = 0; i < len; i++) s += char('a' + rng() % 3);

        if (rng() % 3 == 0 && ref[s] > 0) {
          t.erase(s);
          ref[s]--;
        } else {
          t.add(s);
          ref[s]++;
        }

        // count / contains
        check(t.count(s) == ref[s], "count");
        check(t.contains(s) == (ref[s] > 0), "contains");

        // size
        int tot = 0;
        for (auto& [k, v] : ref) tot += v;
        check(t.size() == tot, "size");

        // count_prefix を総当たりと比べる
        int plen = (int)(rng() % 4);
        string p;
        for (int i = 0; i < plen; i++) p += char('a' + rng() % 3);
        int want = 0;
        for (auto& [k, v] : ref)
          if (k.size() >= p.size() && k.compare(0, p.size(), p) == 0) want += v;
        check(t.count_prefix(p) == want, "count_prefix");
      }
    }
    report("trie");
  }

  {  // ---- trie: 木をたどる / 端のケース ----
    ng = 0;
    trie<> t;
    check(t.empty() && t.size() == 0 && t.node_count() == 1, "空の状態");
    check(t.find("abc") == -1 && t.count("abc") == 0, "空で find");
    check(t.count_prefix("") == 0, "空で count_prefix");

    t.add("apple");
    t.add("apple");
    t.add("apricot");
    check(t.count("apple") == 2 && t.count("apricot") == 1, "count");
    check(t.count_prefix("ap") == 3, "count_prefix ap");
    check(t.count_prefix("app") == 2, "count_prefix app");
    check(t.count_prefix("") == 3, "空文字列は全部の接頭辞");
    check(t.count("app") == 0, "途中のノードは終端ではない");
    check(t.size() == 3, "size");

    // 根からたどる
    int v = t.root();
    for (char c : string("ap")) v = t.child(v, c - 'a');
    check(t.cnt(v) == 3 && t.ends(v) == 0, "child でたどる");

    t.erase("apple");
    check(t.count("apple") == 1 && t.count_prefix("ap") == 2, "erase");

    // 空文字列そのものを入れる
    trie<> u;
    u.add("");
    check(u.count("") == 1 && u.size() == 1, "空文字列を入れる");

    // 番号で渡す版
    trie<2> b;
    b.add(vector<int>{0, 1, 1});
    b.add(vector<int>{0, 1});
    check(b.count(vector<int>{0, 1, 1}) == 1, "vector<int> 版 count");
    check(b.count_prefix(vector<int>{0, 1}) == 2,
          "vector<int> 版 count_prefix");

    // path
    {
      trie<> p;
      for (string x : {"apple", "apply", "apricot", "banana"}) p.add(x);

      auto pa = p.path("apple");
      check((int)pa.size() == 6 && pa[0] == p.root(), "path の長さと先頭");
      check(p.find("apple") == pa.back(), "path の末尾は find と一致");
      // 各接頭辞の本数
      vector<int> want{4, 3, 3, 2, 2, 1};
      for (size_t i = 0; i < pa.size(); i++)
        check(p.cnt(pa[i]) == want[i], "path 上の cnt");

      check((int)p.path("axe").size() == 2, "途中で切れる");
      check((int)p.path("banjo").size() == 4, "ban まで一致");
      check((int)p.path("zzz").size() == 1, "1 文字も一致しない");
      check((int)p.path("").size() == 1, "空文字列は根だけ");
      check((int)p.path("applepie").size() == 6, "辞書より長い");

      // vector<int> 版
      trie<2> q;
      q.add(vector<int>{0, 1, 1});
      check((int)q.path(vector<int>{0, 1}).size() == 3, "vector<int> 版 path");

      // 空のトライ
      trie<> emp;
      check((int)emp.path("abc").size() == 1, "空のトライでは根だけ");
    }

    t.clear();
    check(t.empty() && t.node_count() == 1, "clear");
    report("trie（端のケース）");
  }

  {  // ---- binary_trie: multiset との突き合わせ ----
    ng = 0;
    for (int iter = 0; iter < 300; iter++) {
      binary_trie<8> t;
      multiset<ll> ref;
      for (int q = 0; q < 60; q++) {
        ll x = (ll)(rng() % 256);
        if (rng() % 3 == 0 && ref.count(x)) {
          t.erase(x);
          ref.erase(ref.find(x));
        } else {
          t.insert(x);
          ref.insert(x);
        }
        check(t.size() == (int)ref.size(), "size");
        check(t.count(x) == (int)ref.count(x), "count");
        check(t.contains(x) == (ref.count(x) > 0), "contains");
        if (ref.empty()) continue;

        vector<ll> v(ref.begin(), ref.end());
        int k = (int)(rng() % v.size());
        check(t.kth(k) == v[k], "kth");
        check(t.min_value() == v.front(), "min_value");
        check(t.max_value() == v.back(), "max_value");

        ll y = (ll)(rng() % 256);
        int want = (int)(lower_bound(v.begin(), v.end(), y) - v.begin());
        check(t.count_less(y) == want, "count_less");

        // x と xor した値の k 番目を総当たりと比べる
        vector<ll> w;
        for (ll a : v) w.push_back(a ^ y);
        sort(w.begin(), w.end());
        check(t.min_value(y) == w.front(), "min_value(x)");
        check(t.max_value(y) == w.back(), "max_value(x)");
        check(t.kth(k, y) == w[k], "kth(k, x)");
        check(t.kth(k, 0) == t.kth(k), "flip 省略は flip = 0 と同じ");
        // count_less の flip 版
        int wl = (int)(lower_bound(w.begin(), w.end(), y) - w.begin());
        check(t.count_less(y, y) == wl, "count_less(ub, flip)");
        check(t.count_less(y, 0) == t.count_less(y), "count_less も省略できる");
        // xor_all と同じ結果になるか
        t.xor_all(y);
        check(t.min_value() == w.front(), "xor_all 経由と一致");
        t.xor_all(y);
        check(t.min_value() == v.front(), "xor_all で戻せている");
      }
    }
    report("binary_trie");
  }

  {  // ---- binary_trie: xor_all を混ぜる ----
    ng = 0;
    for (int iter = 0; iter < 300; iter++) {
      binary_trie<8> t;
      multiset<ll> ref;
      for (int q = 0; q < 50; q++) {
        int op = (int)(rng() % 4);
        if (op == 0 && !ref.empty()) {  // 全体に xor
          ll v = (ll)(rng() % 256);
          t.xor_all(v);
          multiset<ll> nx;
          for (ll a : ref) nx.insert(a ^ v);
          ref = nx;
        } else if (op == 1 && !ref.empty()) {  // 削除
          ll x = *next(ref.begin(), (int)(rng() % ref.size()));
          t.erase(x);
          ref.erase(ref.find(x));
        } else {  // 追加
          ll x = (ll)(rng() % 256);
          t.insert(x);
          ref.insert(x);
        }
        check(t.size() == (int)ref.size(), "xor 混在: size");
        if (ref.empty()) continue;
        vector<ll> v(ref.begin(), ref.end());
        int k = (int)(rng() % v.size());
        check(t.kth(k) == v[k], "xor 混在: kth");
        ll y = (ll)(rng() % 256);
        check(t.count(y) == (int)ref.count(y), "xor 混在: count");
        int want = (int)(lower_bound(v.begin(), v.end(), y) - v.begin());
        check(t.count_less(y) == want, "xor 混在: count_less");
      }
    }
    report("binary_trie（xor_all 混在）");
  }

  {  // ---- binary_trie: 大きい値 / 端のケース ----
    ng = 0;
    binary_trie<60> t;
    check(t.empty() && t.size() == 0, "空");
    check(t.count(0) == 0 && !t.contains(0), "空で count");

    t.insert(0);
    t.insert((1LL << 59) - 1);
    t.insert(1000000000000000000LL);
    check(t.size() == 3, "10^18 級を入れる");
    check(t.min_value() == 0, "min");
    check(t.max_value() == 1000000000000000000LL, "max");
    check(t.kth(1) == (1LL << 59) - 1, "kth");
    check(t.count_less(1) == 1, "count_less");

    t.insert(0);
    check(t.count(0) == 2 && t.size() == 4, "同じ値を 2 個");
    t.erase(0);
    check(t.count(0) == 1, "1 個だけ消える");

    // const 参照経由でも読み取り系は呼べる
    {
      binary_trie<8> c;
      for (ll x : {5LL, 9LL, 1LL}) c.insert(x);
      auto probe = [](const binary_trie<8>& r) {
        return r.size() + (int)r.min_value() + (int)r.max_value(3) +
               (int)r.kth(1, 2) + r.count_less(9) + r.count_less(9, 3) +
               (int)r.count(5);
      };
      check(probe(c) == 3 + 1 + 10 + 7 + 2 + 2 + 1, "const 参照から呼べる");
    }

    t.clear();
    check(t.empty() && t.node_count() == 1, "clear");
    report("binary_trie（端のケース）");
  }

#ifdef _GLIBCXX_DEBUG
  puts("速度計測                       : _GLIBCXX_DEBUG のため省略");
#else
  {  // ---- 速度 ----
    auto ms = [](auto a, auto b) {
      return (ll)chrono::duration_cast<chrono::milliseconds>(b - a).count();
    };
    {
      const int N = 200000, L = 10;
      vector<string> ss(N);
      for (auto& s : ss) {
        s.resize(L);
        for (auto& c : s) c = char('a' + rng() % 26);
      }
      auto st = chrono::steady_clock::now();
      trie<> t;
      for (auto& s : ss) t.add(s);
      ll acc = 0;
      for (auto& s : ss) acc += t.count_prefix(s);
      printf("%-30s : %lld ms  (ノード %d)\n", "速度 trie 2e5 本 x 長さ 10",
             ms(st, chrono::steady_clock::now()), t.node_count());
      (void)acc;
    }
    {
      const int N = 200000;
      vector<ll> a(N);
      for (auto& x : a) x = (ll)(rng() % 1000000000);
      auto st = chrono::steady_clock::now();
      binary_trie<30> t;
      ll acc = 0;
      for (ll x : a) {
        if (t.size()) acc ^= t.max_value(x);
        t.insert(x);
      }
      printf("%-30s : %lld ms  (ノード %d)\n", "速度 binary_trie 2e5 個",
             ms(st, chrono::steady_clock::now()), t.node_count());
      (void)acc;
    }
  }
#endif

  printf(ng_total ? "\nNG %d 件\n" : "\nすべて OK\n", ng_total);
  return ng_total ? 1 : 0;
}
