#if !defined __SEGTREE_H
#define __SEGTREE_H


#include <iostream>
#include <vector>
#include <utility>
#include <algorithm>
#include <stdio.h>
#include <assert.h>

#include "types.h"
#include "utils.h"

using namespace std;


class SegmentTree {
    vpui_t repr;
    uint_t len;
    // For each element, first is the max. value under (and including
    // this node) and second is the index where this max. value occurs.

public:
    SegmentTree(uint_t _len = 0);

    void initialize(uint_t _len);

    void initialize(vui_t const& elems);

    pui_t _init(uint_t ni, uint_t b, uint_t e, vui_t const& elems);

    pui_t _query_max(uint_t ni, uint_t b, uint_t e, uint_t qf, uint_t ql);

    // qf & ql are indexes; both inclusive.
    // first -> value, second -> index
    pui_t query_max(uint_t qf, uint_t ql);
    pui_t naive_query_max(vui_t const& v, int i, int j);
  
};




#endif // __SEGTREE_H
