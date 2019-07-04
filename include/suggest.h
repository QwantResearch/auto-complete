// Copyright 2019 Qwant Research. Licensed under the terms of the Apache 2.0
// license. See LICENSE in the project root.
#ifndef __SUGGEST_H
#define __SUGGEST_H

// #define USE_SPARSEPP

#include <symspell.h>
#include <sstream>

class suggest {
public:
  suggest(std::string &filename, std::string &domain);
  std::vector<std::pair<float, std::string>> process_url(std::string& url, int nbest);
  std::string getDomain() { return _domain; }
  int getModelType() { return _modelType; }

private:
    symspell::SymSpell* _symSpellModel;
    std::string _domain;  
};

#endif // __SUGGEST_H
