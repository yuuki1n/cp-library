#include <algorithm>
#include <array>
#include <numeric>
#include <utility>
#include <vector>

/*
 * rollback_dsu : 併合を巻き戻せる dsu
 *
 *   経路圧縮をしない代わりに、merge のたびに変更を記録して undo できる。
 *   union by size だけなので木の高さは O(log n) に収まり、leader / same /
 *   size はすべて O(log n)、merge と undo は O(log n) / O(1)。
 *   辺を消す操作がある問題（Offline Dynamic Connectivity など）で使う。
 *
 *   rollback_dsu(n)   頂点 0 .. n-1
 *   merge(a, b)      併合する。実際に併合したら true
 *   undo()           直前の merge を 1 回取り消す。併合しなかった merge も
 *                    1 回として数えるので、merge と undo は必ず 1 対 1 で対応する。
 *                    履歴が空のときは何もしない
 *   snapshot()       今までに呼んだ merge の回数
 *   rollback(t)      merge を t 回呼んだ時点まで戻す
 *   leader / same / size / group_count / group
 *   clear()          構築直後の状態に戻す（履歴も捨てる）
 *
 *   atcoder::dsu は継承していない。dsu の leader は経路圧縮をしてしまい
 *   巻き戻せる形にならないうえ、parent_or_size が private で書き戻せないため。
 *
 * 使用例:
 *   rollback_dsu uf(N);
 *   uf.merge(0, 1);
 *   int t = uf.snapshot();
 *   uf.merge(1, 2);
 *   uf.merge(3, 4);
 *   uf.rollback(t);          // 後ろ 2 回を取り消す
 *   print(uf.same(1, 2));    // false
 *
 * verify:
 *   (未 verify)
 *   予定: https://judge.yosupo.jp/problem/dynamic_graph_vertex_add_component_sum
 *         （monoid_dsu と組み合わせて Offline Dynamic Connectivity で）
 */
struct rollback_dsu {
 private:
  std::vector<int> dat;  // 負なら -(成分の大きさ)、非負なら親
  std::vector<int> nxt;  // 成分ごとの巡回リスト
  int num;
  // {子になった根, その根の元の dat, 親になった根}。併合しなかったときは {-1,0,0}
  std::vector<std::array<int, 3>> hst;

 public:
  rollback_dsu() : rollback_dsu(0) {}
  explicit rollback_dsu(int n) : dat(n, -1), nxt(n), num(n) {
    std::iota(nxt.begin(), nxt.end(), 0);
  }

  // 経路圧縮はしない。高さは O(log n)
  int leader(int x) const {
    while (dat[x] >= 0) x = dat[x];
    return x;
  }
  bool same(int a, int b) const { return leader(a) == leader(b); }
  int size(int x) const { return -dat[leader(x)]; }
  int group_count() const { return num; }

  // 併合する。もともと別の成分だったら true
  bool merge(int a, int b) {
    int x = leader(a), y = leader(b);
    if (x == y) {
      hst.push_back({-1, 0, 0});  // undo の回数を merge と合わせるための空記録
      return false;
    }
    if (-dat[x] < -dat[y]) std::swap(x, y);  // x を大きい方に
    hst.push_back({y, dat[y], x});
    std::swap(nxt[x], nxt[y]);  // 2 つの環の next を交換すると 1 つの環になる
    dat[x] += dat[y];
    dat[y] = x;
    num--;
    return true;
  }

  // 直前の merge を 1 回取り消す。履歴が空なら何もしない
  void undo() {
    if (hst.empty()) return;
    auto [y, dy, x] = hst.back();
    hst.pop_back();
    if (y < 0) return;
    dat[y] = dy;
    dat[x] -= dy;
    std::swap(nxt[x], nxt[y]);  // 交換の取り消しはもう一度交換するだけ
    num++;
  }

  // 今までに呼んだ merge の回数
  int snapshot() const { return (int)hst.size(); }
  // merge を t 回呼んだ時点まで戻す
  void rollback(int t) {
    while (!hst.empty() && (int)hst.size() > t) undo();
  }

  // 構築直後の状態に戻す（大きさはそのまま）。履歴も捨てる
  void clear() {
    std::fill(dat.begin(), dat.end(), -1);
    std::iota(nxt.begin(), nxt.end(), 0);
    num = (int)dat.size();
    hst.clear();
  }

  // x と同じ成分の頂点を返す（x 自身も含む）
  std::vector<int> group(int x) const {
    int n = size(x), c = leader(x);
    std::vector<int> ret;
    ret.reserve(n);
    for (int i = 0; i < n; i++) ret.push_back(c = nxt[c]);
    return ret;
  }
};
