#include <algorithm>
#include <cmath>
#include <type_traits>
#include <utility>
#include <vector>

/*
 * mo : Mo's algorithm。クエリを並べ替えて区間を少しずつ動かす
 *
 *   mo(n)               長さ n の列を対象にする
 *   mo(n, b)            ブロック幅を b にする（既定は 1.22 n / sqrt(q)）
 *   add_query(l, r)     区間 [l, r) を問う。半開
 *   solve(add, rem, get)                      答えを「足した順」で返す
 *   solve(add_l, add_r, rem_l, rem_r, get)    左右で処理が変わるとき
 *
 *   add(i) / rem(i) は位置 i を今の区間に入れる / 外す。get() は今の答えを返す。
 *   get(i) と書くと、何番目のクエリを答えているかを受け取れる。
 *   状態は呼び出し側で持つ（ラムダで捕まえる）。
 *   区間の転倒数のように「左から入れたか右から入れたか」で処理が違うときは
 *   4 つ渡す。引数は動かす順（左を足す / 右を足す / 左を外す / 右を外す）。
 *   全体で add と rem が O((n + q) sqrt(q)) 回呼ばれるので、
 *   add / rem は O(1)、get は O(sqrt(n)) までに収める。
 *
 *   クエリはブロックごとに並べ、ブロックの偶奇で右端の向きを変える。
 *   これで右端が端から端へ戻るのを防げる（入れないと 1.5 倍ほど遅い）。
 *   区間が短いものばかりのときは既定の b が大きすぎるので、
 *   mo(n, 小さめの b) で上書きする。
 *
 * 使用例:
 *   // 区間に含まれる値の種類数
 *   mo m(N);
 *   rep(Q) { INT(l, r); m.add_query(--l, r); }
 *
 *   vll cnt(200001);
 *   ll kind = 0;
 *   auto add = [&](int i) { if (cnt[A[i]]++ == 0) kind++; };
 *   auto rem = [&](int i) { if (--cnt[A[i]] == 0) kind--; };
 *   auto get = [&] { return kind; };
 *   fore(x, m.solve(add, rem, get)) print(x);
 */
struct mo {
 private:
  int n, b_;  // b_ が 0 以下なら solve のときに決める
  std::vector<int> ql, qr;

  // get は get() でも get(i) でも書ける。戻り値は参照でも値で受け取る
  template <class Get> static auto call_get(Get& g, int i) {
    if constexpr (std::is_invocable_v<Get, int>) return g(i);
    else return g();
  }

 public:
  explicit mo(int n_ = 0, int b = -1) : n(n_), b_(b) {}

  // 区間 [l, r) を問う
  void add_query(int l, int r) { ql.push_back(l), qr.push_back(r); }

  // add(i) / rem(i) で区間を動かし、各クエリで get() を呼ぶ。
  // 返るのは add_query した順に並べた答え
  template <class Add, class Rem, class Get> auto solve(Add add, Rem rem, Get get) const {
    return solve(add, add, rem, rem, get);
  }

  // 左右で処理が変わるとき用。引数は動かす順（左を足す / 右を足す / 左を外す / 右を外す）
  template <class AddL, class AddR, class RemL, class RemR, class Get>
  auto solve(AddL add_l, AddR add_r, RemL rem_l, RemR rem_r, Get get) const {
    int q = (int)ql.size();
    std::vector<decltype(call_get(get, 0))> ans(q);  // get が参照を返しても値で受ける
    if (q == 0) return ans;

    int b = b_ > 0 ? b_ : std::max(1, int(std::sqrt(1.5 / q) * n) + 1);
    // 比較を軽くするため、並べ替えの基準を 1 つの整数に詰める。
    // 奇数ブロックは右端を反転させて、ブロックを跨ぐときの戻りを無くす
    std::vector<std::pair<long long, int>> ord(q);
    for (int i = 0; i < q; i++) {
      long long blk = ql[i] / b;
      ord[i] = {blk * ((long long)n + 1) + (blk & 1 ? qr[i] : n - qr[i]), i};
    }
    std::sort(ord.begin(), ord.end());

    int l = ql[ord[0].second], r = l;
    for (auto [k, i] : ord) {
      while (ql[i] < l) add_l(--l);
      while (r < qr[i]) add_r(r++);
      while (l < ql[i]) rem_l(l++);
      while (qr[i] < r) rem_r(--r);
      ans[i] = call_get(get, i);  // get(i) と書けば何番目のクエリを答えているか分かる
    }
    return ans;
  }
};
