#include "suggest.h"

// fst::fst_error_fatal = 0;

using namespace std;


suggest::suggest(string& filename, std::string &domain)
{
      int nadded = 0;
      int nlines = 0;

    _symSpellModel = new symspell::SymSpell();
    string l_line;
    _if_mmap_addr = NULL;
    _if_length = 0;
    time_t start_time = time(NULL);
    _symSpellModel->LoadDictionary(filename.c_str(),1,0);
    cerr << "Loaded SymSpell data from "<<  filename << " in " <<  (int)(time(NULL) - start_time) << " seconds" << endl;
    start_time = time(NULL);
    load_pm(filename,nadded,nlines);
    cerr << "SegmentTree data loaded: "<< nadded << "/" << nlines << " from "<<  filename << " in " <<  (int)(time(NULL) - start_time) << " seconds" << endl;
    _domain=domain;
}


std::vector<std::pair<float, std::string>>  suggest::process_query_autocomplete(string& query, int nbest)
{
    std::vector<std::pair<float, std::string>> to_return;
    vector< std::unique_ptr<symspell::SuggestItem>> items;
    _symSpellModel->Lookup(query, symspell::Verbosity::All, items);
//     std::shared_ptr<symspell::WordSegmentationItem> answer=_symSpellModel->WordSegmentation(query);
    for(int i=0 ; i < nbest && i < (int)items.size(); i++)
    {    
//               cerr << items[i]->count << "\t" << items[i]->term << endl;
        to_return.push_back(pair<float,string>(items[i]->count,items[i]->term));
    }
//     vp_t results_segTree = smart_suggest(query,nbest);
//     cerr << results_json(query, results_segTree , "unk") << endl;
//     for(int i=0 ; i < nbest && i < (int)results_segTree.size(); i++)
//     {    
//               cerr << items[i]->count << "\t" << items[i]->term << endl;
//         to_return.push_back(pair<float,string>(results_segTree[i]->weight,results_segTree[i]->phrase));
//     }
//     to_return.push_back(pair<float,string>((float)answer->probabilityLogSum,answer->segmentedString));
//           delete answer;
//     answer->~WordSegmentationItem();
    return to_return;
}

std::vector<std::pair<float, std::string>>  suggest::process_query_autosuggest(string& query, int nbest)
{
    std::vector<std::pair<float, std::string>> to_return;
    vp_t results_segTree = smart_suggest(query,nbest);
    cerr << results_json(query, results_segTree , "unk") << endl;
    for(int i=0 ; i < nbest && i < (int)results_segTree.size(); i++)
    {    
//               cerr << items[i]->count << "\t" << items[i]->term << endl;
        to_return.push_back(pair<float,string>(results_segTree[i].weight,results_segTree[i].phrase));
    }
//     to_return.push_back(pair<float,string>((float)answer->probabilityLogSum,answer->segmentedString));
//           delete answer;
//     answer->~WordSegmentationItem();
    return to_return;
}


vp_t suggest::smart_suggest(std::string prefix, uint_t n) {
    pvpi_t phrases = _pm.query(prefix);
    // cerr<<"Got "<<phrases.second - phrases.first<<" candidate phrases from PhraseMap"<<endl;

    uint_t first = phrases.first  - _pm.repr.begin();
    uint_t last  = phrases.second - _pm.repr.begin();

    if (first == last) {
        return vp_t();
    }

    vp_t ret;
    --last;

    pqpr_t heap;
    pui_t best = _segmentTree.query_max(first, last);
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

            best = _segmentTree.query_max(lower, upper);
            heap.push(PhraseRange(lower, upper, best.first, best.second));
        }

        lower = pr.index + 1;
        upper = pr.last;

        // Prevent overflow
        if (pr.index + 1 > pr.index && lower <= upper) {
            // cerr<<"[2] adding to heap: "<<lower<<", "<<upper<<", "<<best.first<<", "<<best.second<<endl;

            best = _segmentTree.query_max(lower, upper);
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
    _segmentTree.initialize(weights);

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
