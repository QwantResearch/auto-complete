#include "phrase_map.h"

using namespace std;

PhraseMap::PhraseMap(uint_t _len) {
    this->repr.reserve(_len);
}

void PhraseMap::insert(uint_t weight, std::string const& p, StringProxy const& s) {
    this->repr.push_back(phrase_t(weight, p, s));
}

void PhraseMap::finalize(int sorted) {
    if (!sorted) {
        std::sort(this->repr.begin(), this->repr.end());
    }
}

pvpi_t PhraseMap::query(std::string const &prefix) {
    return std::equal_range(this->repr.begin(), this->repr.end(), 
                            prefix, PrefixFinder());
}


pvpi_t PhraseMap::naive_query(PhraseMap &pm, std::string prefix) {
    vpi_t f = pm.repr.begin(), l = pm.repr.begin();
    while (f != pm.repr.end() && f->phrase.substr(0, prefix.size()) < prefix) {
        ++f;
    }
    l = f;
    while (l != pm.repr.end() && l->phrase.substr(0, prefix.size()) == prefix) {
        ++l;
    }
    return std::make_pair(f, l);
}

void PhraseMap::show_indexes(PhraseMap &pm, std::string prefix) {
    pvpi_t nq = naive_query(pm, prefix);
    pvpi_t q  = pm.query(prefix);

    cout<<"naive[first] = "<<nq.first - pm.repr.begin()<<", naive[last] = "<<nq.second - pm.repr.begin()<<endl;
    cout<<"phmap[first] = "<<q.first - pm.repr.begin()<<", phmap[last] = "<<q.second - pm.repr.begin()<<endl;
}

