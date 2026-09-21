#include <map>
#include <utility>
#include <vector>

/*
 * keyed_union_find<K, UF, Map> : 任意の Union-Find をキーで引けるようにする
 *
 *   id(x)           x に振られた添字（無ければ振る）
 *   key(i)          添字 i のキー
 *   add(x)          頂点 x を足す。新しく足したら true
 *   uf              素の Union-Find。添字で直に触りたいとき用
 *   vertex_count()  今までに出てきたキーの個数
 *   clear()         構築直後に戻す（キーもすべて捨てる）
 *
 *   大きさを渡さない。キーが初めて出てきた時点で頂点が 1 つ増える。
 *   merge / same / size / leader / group / groups / group_count はキーで呼べる。
 *   diff / pot / prod / apply / undo / rollback / snapshot など UF 固有のものも
 *   そのまま呼べる。呼ばなければ実体化されないので、その UF に無くても構わない。
 *   merge のコールバックが受け取るのはキーではなく添字。
 *
 *   rollback と組み合わせるときは、巻き戻るのは併合だけでキーの登録は戻らない。
 *   group_count() をスナップショット時点と揃えたいなら、先に add() で
 *   出てくるキーを登録しておく。
 *
 * 使用例:
 *   keyed_union_find<pair<int, int>, rollback_union_find<>> uf;
 *   uf.merge({0, 0}, {0, 1});
 *   int t = uf.snapshot();
 *   uf.rollback(t);
 *
 *   keyed_union_find<string, relational_union_find<>> uf;
 *   uf.merge("a", "b", 3);
 *   print(uf.diff("a", "b"));
 */
template <class K, class UF, class Map = std::map<K, int>> struct keyed_union_find {
  UF uf;

 private:
  Map idx;              // キー -> 添字
  std::vector<K> key_;  // 添字 -> キー

 public:
  // x に振られた添字。まだ無ければ頂点を 1 つ増やして振る
  int id(const K& x) {
    auto [it, inserted] = idx.try_emplace(x, (int)key_.size());
    if (inserted) key_.push_back(x), uf.extend();
    return it->second;
  }
  // 頂点 x を足す。新しく足したら true
  bool add(const K& x) {
    int before = (int)key_.size();
    return id(x), (int)key_.size() != before;
  }

  const K& key(int i) const { return key_[i]; }
  int vertex_count() const { return (int)key_.size(); }
  int group_count() const { return uf.group_count(); }

  template <class... A> auto merge(const K& a, const K& b, A&&... r) { return uf.merge(id(a), id(b), std::forward<A>(r)...); }
  bool same(const K& a, const K& b) { return uf.same(id(a), id(b)); }
  int size(const K& x) { return uf.size(id(x)); }
  K leader(const K& x) { return key_[uf.leader(id(x))]; }
  auto diff(const K& a, const K& b) { return uf.diff(id(a), id(b)); }
  auto pot(const K& x) { return uf.pot(id(x)); }
  auto prod(const K& x) { return uf.prod(id(x)); }
  template <class S> void apply(const K& x, const S& v) { uf.apply(id(x), v); }
  template <class F> bool consistent(const K& a, const K& b, F f) { return uf.consistent(id(a), id(b), f); }

  template <class... A> void undo(A&&... a) { uf.undo(std::forward<A>(a)...); }
  template <class... A> void rollback(A&&... a) { uf.rollback(std::forward<A>(a)...); }
  int snapshot() const { return uf.snapshot(); }

  // 構築直後に戻す（キーもすべて捨てる）。uf.clear() では大きさが残る
  void clear() {
    uf = UF();
    idx.clear(), key_.clear();
  }

  // x と同じ成分のキーを返す（x 自身も含む）
  std::vector<K> group(const K& x) {
    std::vector<int> g = uf.group(id(x));
    std::vector<K> ret;
    ret.reserve(g.size());
    for (int i : g) ret.push_back(key_[i]);
    return ret;
  }

  // 連結成分ごとのキー。成分そのものの並び順は決めていない
  std::vector<std::vector<K>> groups() {
    std::vector<std::vector<K>> ret;
    for (const auto& g : uf.groups()) {
      std::vector<K> t;
      t.reserve(g.size());
      for (int i : g) t.push_back(key_[i]);
      ret.push_back(std::move(t));
    }
    return ret;
  }
};
