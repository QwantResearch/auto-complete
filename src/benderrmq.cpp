#include "benderrmq.h"

using namespace std;

  
std::string BenderRMQ::toGraphViz(BinaryTreeNode* par, BinaryTreeNode *n) 
{
    if (!n) {
        return "";
    }

    std::ostringstream sout;

    if (par) {
        sout<<'"'<<par->data<<"_"<<par->index<<"\" -> \""<<n->data<<"_"<<n->index<<"\"\n";
    }
    sout<<toGraphViz(n, n->left);
    sout<<toGraphViz(n, n->right);
    return sout.str();
}

/* This is a destructive function - one which deletes the tree rooted
* at node n
*/
void BenderRMQ::euler_tour(BinaryTreeNode *n, 
          vui_t &output, /* Where the output is written. Should be empty */
          vui_t &levels, /* Where the level for each node is written. Should be empty */
          vui_t &mapping /* mapping stores representative
                            indexes which maps from the original index to the index
                            into the euler tour array, which is a +- RMQ */, 
          vui_t &rev_mapping /* Reverse mapping to go from +-RMQ
                                indexes to user provided indexes */, 
          int level) 
{
    DPRINTF("euler_tour(%d, %d)\n", n?n->data:-1, n?n->index:-1);
    if (!n) {
        return;
    }
    output.push_back(n->data);
    mapping[n->index] = output.size() - 1;
    DPRINTF("mapping[%d] = %d\n", n->index, mapping[n->index]);
    rev_mapping.push_back(n->index);
    levels.push_back(level);
    if (n->left) {
        euler_tour(n->left, output, levels, mapping, rev_mapping, level+1);
        output.push_back(n->data);
        rev_mapping.push_back(n->index);
        levels.push_back(level);
    }
    if (n->right) {
        euler_tour(n->right, output, levels, mapping, rev_mapping, level+1);
        output.push_back(n->data);
        rev_mapping.push_back(n->index);
        levels.push_back(level);
    }
    // We don't delete the node here since the clear() function on the
    // SimpleFixedObjectAllocator<BinaryTreeNode> will take care of
    // cleaning up the associated memory.
    //
    // delete n;
}

BinaryTreeNode* BenderRMQ::make_cartesian_tree(vui_t const &input, SimpleFixedObjectAllocator<BinaryTreeNode> &alloc) 
{
    BinaryTreeNode *curr = NULL;
    std::stack<BinaryTreeNode*> stk;

    if (input.empty()) {
        return NULL;
    }

    for (uint_t i = 0; i < input.size(); ++i) {
        curr = alloc.get();
        new (curr) BinaryTreeNode(input[i], i);
        DPRINTF("ct(%d, %d)\n", curr->data, curr->index);

        if (stk.empty()) {
            stk.push(curr);
            DPRINTF("[1] stack top (%d, %d)\n", curr->data, curr->index);
        } else {
            if (input[i] <= stk.top()->data) {
                // Just add it
                stk.push(curr);
                DPRINTF("[2] stack top (%d, %d)\n", curr->data, curr->index);
            } else {
                // Back up till we are the largest node on the stack
                BinaryTreeNode *top = NULL;
                BinaryTreeNode *prev = NULL;
                while (!stk.empty() && stk.top()->data < input[i]) {
                    prev = top;
                    top = stk.top();
                    DPRINTF("[1] popping & setting (%d, %d)->right = (%d, %d)\n", top->data, top->index, 
                            prev?prev->data:-1, prev?prev->index:-1);
                    top->right = prev;
                    stk.pop();
                }
                curr->left = top;
                DPRINTF("(%d, %d)->left = (%d, %d)\n", curr->data, curr->index, top->data, top->index);
                stk.push(curr);
                DPRINTF("stack top is now (%d, %d)\n", curr->data, curr->index);
            }
        }
    }

    assert(!stk.empty());
    BinaryTreeNode *top = NULL;
    BinaryTreeNode *prev = NULL;
    while (!stk.empty()) {
        prev = top;
        top = stk.top();
        DPRINTF("[2] popping & setting (%d, %d)->right = (%d, %d)\n", top->data, top->index, 
                prev?prev->data:-1, prev?prev->index:-1);
        top->right = prev;
        stk.pop();
    }
    DPRINTF("returning top = (%d, %d)\n", top->data, top->index);

    return top;
}

void BenderRMQ::initialize(vui_t const& elems) 
{
    len = elems.size();

    if (len < MIN_SIZE_FOR_BENDER_RMQ) {
        st.initialize(elems);
        return;
    }

    vui_t levels;
    SimpleFixedObjectAllocator<BinaryTreeNode> alloc(len);

    euler.reserve(elems.size() * 2);
    mapping.resize(elems.size());
    BinaryTreeNode *root = make_cartesian_tree(elems, alloc);

    DPRINTF("GraphViz (paste at: http://ashitani.jp/gv/):\n%s\n", toGraphViz(NULL, root).c_str());

    euler_tour(root, euler, levels, mapping, rev_mapping);

    root = NULL; // This tree has now been deleted
    alloc.clear();

    assert_eq(levels.size(), euler.size());
    assert_eq(levels.size(), rev_mapping.size());

    uint_t n = euler.size();
    lgn_by_2 = log2(n) / 2;
    _2n_lgn  = n / lgn_by_2 + 1;

    DPRINTF("n = %u, lgn/2 = %d, 2n/lgn = %d\n", n, lgn_by_2, _2n_lgn);
    lt.initialize(lgn_by_2);

    table_map.resize(_2n_lgn);
    vui_t reduced;

    for (uint_t i = 0; i < n; i += lgn_by_2) {
        uint_t max_in_block = euler[i];
        int bitmap = 1L;
        DPRINTF("Sequence: (%u, ", euler[i]);
        for (int j = 1; j < lgn_by_2; ++j) {
            int curr_level, prev_level;
            uint_t value;
            if (i+j < n) {
                curr_level = levels[i+j];
                prev_level = levels[i+j-1];
                value = euler[i+j];
            } else {
                curr_level = 1;
                prev_level = 0;
                value = 0;
            }

            const uint_t bit = (curr_level < prev_level);
            bitmap |= (bit << j);
            max_in_block = std::max(max_in_block, value);
            DPRINTF("%u, ", value);
        }
        DPRINTF("), Bitmap: %s\n", bitmap_str(bitmap).c_str());
        table_map[i / lgn_by_2] = bitmap;
        reduced.push_back(max_in_block);
    }

    DPRINTF("reduced.size(): %u\n", reduced.size());
    st.initialize(reduced);
    DCERR("initialize() completed"<<endl);
}

// qf & ql are indexes; both inclusive.
// Return: first -> value, second -> index
pui_t BenderRMQ::query_max(uint_t qf, uint_t ql) 
{
    if (qf >= this->len || ql >= this->len || ql < qf) {
        return make_pair(minus_one, minus_one);
    }

    if (len < MIN_SIZE_FOR_BENDER_RMQ) {
        return st.query_max(qf, ql);
    }

    DPRINTF("[1] (qf, ql) = (%d, %d)\n", qf, ql);
    // Map to +-RMQ co-ordinates
    qf = mapping[qf];
    ql = mapping[ql];
    DPRINTF("[2] (qf, ql) = (%d, %d)\n", qf, ql);

    if (qf > ql) {
        std::swap(qf, ql);
        DPRINTF("[3] (qf, ql) = (%d, %d)\n", qf, ql);
    }

    // Determine whether we need to query 'st'.
    const uint_t first_block_index = qf / lgn_by_2;
    const uint_t last_block_index = ql / lgn_by_2;

    DPRINTF("first_block_index: %u, last_block_index: %u\n", 
            first_block_index, last_block_index);

    pui_t ret(0, 0);

    /* Main logic:
      *
      * [1] If the query is restricted to a single block, then
      * first_block_index == last_block_index, and we only need to
      * do a bitmap based lookup.
      *
      * [2] If last_block_index - first_block_index == 1, then the
      * query spans 2 blocks, and we do not need to lookup the
      * Sparse Table to get the summary max.
      *
      * [3] In all other cases, we need to take the maximum of 3
      * results, (a) the max in the suffix of the first block, (b)
      * the max in the prefix of the last block, and (c) the max of
      * all blocks between the first and the last block.
      *
      */

    if (last_block_index - first_block_index > 1) {
        // We need to perform an inter-block query using the 'st'.
        ret = st.query_max(first_block_index + 1, last_block_index - 1);

        // Now perform an in-block query to get the index of the
        // max value as it appears in 'euler'.
        const uint_t bitmapx = table_map[ret.second];
        const uint_t imax = lt.query_max(bitmapx, 0, lgn_by_2-1) + ret.second*lgn_by_2;
        ret.second = imax;
    } else if (first_block_index == last_block_index) {
        // The query is completely within a block.
        const uint_t bitmapx = table_map[first_block_index];
        DPRINTF("bitmapx: %s\n", bitmap_str(bitmapx).c_str());
        qf %= lgn_by_2;
        ql %= lgn_by_2;
        const uint_t imax = lt.query_max(bitmapx, qf, ql) + first_block_index*lgn_by_2;
        ret = make_pair(euler[imax], rev_mapping[imax]);
        return ret;
    }

    // Now perform an in-block query for the first and last
    // blocks.
    const uint_t f1 = qf % lgn_by_2;
    const uint_t f2 = lgn_by_2 - 1;

    const uint_t l1 = 0;
    const uint_t l2 = ql % lgn_by_2;

    const uint_t bitmap1 = table_map[first_block_index];
    const uint_t bitmap2 = table_map[last_block_index];

    DPRINTF("bitmap1: %s, bitmap2: %s\n", bitmap_str(bitmap1).c_str(),
            bitmap_str(bitmap2).c_str());

    uint_t max1i = lt.query_max(bitmap1, f1, f2);
    uint_t max2i = lt.query_max(bitmap2, l1, l2);

    DPRINTF("max1i: %u, max2i: %u\n", max1i, max2i);

    max1i += first_block_index * lgn_by_2;
    max2i += last_block_index * lgn_by_2;

    if (last_block_index - first_block_index > 1) {
        // 3-way max
        DPRINTF("ret: %u, max1: %u, max2: %u\n", ret.first, euler[max1i], euler[max2i]);
        if (ret.first > euler[max1i] && ret.first > euler[max2i]) {
            ret.second = rev_mapping[ret.second];
        } else if (euler[max1i] >= ret.first && euler[max1i] >= euler[max2i]) {
            ret = make_pair(euler[max1i], rev_mapping[max1i]);
        } else if (euler[max2i] >= ret.first && euler[max2i] >= euler[max1i]) {
            ret = make_pair(euler[max2i], rev_mapping[max2i]);
        }
    } else {
        // 2-way max
        if (euler[max1i] > euler[max2i]) {
            ret = make_pair(euler[max1i], rev_mapping[max1i]);
        } else {
            ret = make_pair(euler[max2i], rev_mapping[max2i]);
        }
    }

    return ret;
}
pui_t BenderRMQ::naive_query_max(vui_t const& v, int i, int j) 
{
    uint_t mv = v[i];
    uint_t mi = i;
    while (i <= j) {
        if (v[i] > mv) {
            mv = v[i];
            mi = i;
        }
        ++i;
    }
    return pui_t(mv, mi);
}
