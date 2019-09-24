#include "suggest.h"

// fst::fst_error_fatal = 0;

using namespace std;


suggest::suggest(string& filename_autocorrection, string& filename_autosuggestion, std::string &domain)
{
      int nadded = 0;
      int nlines = 0;

    _symSpellModel_correction = new symspell::SymSpell();
    string l_line;
    _if_mmap_addr = NULL;
    _if_length = 0;
    time_t start_time = time(NULL);
    cerr << "[INFO]:\t"<<currentDateTime()<<" Loading auto-correction data from "<<  filename_autocorrection <<  " ... " ;
    _symSpellModel_correction->LoadDictionary(filename_autocorrection.c_str(),1,0);
    cerr << "loaded in " <<  (int)(time(NULL) - start_time) << " seconds" << endl;
    start_time = time(NULL);
    cerr << "[INFO]:\t"<<currentDateTime()<<" Loading auto-suggestion data from "<<  filename_autosuggestion <<  " ... " ;
    load_pm(filename_autosuggestion,nadded,nlines);
    cerr << "loaded "<< nadded << "/" << nlines << " in " <<  (int)(time(NULL) - start_time) << " seconds" << endl;
    _domain=domain;
    _symSpellModel_correction->setDistanceAlgorithm(symspell::EditDistance::DistanceAlgorithm::DamerauOSAspe);
}


std::vector<std::pair<std::vector<float>, std::string>>  suggest::process_query_autocorrection(string& query, int nbest)
{
    std::vector<std::pair<std::vector<float>, std::string>> to_return;
    vector< std::unique_ptr<symspell::SuggestItem>> items;
    _symSpellModel_correction->Lookup(query, symspell::Verbosity::All, items);
    if ((int)items.size() == 0)
    {
        string sub_query=query.substr(query.find(" ")+1,(int)query.size()-query.find(" "));
//         cerr << "sub query autocomplete : " << sub_query << endl;
        _symSpellModel_correction->Lookup(sub_query, symspell::Verbosity::All, items);
        string tmp_str=query.substr(0,query.find(" "));
        for(int i=0 ; i < nbest && i < (int)items.size(); i++)
        {    
            to_return.push_back(pair<std::vector<float>,string>({(float)items[i]->count,(float)items[i]->distance},tmp_str+" "+items[i]->term));
        }
    }
    else
    {
        for(int i=0 ; i < nbest && i < (int)items.size(); i++)
        {    
            to_return.push_back(pair<std::vector<float>,string>({(float)items[i]->count,(float)items[i]->distance},items[i]->term));
        }
    }
//     std::sort ( to_return.begin(), to_return.end(), mySortingFunctionFloatString );

    return to_return;
}

std::vector<std::pair<float, std::string>>  suggest::process_query_autosuggestion(string& query, int nbest)
{
    std::vector<std::pair<float, std::string>> to_return;
    vp_t results_segTree = smart_suggest(query,nbest);
    if ((int)results_segTree.size() == 0)
    {
        string sub_query=query.substr(query.find(" ")+1,(int)query.size()-query.find(" "));
//         cerr << "sub query autosuggest : " << sub_query << endl;
        results_segTree = smart_suggest(sub_query,nbest);
        string tmp_str=query.substr(0,query.find(" "));
        for(int i=0 ; i < nbest && i < (int)results_segTree.size(); i++)
        {    
            to_return.push_back(pair<float,string>(results_segTree[i].weight,tmp_str+" "+results_segTree[i].phrase));
        }
    }
    else
    {
        for(int i=0 ; i < nbest && i < (int)results_segTree.size(); i++)
        {    
            to_return.push_back(pair<float,string>(results_segTree[i].weight,results_segTree[i].phrase));
        }
    }
//     std::sort ( to_return.begin(), to_return.end(), mySortingFunctionFloatString );
    return to_return;
}


vp_t suggest::smart_suggest(std::string prefix, uint_t n) {
    pvpi_t phrases = _pm.query(prefix);

    uint_t first = phrases.first  - _pm.repr.begin();
    uint_t last  = phrases.second - _pm.repr.begin();

    if (first == last) {
        return vp_t();
    }

    vp_t ret;
    --last;

    pqpr_t heap;
    pui_t best = _segmentTree_suggestion.query_max(first, last);
    heap.push(PhraseRange(first, last, best.first, best.second));

    while (ret.size() < n && !heap.empty()) {
        PhraseRange pr = heap.top();
        heap.pop();
        // cerr<<"Top phrase is at index: "<<pr.index<<endl;
        // cerr<<"And is: "<<pm.repr[pr.index].first<<endl;

        ret.push_back(_pm.repr[pr.index]);

        uint_t lower = pr.first;
        uint_t upper = pr.index - 1;

        // Prevent underflow
        if (pr.index - 1 < pr.index && lower <= upper) {
            // cerr<<"[1] adding to heap: "<<lower<<", "<<upper<<", "<<best.first<<", "<<best.second<<endl;

            best = _segmentTree_suggestion.query_max(lower, upper);
            heap.push(PhraseRange(lower, upper, best.first, best.second));
        }

        lower = pr.index + 1;
        upper = pr.last;

        // Prevent overflow
        if (pr.index + 1 > pr.index && lower <= upper) {
            // cerr<<"[2] adding to heap: "<<lower<<", "<<upper<<", "<<best.first<<", "<<best.second<<endl;

            best = _segmentTree_suggestion.query_max(lower, upper);
            heap.push(PhraseRange(lower, upper, best.first, best.second));
        }
    }

    return ret;
}

vp_t suggest::naive_suggest(std::string prefix, uint_t n) {
    pvpi_t phrases = _pm.query(prefix);
    std::vector<uint_t> indexes;
    vp_t ret;

    while (phrases.first != phrases.second) {
        indexes.push_back(phrases.first - _pm.repr.begin());
        ++phrases.first;
    }

    while (ret.size() < n && !indexes.empty()) {
        uint_t mi = 0;
        for (size_t i = 1; i < indexes.size(); ++i) {
            if (_pm.repr[indexes[i]].weight > _pm.repr[indexes[mi]].weight) {
                mi = i;
            }
        }
        ret.push_back(_pm.repr[indexes[mi]]);
        indexes.erase(indexes.begin() + mi);
    }
    return ret;
}


int suggest::load_pm(string file, int& rnadded, int& rnlines)
{
    bool is_input_sorted = true;
    std::ifstream fin(file.c_str());

    _pm_is_building = true;
    int nlines = 0;
    int foffset = 0;

    _pm.repr.clear();
    std::string prev_phrase;
    std::string buff;
    while (getline(fin,buff)) 
    {
        int llen = (int)buff.size();
        ++nlines;

        int weight = 0;
        std::string phrase;
        StringProxy snippet;
        InputLineParser(_if_mmap_addr, foffset, buff.c_str(), &weight, &phrase, &snippet).start_parsing(_if_mmap_addr,_if_length);

        foffset += llen;

        if (!phrase.empty()) {
            //str_lowercase(phrase);
            DCERR("Adding: " << weight << ", " << phrase << ", " << std::string(snippet) << endl);
            _pm.insert(weight, phrase, snippet);
        }
        if (is_input_sorted && prev_phrase <= phrase) {
            prev_phrase.swap(phrase);
        } else if (is_input_sorted) {
            is_input_sorted = false;
        }
    }
// 
//         DCERR("Creating PhraseMap::Input is " << (!is_input_sorted ? "NOT " : "") << "sorted\n");

    fin.close();
    _pm.finalize(is_input_sorted);
    vui_t weights;
    for (size_t i = 0; i < _pm.repr.size(); ++i) {
        weights.push_back(_pm.repr[i].weight);
    }
    _segmentTree_suggestion.initialize(weights);

    rnadded = weights.size();
    rnlines = nlines;

    _pm_is_building = false;
//     }

    return 0;

}


off_t suggest::file_size(const char *path) 
{
    struct stat sbuf;
    int r = stat(path, &sbuf);

    assert(r == 0);
    if (r < 0) {
        return 0;
    }

    return sbuf.st_size;
}
