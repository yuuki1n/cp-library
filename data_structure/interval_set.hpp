#include <algorithm>
#include <iterator>
#include <map>
#include <utility>

/*
 * IntervalSet : 半開区間 [l, r) の集合
 *
 *   互いに交わらず隣接もしない正規形で保持する。つまり [0,2) と [2,5) を
 *   入れると [0,5) 1 本にまとまる（整数の集合として同じものなので）。
 *   内部は map<l, r>。1 回の操作で触れた区間の数を k として O(k log n)。
 *   ならせば insert / erase とも O(log n)。
 *
 *   size()        覆っている整数の個数（区間の本数は intervals().size()）
 *   insert(l, r)  [l, r) を追加し、新たに覆った個数を返す
 *   insert(x)     insert(x, x + 1) と同じ
 *   erase(l, r)   [l, r) を取り除き、実際に取り除いた個数を返す
 *   erase(x)      erase(x, x + 1) と同じ
 *   find(x)       x を含む区間のイテレータ。無ければ intervals().end()
 *   contains(x)   x が覆われているか
 *   same(x, y)    x と y が同じ区間に属するか
 *   mex(x)        x 以上で覆われていない最小の値（既定は x = 0）
 *   intervals()   区間の map への const 参照。走査・出力用
 *
 *   l >= r の呼び出しは何もせず 0 を返す。
 *
 *   座標の制限:
 *     - 座標は LLONG_MAX 未満であること。insert(x) は insert(x, x + 1) な
 *       ので、x = LLONG_MAX だと x + 1 が溢れて何も起きない。
 *     - 覆う総数が LLONG_MAX を超えると size() が溢れる。全体幅を
 *       9.2 * 10^18 未満に収めること。
 *
 * 使用例:
 *   IntervalSet s;
 *   s.insert(1, 5);                 // 4 が返る
 *   s.insert(5, 8);                 // 3。[1,8) に併合される
 *   s.erase(3, 4);                  // 1。[1,3) と [4,8) に分かれる
 *   s.contains(3);                  // false
 *   s.mex();                        // 0
 *   fore(t, s.intervals()) print(t.fi, t.se);
 *   print(s.intervals());           // 1 行 1 区間で出力される
 *
 * verify:
 *   (未 verify)
 */
struct IntervalSet {
 private:
  std::map<long long, long long> mp;  // l -> r （半開区間 [l, r)）
  long long sz = 0;                   // 覆っている整数の総数

 public:
  using const_iterator = std::map<long long, long long>::const_iterator;

  // 区間の map。走査と出力に使う（書き換えると sz と食い違うので const）
  const std::map<long long, long long>& intervals() const { return mp; }

  long long size() const { return sz; }
  bool empty() const { return sz == 0; }
  void clear() {
    mp.clear();
    sz = 0;
  }

  // x を含む区間のイテレータ。無ければ intervals().end()
  const_iterator find(long long x) const {
    auto it = mp.upper_bound(x);
    if (it == mp.begin()) return mp.end();
    --it;
    return x < it->second ? it : mp.end();
  }
  bool contains(long long x) const { return find(x) != mp.end(); }
  bool same(long long x, long long y) const {
    auto a = find(x);
    return a != mp.end() && a == find(y);
  }

  // [l, r) を追加する。新たに覆った整数の個数を返す
  long long insert(long long l, long long r) {
    if (l >= r) return 0;
    long long before = sz;
    auto it = mp.upper_bound(l);
    // 左隣が届いていれば（隣接も含めて）吸収する
    if (it != mp.begin() && std::prev(it)->second >= l) {
      --it;
      l = std::min(l, it->first);
      r = std::max(r, it->second);
      sz -= it->second - it->first;
      it = mp.erase(it);
    }
    // 右側に重なる／隣接する区間を吸収する
    while (it != mp.end() && it->first <= r) {
      r = std::max(r, it->second);
      sz -= it->second - it->first;
      it = mp.erase(it);
    }
    mp.emplace(l, r);
    sz += r - l;
    return sz - before;
  }
  long long insert(long long x) { return insert(x, x + 1); }

  // [l, r) を取り除く。実際に取り除いた整数の個数を返す
  long long erase(long long l, long long r) {
    if (l >= r) return 0;
    long long before = sz;
    auto it = mp.upper_bound(l);
    // l をまたぐ区間があれば、はみ出した部分を残して切る
    if (it != mp.begin() && std::prev(it)->second > l) {
      auto pv = std::prev(it);
      long long pl = pv->first, pr = pv->second;
      sz -= pr - pl;
      mp.erase(pv);
      if (pl < l) {
        mp.emplace(pl, l);
        sz += l - pl;
      }
      if (r < pr) {  // [l, r) が丸ごと内側だったので右側も残して終わり
        mp.emplace(r, pr);
        sz += pr - r;
        return before - sz;
      }
    }
    // [l, r) と重なる残りを取り除く
    it = mp.lower_bound(l);
    while (it != mp.end() && it->first < r) {
      long long cr = it->second;
      sz -= cr - it->first;
      it = mp.erase(it);
      if (r < cr) {  // 右にはみ出した部分を残す
        mp.emplace(r, cr);
        sz += cr - r;
        break;
      }
    }
    return before - sz;
  }
  long long erase(long long x) { return erase(x, x + 1); }

  // x 以上で覆われていない最小の値
  long long mex(long long x = 0) const {
    auto it = find(x);
    return it == mp.end() ? x : it->second;
  }
};
