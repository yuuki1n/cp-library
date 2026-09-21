// rollback_union_find の verify 用（Offline Dynamic Connectivity）。
// https://judge.yosupo.jp/problem/dynamic_graph_vertex_add_component_sum
//   g++ -std=gnu++20 -O2 -I../.. FILE.cpp
//
// 辺が消えるので、時刻を葉とするセグメント木に「その辺が生きている区間」を載せ、
// 木を DFS しながら merge と undo を往復する。成分ごとの和は merge / undo の
// コールバックで自分で維持する。頂点への加算も「時刻 t 以降ずっと」の区間として
// 同じ木に載せると、巻き戻しが LIFO で噛み合う。
#include <algorithm>
#include <cstdio>
#include <unordered_map>
#include <vector>

#include "../../graph/union_find/union_find.hpp"

using ll = long long;

namespace {
char buf[1 << 16];
int bl = 0, br = 0;
int getc_() {
  if (bl == br) {
    br = (int)fread(buf, 1, sizeof(buf), stdin);
    bl = 0;
    if (br <= 0) return -1;
  }
  return buf[bl++];
}
ll read_uint() {
  int c = getc_();
  while (c < '0') c = getc_();
  ll x = 0;
  while (c >= '0') x = x * 10 + (c - '0'), c = getc_();
  return x;
}
std::vector<char> out;
void put(ll x) {
  char tmp[20];
  int k = 0;
  do tmp[k++] = char('0' + x % 10), x /= 10;
  while (x);
  while (k) out.push_back(tmp[--k]);
  out.push_back('\n');
}
}  // namespace

int main() {
  int n = (int)read_uint(), q = (int)read_uint();
  std::vector<ll> a(n);
  for (int i = 0; i < n; i++) a[i] = read_uint();

  // 出来事: 前半が辺、後半が頂点への加算。どちらも [l, r) の区間を持つ
  std::vector<int> eu, ev;                 // 辺の両端
  std::vector<int> el, er;                 // 辺が生きている区間
  std::vector<int> au;                     // 加算する頂点
  std::vector<ll> ax;                      // 加算する値
  std::vector<int> al;                     // 加算が効き始める時刻
  std::vector<int> qv(q, -1);              // 時刻 t の質問の頂点（質問でなければ -1）
  std::unordered_map<ll, int> alive;       // 張られている辺 -> 何番目の辺か
  alive.reserve(q * 2);

  for (int t = 0; t < q; t++) {
    int type = (int)read_uint();
    if (type == 0 || type == 1) {
      int u = (int)read_uint(), v = (int)read_uint();
      if (u > v) std::swap(u, v);
      ll key = (ll)u * n + v;
      if (type == 0) {
        alive[key] = (int)eu.size();
        eu.push_back(u), ev.push_back(v), el.push_back(t), er.push_back(q);
      } else {
        er[alive[key]] = t;
        alive.erase(key);
      }
    } else if (type == 2) {
      int v = (int)read_uint();
      au.push_back(v), ax.push_back(read_uint()), al.push_back(t);
    } else {
      qv[t] = (int)read_uint();
    }
  }

  int E = (int)eu.size(), A = (int)au.size();
  int sz = 1;
  while (sz < q) sz <<= 1;

  // 区間 [l, r) が覆うセグメント木の節点をたどる
  auto walk = [&](int l, int r, auto f) {
    for (l += sz, r += sz; l < r; l >>= 1, r >>= 1) {
      if (l & 1) f(l++);
      if (r & 1) f(--r);
    }
  };
  // 節点ごとに vector を持つと 100 万個になるので、2 回たどって平坦な配列に詰める
  std::vector<int> st(2 * sz + 1, 0);
  auto each = [&](auto f) {
    for (int i = 0; i < E; i++) walk(el[i], er[i], [&](int nd) { f(nd, i); });
    for (int i = 0; i < A; i++) walk(al[i], q, [&](int nd) { f(nd, E + i); });
  };
  each([&](int nd, int) { st[nd + 1]++; });
  for (int i = 0; i < 2 * sz; i++) st[i + 1] += st[i];
  std::vector<int> pos(st.begin(), st.end() - 1), item(st[2 * sz]);
  each([&](int nd, int id) { item[pos[nd]++] = id; });

  rollback_union_find uf(n);
  std::vector<ll> sum = a;
  out.reserve(1 << 22);

  auto dfs = [&](auto&& self, int nd) -> void {
    int snap = uf.snapshot();
    int lo = st[nd], hi = st[nd + 1];
    for (int k = lo; k < hi; k++) {
      int id = item[k];
      if (id < E) uf.merge(eu[id], ev[id], [&](int to, int from) { sum[to] += sum[from]; });
      else sum[uf.leader(au[id - E])] += ax[id - E];
    }
    if (nd >= sz) {
      int t = nd - sz;
      if (t < q && qv[t] >= 0) put(sum[uf.leader(qv[t])]);
    } else {
      self(self, nd * 2), self(self, nd * 2 + 1);
    }
    // 値の加算を戻してから辺を巻き戻す。深い側は既に戻っているので leader は同じ
    for (int k = hi - 1; k >= lo; k--)
      if (item[k] >= E) sum[uf.leader(au[item[k] - E])] -= ax[item[k] - E];
    uf.rollback(snap, [&](int to, int from) { sum[to] -= sum[from]; });
  };
  dfs(dfs, 1);

  fwrite(out.data(), 1, out.size(), stdout);
  return 0;
}
