// -*- mode:c++; c-basic-offset:4 -*-
#if !defined __BENDERRMQ_H
#define __BENDERRMQ_H

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
#include "sparsetable.h"
#include "lookuptables.h"
#include "simplefixedobjectallocator.h"


using namespace std;

#define MIN_SIZE_FOR_BENDER_RMQ 16

class BenderRMQ {
    /* For inputs < MIN_SIZE_FOR_BENDER_RMQ in size, we use just the
     * sparse table.
     *
     * For inputs > MIN_SIZE_FOR_BENDER_RMQ in size, we use perform a
     * Euler Tour of the input and potentially blow it up to 2x. Let
     * the size of the blown up input be 'n' elements. We use 'st' for
     * 2n/lg n of the elements and for each block of size (1/2)lg n,
     * we use 'lt'. Since 'n' can be at most 2^32, (1/2)lg n can be at
     * most 16.
     *
     */
    SparseTable st;
    LookupTables lt;

    /* The data after euler tour computation (for +-RMQ) */
    vui_t euler;

    /* mapping stores the mapping of original indexes to indexes
     * within our re-written (using euler tour) structure).
     */
    vui_t mapping;

    /* Stores the bitmask corresponding to a block of size (1/2)(lg n) */
    vui_t table_map;

    /* Stores the mapping from +-RMQ indexes to actual indexes */
    vui_t rev_mapping;

    /* The real length of input that the user gave us */
    uint_t len;

    int lgn_by_2;
    int _2n_lgn;

public:

      
      
    std::string toGraphViz(BinaryTreeNode* par, BinaryTreeNode *n);

    /* This is a destructive function - one which deletes the tree rooted
    * at node n
    */
    void euler_tour(BinaryTreeNode *n, 
              vui_t &output, /* Where the output is written. Should be empty */
              vui_t &levels, /* Where the level for each node is written. Should be empty */
              vui_t &mapping /* mapping stores representative
                                indexes which maps from the original index to the index
                                into the euler tour array, which is a +- RMQ */, 
              vui_t &rev_mapping /* Reverse mapping to go from +-RMQ
                                    indexes to user provided indexes */, 
              int level = 1);
    BinaryTreeNode* make_cartesian_tree(vui_t const &input, SimpleFixedObjectAllocator<BinaryTreeNode> &alloc);
    void initialize(vui_t const& elems);

    // qf & ql are indexes; both inclusive.
    // Return: first -> value, second -> index
    pui_t query_max(uint_t qf, uint_t ql);
    pui_t naive_query_max(vui_t const& v, int i, int j);
};


#endif // __BENDERRMQ_H
