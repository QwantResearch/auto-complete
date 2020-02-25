// Copyright 2019 Qwant Research. Licensed under the terms of the Apache 2.0
// license. See LICENSE in the project root.

#include "rest_server.h"
#include "utils.h"

using namespace std;

rest_server::rest_server(std::string &config_file, int debug) {
  _debug_mode = debug;
  _num_port=9009;
// std::ifstream model_config;
//   model_config.open(classif_config);
    std::string line;
    YAML::Node config;

    try
    {
    // Reading the configuration file for filling the options.
        config = YAML::LoadFile(config_file);
//         cout << "[INFO]\tDomain\t\tautocorrection\t\tautosuggestion"<< endl;
//         threads = config["threads"].as<int>() ;
//         port =  config["port"].as<int>() ;
//         debug =  config["debug"].as<int>() ;
        YAML::Node modelconfig = config["models"];
        for (const auto& modelnode : modelconfig)
        {
            std::string domain=modelnode.first.as<std::string>();
            YAML::Node modelinfos = modelnode.second;
            std::string filename_autosuggestion=modelinfos["autosuggestion"].as<std::string>();
            std::string filename_autocorrection=modelinfos["autocorrection"].as<std::string>();
            std::string filename_arpa_lm=modelinfos["arpa_lm"].as<std::string>();
            try
            {
                // Creating the set of models for the API
                if ((int) filename_arpa_lm.size() == 0)
                {
                    cerr << "[ERROR]\tLanguage model filename is not set for " << domain << endl;
                    continue;
                }
                if ((int) filename_autosuggestion.size() == 0)
                {
                    cerr << "[ERROR]\tAutosuggestion model filename is not set for " << domain << endl;
                    continue;
                }
                if ((int) filename_autocorrection.size() == 0)
                {
                    cerr << "[ERROR]\tAutocorrection model filename is not set for " << domain << endl;
                    continue;
                }
//                 cout << "[INFO]\t"<< domain << "\t" << filename_autocorrection<< "\t"<< filename_autosuggestion << "\t" ;
                suggest* suggest_pointer = new suggest(filename_autocorrection,filename_autosuggestion, domain);
                if ((int) filename_arpa_lm.size() != 0)                
                {
                    suggest_pointer->load_lm(filename_arpa_lm);
                }
                
                _list_suggest.push_back(suggest_pointer);
//                 cout << "\t===> loaded" << endl;
            }
            catch (invalid_argument& inarg)
            {
                cerr << "[ERROR]\t" << inarg.what() << endl;
                continue;
            }
        }
    } catch (YAML::BadFile& bf) {
      cerr << "[ERROR]\t" << bf.what() << endl;
      exit(1);
    }
}

void rest_server::init(size_t thr) {
  // Creating the entry point of the REST API.
  Pistache::Port pport(_num_port);
  Address addr(Ipv4::any(), pport);
  httpEndpoint = std::make_shared<Http::Endpoint>(addr);

  auto opts = Http::Endpoint::options().threads(thr).flags(
      Tcp::Options::InstallSignalHandler);
  httpEndpoint->init(opts);
  setupRoutes();
}

void rest_server::start() {
  httpEndpoint->setHandler(router.handler());
  cout << "[INFO]\t" << currentDateTime() <<"\tREST server listening on 0.0.0.0:" << _num_port << endl;
  httpEndpoint->serve();
  httpEndpoint->shutdown();
}

void rest_server::setupRoutes() {
  using namespace Rest;

  Routes::Post(router, "/autocomplete/",
               Routes::bind(&rest_server::doAutocompletePost, this));

  Routes::Get(router, "/autocomplete/",
              Routes::bind(&rest_server::doAutocompleteGet, this));

  Routes::Options(router, "/autocomplete/",
              Routes::bind(&rest_server::doAutocompleteOptions, this));

  Routes::Post(router, "/autocorrection/",
               Routes::bind(&rest_server::doAutocorrectionPost, this));

  Routes::Get(router, "/autocorrection/",
              Routes::bind(&rest_server::doAutocompleteGet, this));

  Routes::Options(router, "/autocorrection/",
              Routes::bind(&rest_server::doAutocompleteOptions, this));

  Routes::Post(router, "/autosuggestion/",
               Routes::bind(&rest_server::doAutosuggestionPost, this));

  Routes::Get(router, "/autosuggestion/",
              Routes::bind(&rest_server::doAutocompleteGet, this));

  Routes::Options(router, "/autosuggestion/",
              Routes::bind(&rest_server::doAutocompleteOptions, this));
}

void rest_server::doAutocompleteGet(const Rest::Request &request,
                                      Http::ResponseWriter response) {
  response.headers().add<Http::Header::AccessControlAllowHeaders>(
      "Content-Type");
  response.headers().add<Http::Header::AccessControlAllowMethods>(
      "GET, POST, OPTIONS");
  response.headers().add<Http::Header::AccessControlAllowOrigin>("*");
  response.headers().add<Http::Header::ContentType>(MIME(Application, Json));
  string response_string = "{\"domains\":[";
  for (int inc = 0; inc < (int)_list_suggest.size(); inc++) {
    if (inc > 0)
      response_string.append(",");
    response_string.append("\"");
    response_string.append(_list_suggest.at(inc)->getDomain());
    response_string.append("\"");
  }
  response_string.append("]}");
  if (_debug_mode != 0)
    cerr << "[DEBUG]\t" << currentDateTime() << "\tRESPONSE GET\t" << response_string << endl;
  response.send(Pistache::Http::Code::Ok, response_string);
}

void rest_server::doAutocompleteOptions(const Rest::Request &request,
                                      Http::ResponseWriter response) {
  response.headers().add<Http::Header::AccessControlAllowHeaders>(
      "Content-Type");
  response.headers().add<Http::Header::AccessControlAllowMethods>(
      "GET, POST, OPTIONS");
  response.headers().add<Http::Header::AccessControlAllowOrigin>("*");
  response.headers().add<Http::Header::ContentType>(MIME(Application, Json));
  string response_string = "";
  if (_debug_mode != 0)
    cerr << "[DEBUG]\t" << currentDateTime() << "\tRESPONSE OPTIONS\t" << response_string << endl;
  response.send(Pistache::Http::Code::Ok, response_string);
}

void rest_server::doAutocompletePost(const Rest::Request &request,
                                       Http::ResponseWriter response) {
  response.headers().add<Http::Header::AccessControlAllowHeaders>(
      "Content-Type");
  response.headers().add<Http::Header::AccessControlAllowMethods>(
      "GET, POST, OPTIONS");
  response.headers().add<Http::Header::AccessControlAllowOrigin>("*");
  nlohmann::json j = nlohmann::json::parse(request.body());
  int count = 10;
  if (j.find("count") != j.end()) {
    count = j["count"];
  }
  int count_suggestion = count;
  int count_correction = count;  
  float threshold = 0.0;
  bool debugmode = false;
  if (j.find("counts") != j.end()) {
    count_correction = j["counts"][0];
    count_suggestion = j["counts"][1];
  }
  if (j.find("threshold") != j.end()) {
    threshold = j["threshold"];
  }
  if (j.find("debug") != j.end()) {
    debugmode = j["debug"];
  }
  if (j.find("language") == j.end()) {
    response.headers().add<Http::Header::ContentType>(MIME(Application, Json));
    response.send(Http::Code::Bad_Request,
                  std::string("the language value is null"));
  }
  if (j.find("text") != j.end()) {
    string text = j["text"];
    text=trim(text);
    string lang = j["language"];
    if (text.length() > 0) {
      if (_debug_mode != 0)
        cerr << "[DEBUG]\t" << currentDateTime() << "\tASK POST AUTOCOMPLETE\t" << j << endl;
      if (j.find("domain") != j.end()) 
      {
        string domain = j["domain"];
        std::vector<std::pair<std::vector<float>, std::string>> results;
        std::vector<std::pair<float, std::string>> resultsSuggestion,resultsCorrectedSuggestion;
        results = askAutoCorrection(text, domain, count_correction, threshold);
        resultsSuggestion = askAutoSuggestion(text, domain, count_suggestion, threshold);
        j.push_back(nlohmann::json::object_t::value_type(string("suggestions"), resultsSuggestion));
        json json_results_tmp;
        std::string best_correction="";
        for (int i = 0 ; i < (int)results.size(); i++)
        {
            json local_json_results_tmp;
            local_json_results_tmp.push_back(nlohmann::json::object_t::value_type(string("score"), results[i].first[0]));
            local_json_results_tmp.push_back(nlohmann::json::object_t::value_type(string("distance"), results[i].first[1]));
            local_json_results_tmp.push_back(nlohmann::json::object_t::value_type(string("correction"), results[i].second));
            string tmp_str=results[i].second;
            resultsCorrectedSuggestion = askAutoSuggestion(tmp_str, domain, count_suggestion, threshold);
            local_json_results_tmp.push_back(nlohmann::json::object_t::value_type(string("suggestions"), resultsCorrectedSuggestion));
            if ((int)resultsSuggestion.size() == 0 && (int)resultsCorrectedSuggestion.size() > 0 && i == 0)
            {
                j["suggestions"]=resultsCorrectedSuggestion;
                resultsSuggestion=resultsCorrectedSuggestion;
            }
            if ((int)resultsCorrectedSuggestion.size() > 0 && results[i].first[1] == 0)
            {
                if (resultsCorrectedSuggestion[0].first > resultsSuggestion[0].first)
                {
                    j["suggestions"]=resultsCorrectedSuggestion;
                }
            }
            json_results_tmp.push_back(local_json_results_tmp);
        }
        j.push_back(nlohmann::json::object_t::value_type(string("corrections"), json_results_tmp));
        if (j.find("suggestions") == j.end())
        {
            resultsSuggestion = askAutoSuggestion(text, domain, count_suggestion, threshold);
            j.push_back(nlohmann::json::object_t::value_type(string("suggestions"), resultsSuggestion));
        }
        
      } 
      else 
      {
        response.headers().add<Http::Header::ContentType>(
            MIME(Application, Json));
        response.send(Http::Code::Bad_Request,
                      std::string("Domain value is null"));
      }
      std::string s = j.dump();
      if (_debug_mode != 0)
        cerr << "[DEBUG]\t" << currentDateTime() << "\tRESPONSE POST AUTOCOMPLETE\t" << s << endl;
      response.headers().add<Http::Header::ContentType>(
          MIME(Application, Json));
      response.send(Http::Code::Ok, std::string(s));
    } else {
      response.headers().add<Http::Header::ContentType>(
          MIME(Application, Json));
      response.send(Http::Code::Bad_Request,
                    std::string("Length of text value is null"));
    }
  } else {
    response.headers().add<Http::Header::ContentType>(MIME(Application, Json));
    response.send(Http::Code::Bad_Request,
                  std::string("the text value is null"));
  }
}

void rest_server::doAutocorrectionPost(const Rest::Request& request, Http::ResponseWriter response)
{
  response.headers().add<Http::Header::AccessControlAllowHeaders>(
      "Content-Type");
  response.headers().add<Http::Header::AccessControlAllowMethods>(
      "GET, POST, OPTIONS");
  response.headers().add<Http::Header::AccessControlAllowOrigin>("*");
  nlohmann::json j = nlohmann::json::parse(request.body());
  int count = 10;
  float threshold = 0.0;
  bool debugmode = false;
  if (j.find("count") != j.end()) {
    count = j["count"];
  }
  if (j.find("threshold") != j.end()) {
    threshold = j["threshold"];
  }
  if (j.find("debug") != j.end()) {
    debugmode = j["debug"];
  }
  if (j.find("language") == j.end()) {
    response.headers().add<Http::Header::ContentType>(MIME(Application, Json));
    response.send(Http::Code::Bad_Request,
                  std::string("the language value is null"));
  }
  if (j.find("text") != j.end()) {
    string text = j["text"];
    text=trim(text);
    string lang = j["language"];
    if (text.length() > 0) {
      if (_debug_mode != 0)
        cerr << "[DEBUG]\t" << currentDateTime() << "\tASK POST AUTOCORRECTION\t" << j << endl;
      if (j.find("domain") != j.end()) 
      {
        string domain = j["domain"];
        std::vector<std::pair<std::vector<float>, std::string>> results;
        std::vector<std::pair<float, std::string>> results_tmp;
        results = askAutoCorrection(text, domain, count, threshold);
        json json_results_tmp;
        
        for (int i = 0 ; i < (int)results.size(); i++)
        {
            json local_json_results_tmp;
            local_json_results_tmp.push_back(nlohmann::json::object_t::value_type(string("score"), results[i].first[0]));
            local_json_results_tmp.push_back(nlohmann::json::object_t::value_type(string("distance"), results[i].first[1]));
            local_json_results_tmp.push_back(nlohmann::json::object_t::value_type(string("correction"), results[i].second));
            string tmp_str=results[i].second;
//             results_tmp = askAutoSuggestion(tmp_str, domain, count, threshold);
//             local_json_results_tmp.push_back(nlohmann::json::object_t::value_type(string("suggestions"), results_tmp));
            json_results_tmp.push_back(local_json_results_tmp);
        }
        j.push_back(nlohmann::json::object_t::value_type(string("corrections"), json_results_tmp));
//         j.push_back(nlohmann::json::object_t::value_type(string("corrections"), results));
//         results = askAutoSuggestion(text, domain, count, threshold);
//         j.push_back(nlohmann::json::object_t::value_type(string("suggestions"), results));
        
      } 
      else 
      {
        response.headers().add<Http::Header::ContentType>(
            MIME(Application, Json));
        response.send(Http::Code::Bad_Request,
                      std::string("Domain value is null"));
      }
      std::string s = j.dump();
      if (_debug_mode != 0)
        cerr << "[DEBUG]\t" << currentDateTime() << "\tRESPONSE POST AUTOCORRECTION\t" << s << endl;
      response.headers().add<Http::Header::ContentType>(
          MIME(Application, Json));
      response.send(Http::Code::Ok, std::string(s));
    } else {
      response.headers().add<Http::Header::ContentType>(
          MIME(Application, Json));
      response.send(Http::Code::Bad_Request,
                    std::string("Length of text value is null"));
    }
  } else {
    response.headers().add<Http::Header::ContentType>(MIME(Application, Json));
    response.send(Http::Code::Bad_Request,
                  std::string("the text value is null"));
  }

}
void rest_server::doAutosuggestionPost(const Rest::Request& request, Http::ResponseWriter response)
{
  response.headers().add<Http::Header::AccessControlAllowHeaders>(
      "Content-Type");
  response.headers().add<Http::Header::AccessControlAllowMethods>(
      "GET, POST, OPTIONS");
  response.headers().add<Http::Header::AccessControlAllowOrigin>("*");
  nlohmann::json j = nlohmann::json::parse(request.body());
  int count = 10;
  float threshold = 0.0;
  bool debugmode = false;
  if (j.find("count") != j.end()) {
    count = j["count"];
  }
  if (j.find("threshold") != j.end()) {
    threshold = j["threshold"];
  }
  if (j.find("debug") != j.end()) {
    debugmode = j["debug"];
  }
  if (j.find("language") == j.end()) {
    response.headers().add<Http::Header::ContentType>(MIME(Application, Json));
    response.send(Http::Code::Bad_Request,
                  std::string("the language value is null"));
  }
  if (j.find("text") != j.end()) {
    string text = j["text"];
    text=trim(text);
    string lang = j["language"];
    if (text.length() > 0) {
      if (_debug_mode != 0)
        cerr << "[DEBUG]\t" << currentDateTime() << "\tASK POST AUTOSUGGESTION\t" << j << endl;
      if (j.find("domain") != j.end()) 
      {
        string domain = j["domain"];
        std::vector<std::pair<float, std::string>> results;
        std::vector<std::pair<float, std::string>> results_tmp;
//         results = askAutoCorrection(text, domain, count, threshold);
//         json json_results_tmp;
//         
//         for (int i = 0 ; i < (int)results.size(); i++)
//         {
//             json local_json_results_tmp;
//             local_json_results_tmp.push_back(nlohmann::json::object_t::value_type(string("score"), results[i].first));
//             local_json_results_tmp.push_back(nlohmann::json::object_t::value_type(string("correction"), results[i].second));
//             string tmp_str=results[i].second;
//             results_tmp = askAutoSuggestion(tmp_str, domain, count, threshold);
//             local_json_results_tmp.push_back(nlohmann::json::object_t::value_type(string("suggestions"), results_tmp));
//             json_results_tmp.push_back(local_json_results_tmp);
//         }
//         j.push_back(nlohmann::json::object_t::value_type(string("corrections"), json_results_tmp));
        results = askAutoSuggestion(text, domain, count, threshold);
        j.push_back(nlohmann::json::object_t::value_type(string("suggestions"), results));
        
      } 
      else 
      {
        response.headers().add<Http::Header::ContentType>(
            MIME(Application, Json));
        response.send(Http::Code::Bad_Request,
                      std::string("Domain value is null"));
      }
      std::string s = j.dump();
      if (_debug_mode != 0)
        cerr << "[DEBUG]\t" << currentDateTime() << "\tRESPONSE POST AUTOSUGGESTION\t" << s << endl;
      response.headers().add<Http::Header::ContentType>(
          MIME(Application, Json));
      response.send(Http::Code::Ok, std::string(s));
    } else {
      response.headers().add<Http::Header::ContentType>(
          MIME(Application, Json));
      response.send(Http::Code::Bad_Request,
                    std::string("Length of text value is null"));
    }
  } else {
    response.headers().add<Http::Header::ContentType>(MIME(Application, Json));
    response.send(Http::Code::Bad_Request,
                  std::string("the text value is null"));
  }

}



std::vector<std::pair<std::vector<float>, std::string>>
rest_server::askAutoCorrection(std::string &text, std::string &domain,
                               int count, float threshold) {
  std::vector<std::pair<std::vector<float>, std::string>> to_return;
  if ((int)text.size() > 0) {
    auto it_suggest = std::find_if(_list_suggest.begin(), _list_suggest.end(),
                                   [&](suggest *l_suggest) {
                                     return l_suggest->getDomain() == domain;
                                   });
    if (it_suggest != _list_suggest.end()) {
      to_return = (*it_suggest)->process_query_autocorrection(text, count);
    }
  }
  return to_return;
}

std::vector<std::pair<float, std::string>>
rest_server::askAutoSuggestion(std::string &text, std::string &domain,
                               int count, float threshold) {
  std::vector<std::pair<float, std::string>> to_return;
  if ((int)text.size() > 0) {
    auto it_suggest = std::find_if(_list_suggest.begin(), _list_suggest.end(),
                                   [&](suggest *l_suggest) {
                                     return l_suggest->getDomain() == domain;
                                   });
    if (it_suggest != _list_suggest.end()) {
      to_return = (*it_suggest)->process_query_autosuggestion(text, count);
    }
  }
  return to_return;
}

void rest_server::doAuth(const Rest::Request &request,
                         Http::ResponseWriter response) {
  printCookies(request);
  response.cookies().add(Http::Cookie("lang", "fr-FR"));
  response.send(Http::Code::Ok);
}


