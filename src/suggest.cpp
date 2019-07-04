#include "suggest.h"
// fst::fst_error_fatal = 0;

using namespace std;


suggest::suggest(string& filename, std::string &domain)
{
        _symSpellModel = new symspell::SymSpell();
        string l_line;
//         ifstream inputfile(filename);
//         char* filename_char;
//         std::strcpy(filename_char,filename.c_str());
        _symSpellModel->LoadDictionary(filename.c_str(),1,0);
//         while (getline(inputfile,l_line))
//         {
//             
//         }
//         inputfile.close();
      
    _modelType=type;
    _domain=domain;
}


std::vector<std::pair<float, std::string>>  suggest::process_url(string& url, int nbest)
{
    std::vector<std::pair<float, std::string>> to_return;
    vector< std::unique_ptr<symspell::SuggestItem>> items;
    _symSpellModel->Lookup(url, symspell::Verbosity::All, items);
    std::shared_ptr<symspell::WordSegmentationItem> answer=_symSpellModel->WordSegmentation(url);
    for(int i=0 ; i < nbest && i < (int)items.size(); i++)
    {    
//               cerr << items[i]->count << "\t" << items[i]->term << endl;
        to_return.push_back(pair<float,string>(items[i]->count,items[i]->term));
    }
    to_return.push_back(pair<float,string>((float)answer->probabilityLogSum,answer->segmentedString));
//           delete answer;
    answer->~WordSegmentationItem();
    return to_return;
}

