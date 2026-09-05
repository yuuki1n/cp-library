#include <map>
#include <utility>
#include <vector>

/*
 * dynamic_dsu : 頂点を後から足せる dsu
 *
 *   頂点番号が 0 .. n-1 でない（座標、文字列、大きすぎる値など）ときに使う。
 *   出てきたキーを内部で 0 から順に振り直し、実体はふつうの配列で持つ。
 *   キーの引き当てにだけ Map の分の時間がかかり、あとはならし O(alpha(n))。
 *   既定の std::map なら 1 操作 O(log n)。比較できる型ならそのまま使えるので、
 *   pair や tuple や string をキーにできる。ハッシュを用意できるなら
 *   dynamic_dsu<K, unordered_map<K, int>> にすると速い。
 *
 *   add(x)        頂点 x を足す。新しく足したら true
 *   merge(a, b)   併合する。知らないキーは自動で足す。実際に併合したら true
 *   leader(x)     x の属する成分の代表キー
 *   same(a, b)    同じ成分か
 *   size(x)       x の属する成分の大きさ
 *   group_count()       連結成分の個数
 *   vertex_count()    今までに足した頂点の個数
 *   group(x)      x と同じ成分のキー。O(|成分| log n)
 *   id(x)         x に振られた内部番号（無ければ足す）
 *   clear()       構築直後の状態に戻す（頂点もすべて捨てる）
 *
 *   atcoder::dsu は継承していない。dsu は構築時に頂点数が決まる作りで、
 *   後から増やせないため。
 *
 * 使用例:
 *   dynamic_dsu<pair<int, int>> uf;
 *   uf.merge({0, 0}, {0, 1});
 *   uf.merge({5, 5}, {0, 0});
 *   print(uf.vertex_count(), uf.group_count());   // 3 1
 *   print(uf.size({0, 1}));            // 3
 *   fore(k, uf.group({0, 0})) print(k);
 *
 *   // 座標圧縮の代わりに使う
 *   dynamic_dsu<long long> uf;
 *   rep(M) { LL(a, b); uf.merge(a, b); }
 *   print(uf.vertex_count(), uf.group_count());
 *
 * verify:
 *   (未 verify)
 */
template <class K, class Map = std::map<K, int>>
struct dynamic_dsu {
 private:
  Map idx;               // キー -> 内部番号
  std::vector<K> key;    // 内部番号 -> キー
  std::vector<int> dat;  // 負なら -(成分の大きさ)、非負なら親
  std::vector<int> nxt;  // 成分ごとの巡回リスト
  int num = 0;           // 連結成分の個数

  int root(int x) {
    while (dat[x] >= 0) {
      if (dat[dat[x]] >= 0) dat[x] = dat[dat[x]];  // 経路を半分に畳む
      x = dat[x];
    }
    return x;
  }

 public:
  // x に振られた内部番号。まだ無ければ足す
  int id(const K& x) {
    // 探索を 1 回で済ませたいので try_emplace で「有無の確認」と「追加」を兼ねる
    auto [it, inserted] = idx.try_emplace(x, (int)key.size());
    if (inserted) {
      key.push_back(x);
      dat.push_back(-1);
      nxt.push_back(it->second);
      num++;
    }
    return it->second;
  }

  // 頂点 x を足す。新しく足したら true
  bool add(const K& x) {
    int before = (int)key.size();
    id(x);
    return (int)key.size() != before;
  }

  // 構築直後の状態に戻す（頂点もすべて捨てる）
  void clear() {
    idx.clear();
    key.clear();
    dat.clear();
    nxt.clear();
    num = 0;
  }

  int vertex_count() const { return (int)key.size(); }
  int group_count() const { return num; }

  K leader(const K& x) { return key[root(id(x))]; }
  bool same(const K& a, const K& b) { return root(id(a)) == root(id(b)); }
  int size(const K& x) { return -dat[root(id(x))]; }

  // 併合する。もともと別の成分だったら true
  bool merge(const K& a, const K& b) {
    int x = root(id(a)), y = root(id(b));
    if (x == y) return false;
    if (-dat[x] < -dat[y]) std::swap(x, y);  // x を大きい方に
    std::swap(nxt[x], nxt[y]);  // 2 つの環の next を交換すると 1 つの環になる
    dat[x] += dat[y];
    dat[y] = x;
    num--;
    return true;
  }

  // x と同じ成分のキーを返す（x 自身も含む）
  std::vector<K> group(const K& x) {
    int r = root(id(x)), n = -dat[r], c = r;
    std::vector<K> ret;
    ret.reserve(n);
    for (int i = 0; i < n; i++) ret.push_back(key[c = nxt[c]]);
    return ret;
  }
};
