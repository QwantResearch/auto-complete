// Copyright 2019 Qwant Research. Licensed under the terms of the Apache 2.0
// license. See LICENSE in the project root.
#ifndef __SUGGEST_H
#define __SUGGEST_H

// #define USE_SPARSEPP

#include <symspell.h>
#include <sstream>
#include "utils.h"
#include "types.h"
#include "phrase_map.h"
#include "benderrmq.h"


using namespace std;


struct PhraseRange {
    // first & last are both inclusive of the range. i.e. The range is
    // [first, last] and NOT [first, last)
    uint_t first, last;

    // weight is the value of the best solution in the range [first,
    // last] and index is the index of this best solution in the
    // original array of strings.
    uint_t weight, index;

    PhraseRange(uint_t f, uint_t l, uint_t w, uint_t i)
        : first(f), last(l), weight(w), index(i)
    { }

    bool
    operator<(PhraseRange const &rhs) const {
        return this->weight < rhs.weight;
    }
};

typedef std::priority_queue<PhraseRange> pqpr_t;


class suggest {
public:
  suggest(std::string &filename, std::string &domain);
  std::vector<std::pair<float, std::string>> process_query_autocomplete(std::string& url, int nbest);
  std::vector<std::pair<float, std::string>> process_query_autosuggest(std::string& url, int nbest);
  std::string getDomain() { return _domain; }
//   int getModelType() { return _modelType; }
  vp_t naive_suggest(std::string prefix, uint_t n = 16);
  vp_t smart_suggest(std::string prefix, uint_t n = 16);
  int load_pm(string file, int& rnadded, int& rnlines);
  off_t file_size(const char *path);

  
  
  
private:
    symspell::SymSpell* _symSpellModel;
    std::string _domain;  
    PhraseMap _pm;
    RMQ _segmentTree; 
    bool _pm_is_building;
    char *_if_mmap_addr;
    off_t _if_length;
    
};

#endif // __SUGGEST_H
