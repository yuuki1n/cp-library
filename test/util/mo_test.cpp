// mo の検証
#include <bits/stdc++.h>
using namespace std;

#include "../../util/mo.hpp"

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

int main() {
  mt19937 rng(20260927);

  {  // ---- 種類数を素朴な実装と突き合わせる ----
    ng = 0;
    for (int it = 0; it < 500; it++) {
      int n = 1 + (int)(rng() % 30), q = 1 + (int)(rng() % 20), vmax = 1 + (int)(rng() % 5);
      vector<int> a(n);
      for (auto& x : a) x = (int)(rng() % (unsigned)vmax);
      vector<pair<int, int>> qs;
      mo m(n);
      for (int i = 0; i < q; i++) {
        int l = (int)(rng() % (unsigned)(n + 1)), r = (int)(rng() % (unsigned)(n + 1));
        if (l > r) swap(l, r);
        qs.push_back({l, r});
        m.add_query(l, r);
      }

      // 区間が一時的に負の幅にならないか見る。入っていないものを外したら分かる
      vector<int> cnt(vmax, 0);
      vector<char> in_(n, 0);
      long long kind = 0;
      bool bad = false;
      auto got = m.solve(
          [&](int i) {
            if (in_[i]) bad = true;
            in_[i] = 1;
            if (cnt[a[i]]++ == 0) kind++;
          },
          [&](int i) {
            if (!in_[i]) bad = true;
            in_[i] = 0;
            if (--cnt[a[i]] == 0) kind--;
          },
          [&] { return kind; });
      check(!bad, "入っていない位置を外していない（区間が負の幅にならない）");
      check((int)got.size() == q, "答えの個数");
      for (int i = 0; i < q; i++) {
        set<int> s(a.begin() + qs[i].first, a.begin() + qs[i].second);
        check(got[i] == (long long)s.size(), "種類数");
      }
      // 最後に区間を空にすれば状態が戻っているはず
      check(kind >= 0, "状態が壊れていない");
    }
    report("種類数");
  }

  {  // ---- 和（add / rem が対称なもの） ----
    ng = 0;
    for (int it = 0; it < 300; it++) {
      int n = 1 + (int)(rng() % 25), q = 1 + (int)(rng() % 15);
      vector<long long> a(n);
      for (auto& x : a) x = (long long)(rng() % 100) - 50;
      vector<pair<int, int>> qs;
      mo m(n);
      for (int i = 0; i < q; i++) {
        int l = (int)(rng() % (unsigned)(n + 1)), r = (int)(rng() % (unsigned)(n + 1));
        if (l > r) swap(l, r);
        qs.push_back({l, r});
        m.add_query(l, r);
      }
      long long s = 0;
      auto got = m.solve([&](int i) { s += a[i]; }, [&](int i) { s -= a[i]; }, [&] { return s; });
      for (int i = 0; i < q; i++) {
        long long want = 0;
        for (int j = qs[i].first; j < qs[i].second; j++) want += a[j];
        check(got[i] == want, "区間和");
      }
    }
    report("区間和");
  }

  {  // ---- 左右を区別する版。区間の並びそのものを保つ ----
    // deque に前後から出し入れして、区間 [l, r) の中身と一致するか見る。
    // add_l / add_r / rem_l / rem_r のどれか 1 つでも取り違えると壊れる
    ng = 0;
    for (int it = 0; it < 400; it++) {
      int n = 1 + (int)(rng() % 25), q = 1 + (int)(rng() % 15);
      vector<int> a(n);
      for (auto& x : a) x = (int)(rng() % 100);
      vector<pair<int, int>> qs;
      mo m(n);
      for (int i = 0; i < q; i++) {
        int l = (int)(rng() % (unsigned)(n + 1)), r = (int)(rng() % (unsigned)(n + 1));
        if (l > r) swap(l, r);
        qs.push_back({l, r});
        m.add_query(l, r);
      }
      deque<int> dq;
      auto got = m.solve([&](int i) { dq.push_front(a[i]); },     // 左を足す
                         [&](int i) { dq.push_back(a[i]); },      // 右を足す
                         [&](int i) { (void)i; dq.pop_front(); }, // 左を外す
                         [&](int i) { (void)i; dq.pop_back(); },  // 右を外す
                         [&] { return vector<int>(dq.begin(), dq.end()); });
      for (int i = 0; i < q; i++) {
        vector<int> want(a.begin() + qs[i].first, a.begin() + qs[i].second);
        check(got[i] == want, "左右を区別した出し入れで区間の並びが保たれる");
      }
    }
    report("左右を区別する版");
  }

  {  // ---- ブロック幅を変えても答えは同じ ----
    ng = 0;
    for (int it = 0; it < 200; it++) {
      int n = 1 + (int)(rng() % 40), q = 1 + (int)(rng() % 20);
      vector<int> a(n);
      for (auto& x : a) x = (int)(rng() % 4);
      vector<pair<int, int>> qs;
      for (int i = 0; i < q; i++) {
        int l = (int)(rng() % (unsigned)(n + 1)), r = (int)(rng() % (unsigned)(n + 1));
        if (l > r) swap(l, r);
        qs.push_back({l, r});
      }
      vector<long long> base;
      for (int b : {-1, 1, 2, 3, 7, 100}) {
        mo m(n, b);
        for (auto [l, r] : qs) m.add_query(l, r);
        vector<int> cnt(4, 0);
        long long kind = 0;
        auto got = m.solve([&](int i) { if (cnt[a[i]]++ == 0) kind++; },
                           [&](int i) { if (--cnt[a[i]] == 0) kind--; },
                           [&] { return kind; });
        if (base.empty()) base = got;
        else check(got == base, "b を変えても同じ答え");
      }
    }
    report("ブロック幅を変える");
  }

  {  // ---- 並べ替えの質（add / rem の回数が増えていないか） ----
    // 正しさだけ見ていると「並べ替えない」「ジグザグをやめる」を見逃す。
    // どれも答えは合うが呼び出し回数が増えるので、そこを縛る
    ng = 0;
    for (auto [n, q] : vector<pair<int, int>>{{20000, 20000}, {50000, 20000}, {20000, 50000}}) {
      mo m(n);
      for (int i = 0; i < q; i++) {
        int l = (int)(rng() % (unsigned)n), r = (int)(rng() % (unsigned)n);
        if (l > r) swap(l, r);
        m.add_query(l, r + 1);
      }
      long long calls = 0;
      m.solve([&](int) { calls++; }, [&](int) { calls++; }, [] { return 0; });
      // 実測は (n+q)*sqrt(q) の 0.24 〜 0.59 倍。
      // ジグザグをやめると 0.35 〜 0.86 倍に増えるので、0.7 倍を上界にする
      double lim = 0.7 * (n + q) * sqrt((double)q);
      check(calls <= (long long)lim,
            "呼び出し回数 " + to_string(calls) + " <= " + to_string((long long)lim) + "（n=" + to_string(n) + "）");
    }
    report("並べ替えの質");
  }

  {  // ---- ブロック幅の上書きが効いているか ----
    // 短い区間ばかりのとき、b を小さくすると移動が減る
    ng = 0;
    int n = 20000, q = 20000;
    vector<pair<int, int>> qs;
    for (int i = 0; i < q; i++) {
      int l = (int)(rng() % (unsigned)(n - 20));
      qs.push_back({l, l + 1 + (int)(rng() % 20)});
    }
    auto calls_with = [&](int b) {
      mo m(n, b);
      for (auto [l, r] : qs) m.add_query(l, r);
      long long calls = 0;
      m.solve([&](int) { calls++; }, [&](int) { calls++; }, [] { return 0; });
      return calls;
    };
    long long base = calls_with(-1), small = calls_with(10);
    check(small < base, "b を小さくすると移動が減る（" + to_string(base) + " -> " + to_string(small) + "）");
    report("ブロック幅の上書き");
  }

  {  // ---- get(i) でクエリ番号を受け取れる ----
    ng = 0;
    int n = 60;
    vector<long long> a(n);
    for (auto& x : a) x = (long long)(rng() % 100);

    int q = 40;
    vector<int> ql(q), qr(q);
    mo m(n);
    for (int i = 0; i < q; i++) {
      int l = (int)(rng() % (unsigned)(n + 1)), r = (int)(rng() % (unsigned)(n + 1));
      if (l > r) swap(l, r);
      ql[i] = l, qr[i] = r, m.add_query(l, r);
    }

    long long sum = 0;
    auto add = [&](int i) { sum += a[i]; };
    auto rem = [&](int i) { sum -= a[i]; };

    // 渡ってくる i が、並べ替える前のクエリ番号になっているか
    vector<int> seen(q, 0);
    auto got = m.solve(add, rem, [&](int i) {
      seen[i]++;
      return sum * 1000 + i;  // 番号を答えに混ぜて、どのクエリに入ったか見る
    });
    for (int i = 0; i < q; i++) {
      long long want = 0;
      for (int j = ql[i]; j < qr[i]; j++) want += a[j];
      check(got[i] == want * 1000 + i, "get(i) の i が add_query 順 i=" + to_string(i));
      check(seen[i] == 1, "各クエリちょうど 1 回 i=" + to_string(i));
    }

    // get() と get(i) で答えが変わらない
    sum = 0;
    auto plain = m.solve(add, rem, [&] { return sum; });
    for (int i = 0; i < q; i++) check(got[i] / 1000 == plain[i], "get() と一致 i=" + to_string(i));
    report("get(i) でクエリ番号を受け取れる");
  }

  {  // ---- 端のケース ----
    ng = 0;
    {  // クエリ 0 個
      mo m(10);
      auto got = m.solve([](int) {}, [](int) {}, [] { return 0; });
      check(got.empty(), "クエリ 0 個");
    }
    {  // 長さ 0 の列
      mo m(0);
      m.add_query(0, 0);
      int calls = 0;
      auto got = m.solve([&](int) { calls++; }, [&](int) { calls++; }, [] { return 7; });
      check(got.size() == 1 && got[0] == 7, "長さ 0 の列");
      check(calls == 0, "add / rem は呼ばれない");
    }
    {  // 空区間だけ。区間を移す途中で add / rem は呼ばれるが、答えは 0 のまま
      mo m(5);
      m.add_query(2, 2), m.add_query(0, 0), m.add_query(5, 5);
      long long s = 0;
      vector<long long> a{1, 2, 4, 8, 16};
      auto got = m.solve([&](int i) { s += a[i]; }, [&](int i) { s -= a[i]; }, [&] { return s; });
      check(got == vector<long long>(3, 0), "空区間の答えは 0");
    }
    {  // 全体を 1 回
      mo m(4);
      m.add_query(0, 4);
      long long s = 0;
      vector<long long> a{1, 2, 4, 8};
      auto got = m.solve([&](int i) { s += a[i]; }, [&](int i) { s -= a[i]; }, [&] { return s; });
      check(got.size() == 1 && got[0] == 15, "全体");
    }
    {  // get が参照を返しても受けられる
      mo m(4);
      m.add_query(0, 2);
      deque<int> dq;
      vector<int> a{1, 2, 4, 8};
      auto got = m.solve([&](int i) { dq.push_back(a[i]); }, [&](int i) { (void)i; dq.pop_back(); },
                         [&]() -> const deque<int>& { return dq; });
      check(got.size() == 1 && got[0] == deque<int>({1, 2}), "get が参照を返す");
    }
    {  // 既定コンストラクタ
      mo m;
      auto got = m.solve([](int) {}, [](int) {}, [] { return 0; });
      check(got.empty(), "既定コンストラクタで solve");
    }
    {  // 同じ mo を 2 回 solve できる（状態を持たない）
      mo m(4);
      m.add_query(0, 2), m.add_query(1, 4);
      vector<long long> a{1, 2, 4, 8};
      auto f = [&] {
        long long s = 0;
        return m.solve([&](int i) { s += a[i]; }, [&](int i) { s -= a[i]; }, [&] { return s; });
      };
      check(f() == f(), "2 回 solve しても同じ");
    }
    report("端のケース");
  }

#ifdef _GLIBCXX_DEBUG
  puts("速度計測                           : _GLIBCXX_DEBUG のため省略");
#else
  {  // ---- 速度 ----
    const int N = 200000, Q = 200000;
    vector<int> a(N);
    for (auto& x : a) x = (int)(rng() % (unsigned)N);
    mo m(N);
    for (int i = 0; i < Q; i++) {
      int l = (int)(rng() % (unsigned)N), r = (int)(rng() % (unsigned)N);
      if (l > r) swap(l, r);
      m.add_query(l, r + 1);
    }
    auto bench = [&](const char* name, auto f) {
      auto st = chrono::steady_clock::now();
      f();
      printf("%-34s : %lld ms\n", name,
             (long long)chrono::duration_cast<chrono::milliseconds>(chrono::steady_clock::now() - st).count());
    };
    long long sink = 0;
    bench("速度 種類数 N=Q=2e5", [&] {
      vector<int> cnt(N + 1, 0);
      long long kind = 0;
      auto got = m.solve([&](int i) { if (cnt[a[i]]++ == 0) kind++; },
                         [&](int i) { if (--cnt[a[i]] == 0) kind--; },
                         [&] { return kind; });
      for (auto x : got) sink += x;
    });
    check(sink != 0, "最適化で消えない");
  }
#endif
  printf(ng_total ? "\nNG %d 件\n" : "\nすべて OK\n", ng_total);
  return ng_total ? 1 : 0;
}
