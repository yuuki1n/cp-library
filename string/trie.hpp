#include <array>
#include <cassert>
#include <cstddef>
#include <string>
#include <vector>

/*
 * trie<SIGMA, OFFSET> : 文字列のトライ木
 *
 *   SIGMA は文字の種類数（＝子の本数）。
 *   SIGMA 種類の文字からなる文字列を貯めて、「その文字列が何本あるか」と
 *   「その文字列を接頭辞に持つ文字列が何本あるか」を長さぶんの時間で答える。
 *   同じ文字列を何本入れてもよい。削除もできる。
 *   既定は英小文字 26 種類。trie<2, '0'> なら 01 文字列になる。
 *
 *   add(s)           s を 1 本足す。終端ノードの番号を返す
 *   add(s, d)        s を d 本追加する（d = -1 で 1 本削除）
 *   erase(s)         add(s, -1) と同じ。入っていない文字列を消してはいけない
 *   count(s)         s がちょうど何本入っているか
 *   count_prefix(s)  s を接頭辞に持つ文字列が何本入っているか
 *   contains(s)      count(s) > 0
 *   find(s)          s をたどった先のノード番号。途中で切れたら -1
 *   path(s)          根から s をたどって通ったノードの列。先頭は必ず根。
 *                    途中で切れたらそこまで。長さ - 1 が一致した文字数
 *   size()           入っている文字列の総数
 *   node_count()     ノード数（根を含む）
 *   empty() / clear()
 *
 *   木を自分でたどるとき用:
 *     root()         根のノード番号（= 0）
 *     child(v, c)    ノード v の文字 c（0..SIGMA-1）の子。無ければ -1
 *     cnt(v)         v を通る文字列の本数
 *     ends(v)        v で終わる文字列の本数
 *
 *   文字列版は文字 c を c - OFFSET で番号に直す。連続していない文字集合を
 *   使うときは、自分で 0..SIGMA-1 に直して vector<int> 版に渡す。
 *
 *   計算量は文字列長を L として add / find / path が O(L)、その他は O(1)。
 *
 *   ノード数は最大「入れた文字列の長さの合計 + 1」（共通接頭辞のぶん減る）。
 *   メモリは おおよそ ノード数 x (4 * SIGMA + 8) バイト。
 *     SIGMA = 26 なら 1 ノード 112 バイト。長さ 10 を 2*10^5 本で約 150 MB
 *     SIGMA =  2 なら 1 ノード  16 バイト
 *   SIGMA が大きいと重いので、長い文字列を大量に入れるときは見積もること。
 *
 *   削除した文字列のノードは残る（番号が変わらないようにするため）。
 *   ノード数を減らしたいときは clear() で作り直す。
 *
 *   入れていない文字列を erase してはいけない。無いノードを作ったうえで
 *   本数が負になり、以降の count / count_prefix が壊れる。
 *
 * 使用例:
 *   trie<> t;                      // 英小文字
 *   t.add("apple");
 *   t.add("apple");
 *   t.add("apricot");
 *   t.count("apple");              // 2
 *   t.count_prefix("ap");          // 3
 *   t.count_prefix("app");         // 2
 *   t.erase("apple");
 *   t.count("apple");              // 1
 *
 *   trie<2, 'o'> b;                // 'o' と 'p' ... 連続していない文字は不可
 *   trie<2> u;                     // 番号で渡す
 *   u.add(vi{0, 1, 1, 0});
 *
 *   // 辞書のどれかと何文字一致するか
 *   len(t.path(s)) - 1;
 *
 *   // 各接頭辞が何本あるか
 *   fore(v, t.path(s)) print(t.cnt(v));
 *
 *   // 根から 1 本の道をたどる（path を使わない書き方）
 *   int v = t.root();
 *   for (char c : s) {
 *     v = t.child(v, c - 'a');
 *     if (v < 0) break;
 *     print(t.cnt(v));
 *   }
 *
 * verify:
 *   (未 verify)
 */
template <int SIGMA = 26, char OFFSET = 'a'>
struct trie {
 private:
  struct node {
    std::array<int, std::size_t(SIGMA)> ch;  // SIGMA は int なので明示的に直す
    int through = 0;                         // このノードを通る文字列の本数
    int terminal = 0;                        // ここで終わる文字列の本数
    node() { ch.fill(-1); }
  };
  std::vector<node> nodes;

 public:
  trie() { nodes.emplace_back(); }

  int root() const { return 0; }
  int size() const { return nodes[0].through; }
  int node_count() const { return (int)nodes.size(); }
  bool empty() const { return size() == 0; }
  void clear() {
    nodes.clear();
    nodes.emplace_back();
  }

  int child(int v, int c) const {
    assert(0 <= v && v < node_count());
    assert(0 <= c && c < SIGMA);
    return nodes[v].ch[c];
  }
  int cnt(int v) const { return v < 0 ? 0 : nodes[v].through; }
  int ends(int v) const { return v < 0 ? 0 : nodes[v].terminal; }

  // s を d 本追加する（d = -1 で 1 本削除）。終端ノードの番号を返す
  int add(const std::vector<int>& s, int d = 1) {
    int v = 0;
    nodes[0].through += d;
    for (int c : s) {
      assert(0 <= c && c < SIGMA);
      if (nodes[v].ch[c] < 0) {
        int nx = (int)nodes.size();
        nodes.emplace_back();  // ここで再確保されうるので
        nodes[v].ch[c] = nx;   // 参照を持ち越さず、番号で書き戻す
      }
      v = nodes[v].ch[c];
      nodes[v].through += d;
    }
    nodes[v].terminal += d;
    return v;
  }
  int add(const std::string& s, int d = 1) { return add(to_index(s), d); }

  int erase(const std::vector<int>& s) { return add(s, -1); }
  int erase(const std::string& s) { return add(s, -1); }

  // s をたどった先のノード番号。途中で切れたら -1
  int find(const std::vector<int>& s) const {
    int v = 0;
    for (int c : s) {
      v = child(v, c);
      if (v < 0) return -1;
    }
    return v;
  }
  int find(const std::string& s) const { return find(to_index(s)); }

  // 根から s をたどって通ったノードを順に返す。先頭は必ず根。
  // 途中で切れたらそこまで。戻り値の長さ - 1 が「一致した文字数」になる
  std::vector<int> path(const std::vector<int>& s) const {
    std::vector<int> q;
    q.reserve(s.size() + 1);
    int v = 0;
    q.push_back(v);
    for (int c : s) {
      v = child(v, c);
      if (v < 0) break;
      q.push_back(v);
    }
    return q;
  }
  std::vector<int> path(const std::string& s) const {
    return path(to_index(s));
  }

  int count(const std::vector<int>& s) const { return ends(find(s)); }
  int count(const std::string& s) const { return ends(find(s)); }
  int count_prefix(const std::vector<int>& s) const { return cnt(find(s)); }
  int count_prefix(const std::string& s) const { return cnt(find(s)); }
  bool contains(const std::vector<int>& s) const { return count(s) > 0; }
  bool contains(const std::string& s) const { return count(s) > 0; }

  static std::vector<int> to_index(const std::string& s) {
    std::vector<int> r(s.size());
    for (std::size_t i = 0; i < s.size(); i++) r[i] = s[i] - OFFSET;
    return r;
  }
};
