// Copyright 2019 Qwant Research. Licensed under the terms of the Apache 2.0
// license. See LICENSE in the project root.
#ifndef __SUGGEST_H
#define __SUGGEST_H

// #define USE_SPARSEPP

#include <unordered_map>
#include <symspell.h>
#include <editdistance.h>
#include <sstream>
#include <nlohmann/json.hpp>
#include "utils.h"
#include "types.h"
#include "phrase_map.h"
#include "benderrmq.h"
#include "embeddings.h"


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
  suggest(std::string& filename_autocorrection, std::string& filename_autosuggestion, std::string &domain);
  std::vector<std::pair<std::vector<float>, std::string>> process_query_autocorrection(std::string& url, int nbest);
  std::vector<std::pair<float, std::string>> process_query_autosuggestion(std::string& url, int nbest);
  std::string getDomain() { return _domain; }
//   int getModelType() { return _modelType; }
  vp_t naive_suggest(std::string prefix, uint_t n = 16);
  vp_t smart_suggest(std::string prefix, uint_t n = 16);
  int load_pm(string file, int& rnadded, int& rnlines);
  float eval_cosine_distance(std::string & s1, std::string & s2);
  off_t file_size(const char *path);
  bool load_we_model(std::string & we_model_filename);

  
  
  
private:
  // model for auto-correction
    symspell::SymSpell* _symSpellModel_correction;
  // model for auto-suggestion
    RMQ _segmentTree_suggestion; 
    std::string _domain;  
    PhraseMap _pm;
    bool _use_correspondance_data;
    std::unordered_map <std::string, std::string> _correspondances;
    bool _pm_is_building;
    char *_if_mmap_addr;
    off_t _if_length;
    Embeddings *we_model;
    
};

#endif // __SUGGEST_H
