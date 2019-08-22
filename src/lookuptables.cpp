#include "lookuptables.h"

using namespace std;

void LookupTables::initialize(int nbits) 
{
    int ntables = 1 << nbits;
    repr.resize(ntables);
    std::vector<int> tmp(nbits);

    for (int i = 0; i < ntables; ++i) {
        const int start = 40;
        int diff = 0;
        for (int j = 0; j < nbits; ++j) {
            const int mask = 1L << j;
            if (i & mask) {
                diff += 1;
            } else {
                diff -= 1;
            }
            tmp[j] = start + diff;
        }

        // Always perform a lookup with the lower index
        // first. i.e. table[3][5] and NOT table[5][3]. Never
        // lookup with the same index on both dimenstion (for
        // example: table[3][3]).
        char_array_2d_t table(nbits-1, vc_t(nbits, -1));

        // Compute the lookup table for the bitmap in 'i'.
        for (int r = 0; r < nbits-1; ++r) {
            int maxi = r;
            int maxv = tmp[r];

            for (int c = r+1; c < nbits; ++c) {
                if (tmp[c] > maxv) {
                    maxv = tmp[c];
                    maxi = c;
                }
                table[r][c] = maxi;

            } // for(c)

        } // for (r)

        repr[i].swap(table);

    } // for (i)
}

/* Return the index of the largest element in the range [l..u]
  * (both inclusive) within the lookup table at index 'index'.
  */
uint_t LookupTables::query_max(uint_t index, uint_t l, uint_t u) 
{
    assert_le(l, u);
    assert_lt(index, repr.size());
    assert_lt(l, repr[0].size() + 1);
    assert_lt(u, repr[0][0].size());

    if (u == l) {
        return u;
    }

    return repr[index][l][u];
}

void LookupTables::show_tables() 
{
    if (repr.empty()) {
        return;
    }
    int nr = repr[0].size();
    int nc = repr[0][0].size();
    for (uint_t i = 0; i < repr.size(); ++i) {
        DPRINTF("Bitmap: %s\n", bitmap_str(i).c_str());
        DPRINTF("   |");

        for (int c = 0; c < nc; ++c) {
            DPRINTF("%3d ", c);
            if (c+1 != nc) {
                DPRINTF("|");
            }
        }
        DPRINTF("\n");
        for (int r = 0; r < nr; ++r) {
            DPRINTF("%2d |", r);
            for (int c = 0; c < nc; ++c) {
                DPRINTF("%3d ", (r<c ? repr[i][r][c] : -1));
                if (c+1 != nc) {
                    DPRINTF("|");
                }
            }
            DPRINTF("\n");
        }
    }
}

    