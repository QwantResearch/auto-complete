#include "sparsetable.h"

void SparseTable::initialize(vui_t const& elems) {

    this->data = elems;
    this->len = elems.size();
    this->repr.clear();

    DCERR("len: "<<this->len<<endl);

    const size_t ntables = log2(this->len) + 1;
    this->repr.resize(ntables);

    DCERR("ntables: "<<ntables<<endl);

    this->repr[0].resize(this->len);
    for (size_t i = 0; i < this->len; ++i) {
        // This is the identity mapping, since the MAX element in
        // a range of length 1 is the element itself.
        this->repr[0][i] = i;
    }

    for (size_t i = 1; i < ntables; ++i) {
        /* The previous 'block size' */
        const uint_t pbs = 1<<(i-1);

        /* bs is the 'block size'. i.e. The number of elements
          * from the data that are used to computed the max value
          * and store it at repr[i][...].
          */
        const uint_t bs = 1<<i;

        /* The size of the vector at repr[i]. We need to resize it
          * to this size.
          */
        const size_t vsz = this->len - bs + 1;

        DCERR("starting i: "<<i<<" bs: "<<bs<<endl);

        this->repr[i].resize(vsz);

        // cerr<<"i: "<<i<<", vsz: "<<vsz<<endl;

        vui_t& curr = this->repr[i];
        vui_t& prev = this->repr[i - 1];

        for (size_t j = 0; j < vsz; ++j) {
            // 'j' is the starting index of a block of size 'bs'
            const uint_t prev_elem1 = data[prev[j]];
            const uint_t prev_elem2 = data[prev[j+pbs]];
            if (prev_elem1 > prev_elem2) {
                curr[j] = prev[j];
            }
            else {
                curr[j] = prev[j+pbs];
            }
            // cerr<<"curr["<<j<<"] = "<<curr[j].first<<endl;
        }
        // cerr<<"done with i: "<<i<<endl;
    }
    // cerr<<"initialize() completed"<<endl;
}

// qf & ql are indexes; both inclusive.
// first -> value, second -> index
pui_t SparseTable::query_max(uint_t qf, uint_t ql) {
    if (qf >= this->len || ql >= this->len || ql < qf) {
        return make_pair(minus_one, minus_one);
    }

    const int rlen = ql - qf + 1;
    const size_t ti = log2(rlen);
    const size_t f = qf, l = ql + 1 - (1 << ti);

    // cerr<<"query_max("<<qf<<", "<<ql<<"), ti: "<<ti<<", f: "<<f<<", l: "<<l<<endl;
    const uint_t data1 = data[this->repr[ti][f]];
    const uint_t data2 = data[this->repr[ti][l]];

    if (data1 > data2) {
        return std::make_pair(data1, this->repr[ti][f]);
    }
    else {
        return std::make_pair(data2, this->repr[ti][l]);
    }
}
pui_t  SparseTable::naive_query_max(vui_t const& v, int i, int j) {
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