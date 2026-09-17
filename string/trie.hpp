#include <array>
#include <cassert>
#include <cstddef>
#include <string>
#include <vector>

/*
 * trie<SIGMA, OFFSET> : 文字列のトライ木
 *
 *   add(s, d)        s を d 本追加（d = -1 で 1 本削除）。終端ノード番号を返す
 *   erase(s)         add(s, -1)。入っていない文字列を消さないこと
 *   count(s)         s がちょうど何本あるか
 *   count_prefix(s)  s を接頭辞に持つ文字列の本数
 *   contains(s)      count(s) > 0
 *   find(s)          たどった先のノード番号。途中で切れたら -1
 *   path(s)          たどって通ったノードの列。先頭は根。長さ - 1 が一致文字数
 *   size() / node_count() / empty() / clear()
 *   root() / child(v, c) / cnt(v) / ends(v)    木を自分でたどる用
 *
 *   長さ L として add / find / path が O(L)、他は O(1)。
 *   SIGMA は文字の種類数（＝子の本数）。既定は英小文字 26。
 *   文字は c - OFFSET で番号に直す。連続しない文字集合は自分で
 *   0..SIGMA-1 に直して vector<int> 版に渡す。
 *   メモリは ノード数 x (4 * SIGMA + 8) バイト。SIGMA = 26 で 112 バイト、
 *   長さ 10 を 2*10^5 本入れると約 150 MB。長い文字列を大量に入れるなら要注意。
 *   削除してもノードは残る。減らすなら clear()。
 *
 * 使用例:
 *   trie<> t;
 *   t.add("apple");
 *   t.count_prefix("ap");
 *   len(t.path(s)) - 1;              // 辞書のどれかと何文字一致するか
 *   fore(v, t.path(s)) print(t.cnt(v));
 *
 *   trie<2> u;                       // 番号で渡す
 *   u.add(vi{0, 1, 1, 0});
 *
 * verify:
 *   (未 verify)
 */
template <int SIGMA = 26, char OFFSET = 'a'> struct trie {
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
