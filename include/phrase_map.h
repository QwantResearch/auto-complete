#if !defined __PHRASE_MAP_H
#define __PHRASE_MAP_H

#include <iostream>
#include <vector>
#include <utility>
#include <algorithm>
#include <string>
#include <stdio.h>
#include <assert.h>

#include "types.h"

using namespace std;


class PhraseMap {
public:
    vp_t repr;

public:
    PhraseMap(uint_t _len = 15000000);

    void insert(uint_t weight, std::string const& p, StringProxy const& s);

    void finalize(int sorted = 0);

    pvpi_t query(std::string const &prefix);
    pvpi_t naive_query(PhraseMap &pm, std::string prefix);

    void show_indexes(PhraseMap &pm, std::string prefix);


};


#endif // __PHRASE_MAP_H
