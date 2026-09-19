#include <algorithm>
#include <numeric>
#include <utility>
#include <vector>

/*
 * union_find : 1 つの成分を O(|成分|) で取り出せる Union-Find。ならし O(a(n))
 *
 *   merge(a, b)     併合する。実際に併合したら true
 *   leader(x)       成分の代表頂点
 *   same(a, b) / size(x)
 *   group_count()   連結成分の個数
 *   group(x)        x と同じ成分の頂点      O(a(n) + |成分|)
 *   groups()        連結成分ごとの頂点      O(n)
 *   clear()         構築直後に戻す（大きさはそのまま）
 *
 *   成分ごとに巡回リスト nxt を持つ。merge では 2 つの環の next を交換するだけで
 *   1 つの環になる。全成分が要るなら groups()、1 成分だけなら group()。
 *   groups() は成分の大きさによらず O(n) かかる。
 *   group() の中は環をたどった順、groups() の中は昇順。どちらも成分そのものの
 *   並び順は決めていない。
 *
 * 使用例:
 *   union_find uf(N);
 *   rep(M) { INT(u, v); uf.merge(--u, --v); }
 *   print(uf.group(0));
 *   fore(g, uf.groups()) print(g);
 *
 * verify:
 *   (未 verify)
 */
struct union_find {
 private:
  std::vector<int> dat;  // 負なら -(成分の大きさ)、非負なら親
  std::vector<int> nxt;  // 成分ごとの巡回リスト
  int num;               // 連結成分の個数

 public:
  union_find() : union_find(0) {}
  explicit union_find(int n) : dat(n, -1), nxt(n), num(n) {
    std::iota(nxt.begin(), nxt.end(), 0);
  }

  // 構築直後の状態に戻す（大きさはそのまま）
  void clear() {
    std::fill(dat.begin(), dat.end(), -1);
    std::iota(nxt.begin(), nxt.end(), 0);
    num = (int)dat.size();
  }

  int leader(int x) {
    while (dat[x] >= 0) {
      if (dat[dat[x]] >= 0) dat[x] = dat[dat[x]];  // 経路を半分に畳む
      x = dat[x];
    }
    return x;
  }
  bool same(int a, int b) { return leader(a) == leader(b); }
  int size(int x) { return -dat[leader(x)]; }
  int group_count() const { return num; }

  // 併合する。もともと別の成分だったら true
  bool merge(int a, int b) {
    int x = leader(a), y = leader(b);
    if (x == y) return false;
    if (-dat[x] < -dat[y]) std::swap(x, y);  // x を大きい方に
    std::swap(nxt[x], nxt[y]);  // 2 つの環の next を交換すると 1 つの環になる
    dat[x] += dat[y];
    dat[y] = x;
    num--;
    return true;
  }

  // x と同じ成分の頂点を返す（x 自身も含む）
  std::vector<int> group(int x) {
    int r = leader(x), n = -dat[r], c = r;
    std::vector<int> ret;
    ret.reserve(n);
    for (int i = 0; i < n; i++) ret.push_back(c = nxt[c]);
    return ret;
  }

  // 連結成分ごとの頂点。各成分の中は昇順
  std::vector<std::vector<int>> groups() {
    int n = (int)dat.size();
    std::vector<std::vector<int>> buf(n), ret;
    for (int i = 0; i < n; i++) buf[leader(i)].push_back(i);
    for (auto& g : buf)
      if (!g.empty()) ret.push_back(std::move(g));
    return ret;
  }
};
