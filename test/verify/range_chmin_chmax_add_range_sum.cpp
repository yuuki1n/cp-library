// avl_segtree の verify 用（Segment Tree Beats）。Library Checker に提出するコード。
// https://judge.yosupo.jp/problem/range_chmin_chmax_add_range_sum
//   g++ -std=gnu++20 -O2 -I../.. FILE.cpp
#include <algorithm>
#include <cstdio>
#include <vector>

#include "../../data_structure/avl_segtree.hpp"

using ll = long long;

// |a_i| <= 10^12 なので、番兵は足し算しても溢れない大きさにしておく
constexpr ll INF = 2e18;

// 区間 chmin / chmax を入れるので、最大・最小まわりを一式持つ。
// mx2 は「最大値と異なる最大の値」、cmx は最大値の個数（min 側も同じ）
struct S : avl_value {
  ll sum = 0;
  ll mx = -INF, mx2 = -INF;
  ll mn = INF, mn2 = INF;
  int cmx = 0, cmn = 0;
};

S op(S a, S b) {
  S r;
  r.sum = a.sum + b.sum;
  if (a.mx > b.mx) r.mx = a.mx, r.cmx = a.cmx, r.mx2 = std::max(a.mx2, b.mx);
  else if (a.mx < b.mx) r.mx = b.mx, r.cmx = b.cmx, r.mx2 = std::max(a.mx, b.mx2);
  else r.mx = a.mx, r.cmx = a.cmx + b.cmx, r.mx2 = std::max(a.mx2, b.mx2);
  if (a.mn < b.mn) r.mn = a.mn, r.cmn = a.cmn, r.mn2 = std::min(a.mn2, b.mn);
  else if (a.mn > b.mn) r.mn = b.mn, r.cmn = b.cmn, r.mn2 = std::min(a.mn, b.mn2);
  else r.mn = a.mn, r.cmn = a.cmn + b.cmn, r.mn2 = std::min(a.mn2, b.mn2);
  return r;
}
// 空の値。op の単位元になっている（-INF は必ず負け、INF は必ず勝つ）
S e() { return S{}; }

// x -> min(max(x + add, lo), hi) の順で作用させる
struct F {
  ll add = 0, lo = -INF, hi = INF;
};

S mapping(F f, S x) {
  if (x.sz == 0) return x;
  if (f.add != 0) {
    x.sum += f.add * x.sz;
    x.mx += f.add;
    x.mn += f.add;
    if (x.mx2 != -INF) x.mx2 += f.add;
    if (x.mn2 != INF) x.mn2 += f.add;
  }
  if (f.lo > x.mn) {  // chmax。最小値だけが上がるなら、まとめて適用できる
    if (f.lo >= x.mn2) {
      x.fail = true;
      return x;
    }
    x.sum += (f.lo - x.mn) * x.cmn;
    if (x.mx == x.mn) x.mx = f.lo;  // 全部同じ値だった
    else if (x.mx2 == x.mn) x.mx2 = f.lo;
    x.mn = f.lo;
  }
  if (f.hi < x.mx) {  // chmin。最大値だけが下がるなら、まとめて適用できる
    if (f.hi <= x.mx2) {
      x.fail = true;
      return x;
    }
    x.sum -= (x.mx - f.hi) * x.cmx;
    if (x.mn == x.mx) x.mn = f.hi;
    else if (x.mn2 == x.mx) x.mn2 = f.hi;
    x.mx = f.hi;
  }
  return x;
}

// clamp を 2 段重ねても 1 段にまとめられる:
//   clamp(clamp(t, a, b), c, d) = clamp(t, clamp(a, c, d), clamp(b, c, d))
// これを使って「g のあと f」を 1 つの (add, lo, hi) にする
static ll clamp_(ll t, ll a, ll b) { return std::min(std::max(t, a), b); }
F composition(F f, F g) {
  F r;
  r.add = g.add + f.add;
  r.lo = clamp_(g.lo == -INF ? -INF : g.lo + f.add, f.lo, f.hi);
  r.hi = clamp_(g.hi == INF ? INF : g.hi + f.add, f.lo, f.hi);
  return r;
}
F id() { return F{}; }

// 和・最大・最小はどれも並び順に依らないので rev は既定のまま
using tree = avl_segtree<S, op, e, F, mapping, composition, id>;

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
ll read_int() {
  int c = getc_();
  while (c != '-' && (c < '0' || c > '9')) c = getc_();
  bool neg = c == '-';
  if (neg) c = getc_();
  ll x = 0;
  while (c >= '0' && c <= '9') {
    x = x * 10 + (c - '0');
    c = getc_();
  }
  return neg ? -x : x;
}
}  // namespace

int main() {
  int n = (int)read_int(), q = (int)read_int();
  std::vector<S> a(n);
  for (int i = 0; i < n; i++) {
    ll v = read_int();
    a[i].sum = a[i].mx = a[i].mn = v;
    a[i].cmx = a[i].cmn = 1;
  }
  tree t(a);

  std::vector<char> out;
  out.reserve(1 << 22);
  auto put = [&](ll x) {
    if (x < 0) out.push_back('-'), x = -x;
    char tmp[20];
    int k = 0;
    do {
      tmp[k++] = char('0' + x % 10);
      x /= 10;
    } while (x);
    while (k) out.push_back(tmp[--k]);
    out.push_back('\n');
  };

  while (q--) {
    int type = (int)read_int();
    int l = (int)read_int(), r = (int)read_int();
    if (type == 0) t.apply(l, r, F{0, -INF, read_int()});
    else if (type == 1) t.apply(l, r, F{0, read_int(), INF});
    else if (type == 2) t.apply(l, r, F{read_int(), -INF, INF});
    else put(t.prod(l, r).sum);
  }
  fwrite(out.data(), 1, out.size(), stdout);
  return 0;
}
