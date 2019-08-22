// -*- mode:c++; c-basic-offset:4 -*-
#if !defined __LOOKUPTABLE_H
#define __LOOKUPTABLE_H

#include <iostream>
#include <vector>
#include <utility>
#include <algorithm>
#include <stack>
#include <stdio.h>
#include <sstream>
#include <assert.h>

#include "types.h"
#include "utils.h"

using namespace std;

class LookupTables {
    char_array_3d_t repr;

    /* The bitmaps are stored with the LSB always set as 1 and the LSB
     * signifying the sign of the element at array index 0. The MSB
     * represents the sign of the last element in the array relative
     * to the immediately previous element.
     */
public:
    void initialize(int nbits);

    /* Return the index of the largest element in the range [l..u]
     * (both inclusive) within the lookup table at index 'index'.
     */
    uint_t query_max(uint_t index, uint_t l, uint_t u);

    void show_tables();

};

#endif // __LOOKUPTABLE_H
