#include "suggest.h"

// fst::fst_error_fatal = 0;

using namespace std;


suggest::suggest(string& filename_autocorrection, string& filename_autosuggestion, std::string &domain)
{
      int nadded = 0;
      int nlines = 0;
    _use_correspondance_data = false;
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
    std::vector<std::pair<float, std::string>> to_return_final;
    vp_t results_segTree = smart_suggest(query,nbest);
    string phrase_to_print;
    if ((int)results_segTree.size() == 0)
    {
        string sub_query="";
        string tmp_str="";
        sub_query=query.substr(query.find(" ")+1,(int)query.size()-query.find(" "));
        results_segTree = smart_suggest(sub_query,nbest);
        tmp_str=query.substr(0,query.find(" "));
        while ((int)results_segTree.size()==0 && (int)sub_query.size() > 0)
        {
            sub_query=sub_query.substr(sub_query.find(" ")+1,(int)sub_query.size()-sub_query.find(" "));
            results_segTree = smart_suggest(sub_query,nbest);
            tmp_str=tmp_str+" "+sub_query.substr(0,sub_query.find(" "));
        }
        for(int i=0 ; i < (int)results_segTree.size(); i++)
        {    
            if (_use_correspondance_data) phrase_to_print=(*(_correspondances.find(results_segTree[i].phrase))).second;
            else phrase_to_print=results_segTree[i].phrase;
            to_return.push_back(pair<float,string>(results_segTree[i].weight,tmp_str+" "+phrase_to_print));
        }
    }
    else
    {
        for(int i=0 ; i < (int)results_segTree.size(); i++)
        {    
            if (_use_correspondance_data) phrase_to_print=(*(_correspondances.find(results_segTree[i].phrase))).second;
            else phrase_to_print=results_segTree[i].phrase;
            to_return.push_back(pair<float,string>(results_segTree[i].weight,phrase_to_print));
        }
    }
//     std::sort ( to_return.begin(), to_return.end(), mySortingFunctionFloatString );
    if ((int)to_return.size() > 0) to_return_final.push_back(to_return.at(0));
    for(int i=1 ; i < (int)to_return.size() && nbest > (int)to_return_final.size(); i++)
    {
        string prev = to_return.at(i-1).second;
        string current = to_return.at(i).second;
        if (prev != current) to_return_final.push_back(to_return.at(i));
    }
    return to_return_final;
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

    while (ret.size() < 2*n && !heap.empty()) {
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
    bool is_json = false;
    
    std::ifstream fin(file.c_str());
    if ((int)file.find(".json") > -1) 
    {
            is_json = true; 
            _use_correspondance_data = true;
    }
    _pm_is_building = true;
    int nlines = 0;
    int foffset = 0;

    _pm.repr.clear();
    std::string prev_phrase;
    std::string buff;
    std::string phrase;
    while (getline(fin,buff)) 
    {
        if ( ! is_json)
        {
            int llen = (int)buff.size();
            ++nlines;

            int weight = 0;
            phrase.clear();
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
        else
        {
            nlohmann::json l_data_root=nlohmann::json::parse(buff);
            if (l_data_root.find("_source") != l_data_root.end())
            {
                nlohmann::json l_data = l_data_root["_source"];
    //             nlohmann::json l_data_suggest = l_data.find("suggest");
                if (l_data.find("suggest") != l_data.end() && l_data.find("count") != l_data.end() )
                {
                    nlohmann::json l_data_suggest = l_data["suggest"];
                    long l_count=l_data["count"];
                    std::string l_str_count=std::to_string(l_count);
                    if (l_data_suggest.find("input") != l_data_suggest.end() && l_data_suggest.find("output") != l_data_suggest.end() )
                    {
                        
                        vector<std::string> l_input=l_data_suggest["input"];
                        std::string l_output=l_data_suggest["output"];
                        int l_inc=0;
                        std::string l_buff;
                        std::string phrase;
                        int l_weight=1;
                        if (l_data_suggest.find("weight") != l_data_suggest.end()) l_weight=l_data_suggest["weight"];
                        while (l_inc < (int)l_input.size())
                        {
//                             cerr << ".";
                            l_buff=l_str_count+"\t"+l_input[l_inc];
                            int llen = (int)l_buff.size();
                            ++nlines;
                            phrase.clear();
                            int weight = 0;
                            StringProxy snippet;
                            InputLineParser(_if_mmap_addr, foffset, l_buff.c_str(), &weight, &phrase, &snippet).start_parsing(_if_mmap_addr,_if_length);

                            foffset += llen;

                            if (!phrase.empty()) {
                                //str_lowercase(phrase);
                                weight=weight*l_weight;
                                DCERR("Adding: " << weight << ", " << phrase << ", " << std::string(snippet) << endl);
                                _pm.insert(weight, phrase, snippet);
                            }
                            if (is_input_sorted && prev_phrase <= phrase) {
                                prev_phrase.swap(phrase);
                            } else if (is_input_sorted) {
                                is_input_sorted = false;
                            }

                            _correspondances.insert(pair<std::string,std::string>(l_input[l_inc],l_output));
                            l_inc++;
                        }
                    }
                }
            }
        }
    }
    cerr << "."<< endl;
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
