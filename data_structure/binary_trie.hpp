#include <array>
#include <cassert>
#include <vector>

/*
 * binary_trie<BITS, T> : 非負整数の多重集合。xor と k 番目が速い
 *
 *   0 以上 2^BITS 未満の整数を貯める。同じ値を何個入れてもよい。
 *   すべての要素に xor を掛ける操作が O(1) でできるのが特徴で、
 *   「xor したときの最小・最大」を求める問題で使う。
 *   BITS は扱うビット数（木の深さ）。子は常に 2 本なので分岐数ではない。
 *   既定は BITS = 30（10^9 まで）。10^18 まで使うなら binary_trie<60>。
 *
 *   insert(x)       x を 1 個足す
 *   insert(x, d)    x を d 個追加する（d = -1 で 1 個削除）
 *   erase(x)        insert(x, -1) と同じ。無い値を消してはいけない
 *   count(x)        x が何個入っているか
 *   contains(x)     count(x) > 0
 *   size()          要素数（重複を含む）
 *   empty() / clear()
 *
 *   kth(k, flip)        各要素を flip と xor した値のうち、小さいほうから k 番目
 *   min_value(flip)     kth(0, flip)
 *   max_value(flip)     kth(size() - 1, flip)
 *   count_less(ub, flip) flip と xor した値が ub 未満になる要素の個数
 *
 *   flip は省略でき、既定は 0（xor しない）。この 4 つは状態を変えないので
 *   const な binary_trie に対しても呼べる。
 *
 *   xor_all(v)      すべての要素を v と xor する。O(1)。こちらは状態が残る
 *
 *   計算量は insert / erase / count / kth / count_less が O(BITS)、
 *   xor_all が O(1)。ノード数は最大「入れた個数 × BITS + 1」。
 *
 *   削除した値のノードは残る。減らしたいときは clear() で作り直す。
 *   負の数は入れられない。入れたいときは下駄を履かせる。
 *
 * 使用例:
 *   binary_trie<> t;
 *   fore(a, A) t.insert(a);
 *   t.min_value();             // 最小値
 *   t.max_value(x);            // x と xor して最大になる値
 *   t.kth(2, x);               // x と xor した値のうち 3 番目に小さいもの
 *   t.count_less(10);          // 10 未満の個数
 *   t.count_less(10, x);       // x と xor した値が 10 未満の個数
 *
 *   // 数列の中から xor が最大になる 2 数を選ぶ
 *   binary_trie<> s;
 *   ll ans = 0;
 *   fore(a, A) {
 *     if (s.size()) chmax(ans, s.max_value(a));
 *     s.insert(a);
 *   }
 *
 *   t.xor_all(v);              // 以降ずっと v と xor された状態になる
 *   t.min_value();             // その状態での最小値
 *
 * verify:
 *   (未 verify)
 */
template <int BITS = 30, class T = long long>
struct binary_trie {
 private:
  std::vector<std::array<int, 2>> ch;
  std::vector<int> sz;
  T lz = 0;  // 全体に掛かっている xor

  int make() {
    ch.push_back({-1, -1});
    sz.push_back(0);
    return (int)ch.size() - 1;
  }
  static int bit_of(T x, int b) { return (int)((x >> b) & 1); }

 public:
  binary_trie() { make(); }

  int size() const { return sz[0]; }
  bool empty() const { return size() == 0; }
  int node_count() const { return (int)ch.size(); }
  void clear() {
    ch.clear();
    sz.clear();
    lz = 0;
    make();
  }

  // x を d 個追加する（d = -1 で 1 個削除）
  void insert(T x, int d = 1) {
    assert(0 <= x && (BITS >= (int)sizeof(T) * 8 || x < (T(1) << BITS)));
    T y =
        x ^ lz;  // 見え方が x になるように、掛かっている xor を打ち消して入れる
    int v = 0;
    sz[0] += d;
    for (int b = BITS - 1; b >= 0; b--) {
      int c = bit_of(y, b);
      if (ch[v][c] < 0) {
        int nx = make();  // push_back で再確保されうるので
        ch[v][c] = nx;    // 参照ではなく番号で書き戻す
      }
      v = ch[v][c];
      sz[v] += d;
    }
  }

  void erase(T x) { insert(x, -1); }

  int count(T x) const {
    T y = x ^ lz;
    int v = 0;
    for (int b = BITS - 1; b >= 0; b--) {
      v = ch[v][bit_of(y, b)];
      if (v < 0) return 0;
    }
    return sz[v];
  }

  bool contains(T x) const { return count(x) > 0; }

  // 各要素を flip と xor した値のうち、小さいほうから k 番目（0-indexed）
  // flip = 0 なら素直に k 番目に小さい要素
  T kth(int k, T flip = 0) const {
    assert(0 <= k && k < size());
    T xr = lz ^ flip;  // 全体に掛かっている xor と、今回の flip をまとめる
    int v = 0;
    T res = 0;
    for (int b = BITS - 1; b >= 0; b--) {
      int zero = bit_of(xr, b);  // この段で 0 に見えるのは zero 側の子
      int lo = ch[v][zero];
      int c = zero;
      if (lo < 0 || sz[lo] <= k) {  // 0 側が足りなければ 1 側へ
        if (lo >= 0) k -= sz[lo];
        c = zero ^ 1;
        res |= T(1) << b;
      }
      v = ch[v][c];
    }
    return res;
  }

  // flip と xor したときの最小・最大。flip = 0 なら素直な最小・最大
  T min_value(T flip = 0) const { return kth(0, flip); }
  T max_value(T flip = 0) const { return kth(size() - 1, flip); }

  // flip と xor した値が ub 未満になる要素の個数
  int count_less(T ub, T flip = 0) const {
    T xr = lz ^ flip;
    int v = 0, res = 0;
    for (int b = BITS - 1; b >= 0; b--) {
      if (v < 0) break;
      int zero = bit_of(xr, b);
      if (bit_of(ub, b)) {  // ub のこのビットが 1 なら 0 側は全部 ub 未満
        int lo = ch[v][zero];
        if (lo >= 0) res += sz[lo];
        v = ch[v][zero ^ 1];
      } else {
        v = ch[v][zero];
      }
    }
    return res;
  }

  // すべての要素を v と xor する
  void xor_all(T v) { lz ^= v; }
};
