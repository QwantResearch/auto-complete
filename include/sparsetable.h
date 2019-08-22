// -*- mode:c++; c-basic-offset:4 -*-
#if !defined __SPARSETABLE_H
#define __SPARSETABLE_H

#include <iostream>
#include <vector>
#include <utility>
#include <algorithm>
#include <stdio.h>
#include <assert.h>

#include "types.h"
#include "utils.h"

using namespace std;


class SparseTable {
    /* For each element in repr, we store just the index of the MAX
     * element in data.
     *
     * i.e. If the MAX element in the index range [10..17] is at index
     * 14, then repr[3][10] will contain the value 14.
     *
     * repr[X] stores MAX indexes for blocks of length (1<<X == 2^X).
     *
     */
    vui_t data;
    vvui_t repr;
    uint_t len;

public:

    void initialize(vui_t const& elems);

    // qf & ql are indexes; both inclusive.
    // first -> value, second -> index
    pui_t query_max(uint_t qf, uint_t ql);
    pui_t  naive_query_max(vui_t const& v, int i, int j);
};


#endif // __SPARSETABLE_H
