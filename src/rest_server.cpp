// Copyright 2019 Qwant Research. Licensed under the terms of the Apache 2.0
// license. See LICENSE in the project root.

#include "rest_server.h"
#include "utils.h"

using namespace std;

rest_server::rest_server(Address addr, std::string &classif_config, int debug) {
  httpEndpoint = std::make_shared<Http::Endpoint>(addr);
  _debug_mode = debug;
  std::ifstream model_config;
  model_config.open(classif_config);
  std::string line;

  YAML::Node config;
  try {
    config = YAML::LoadFile(classif_config);
  } catch (YAML::BadFile& bf) {
    cerr << "[ERROR] " << bf.what() << endl;
    exit(1);
  }

  for (const auto& line : config){
    string domain = line.first.as<std::string>();
    string file = line.second.as<std::string>();

    if(domain.empty() || file.empty()) {
      cerr << "[ERROR] Malformed config for pair ("
        << domain << ", " << file << ")" << endl;
      cerr << "        Skipped line..." << endl;
      continue;
    }

    cout << domain << "\t" << file << "\t" ;
    
    try {
      suggest* suggest_pointer = new suggest(file, domain);
      _list_suggest.push_back(suggest_pointer);
    } catch (invalid_argument& inarg) {
      cerr << "[ERROR] " << inarg.what() << endl;
      continue;
    }
    cout << "loaded" << endl;
  }
}

void rest_server::init(size_t thr) {
  auto opts = Http::Endpoint::options().threads(thr).flags(
      Tcp::Options::InstallSignalHandler);
  httpEndpoint->init(opts);
  setupRoutes();
}

void rest_server::start() {
  httpEndpoint->setHandler(router.handler());
  httpEndpoint->serve();
  httpEndpoint->shutdown();
}

void rest_server::setupRoutes() {
  using namespace Rest;

  Routes::Post(router, "/autocomplete/",
               Routes::bind(&rest_server::doSegmentationPost, this));

  Routes::Get(router, "/autocomplete/",
              Routes::bind(&rest_server::doSegmentationGet, this));
}

void rest_server::doSegmentationGet(const Rest::Request &request,
                                      Http::ResponseWriter response) {
  response.headers().add<Http::Header::AccessControlAllowHeaders>(
      "Content-Type");
  response.headers().add<Http::Header::AccessControlAllowMethods>(
      "GET, POST, DELETE, OPTIONS");
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
    cerr << "LOG: " << currentDateTime() << "\t" << response_string << endl;
  response.send(Pistache::Http::Code::Ok, response_string);
}

void rest_server::doSegmentationPost(const Rest::Request &request,
                                       Http::ResponseWriter response) {
  response.headers().add<Http::Header::AccessControlAllowHeaders>(
      "Content-Type");
  response.headers().add<Http::Header::AccessControlAllowMethods>(
      "GET, POST, DELETE, OPTIONS");
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
    string lang = j["language"];
    if (text.length() > 0) {
      if (_debug_mode != 0)
        cerr << "LOG: " << currentDateTime() << "\t"
             << "ASK CLASS :\t" << j << endl;
      if (j.find("domain") != j.end()) 
      {
        string domain = j["domain"];
        std::vector<std::pair<float, std::string>> results;
        std::vector<std::pair<float, std::string>> results_tmp;
        results = askAutoComplete(text, domain, count, threshold);
        json json_results_tmp;
        
        for (int i = 0 ; i < (int)results.size(); i++)
        {
            json local_json_results_tmp;
            local_json_results_tmp.push_back(nlohmann::json::object_t::value_type(string("score"), results[i].first));
            local_json_results_tmp.push_back(nlohmann::json::object_t::value_type(string("correction"), results[i].second));
            string tmp_str=results[i].second;
            results_tmp = askAutoSuggest(tmp_str, domain, count, threshold);
            local_json_results_tmp.push_back(nlohmann::json::object_t::value_type(string("suggestions"), results_tmp));
            json_results_tmp.push_back(local_json_results_tmp);
        }
        j.push_back(nlohmann::json::object_t::value_type(string("corrections"), json_results_tmp));
        results = askAutoSuggest(text, domain, count, threshold);
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
        cerr << "LOG: " << currentDateTime() << "\t" << s << endl;
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

std::vector<std::pair<float, std::string>>
rest_server::askAutoComplete(std::string &text, std::string &domain,
                               int count, float threshold) {
  std::vector<std::pair<float, std::string>> to_return;
  if ((int)text.size() > 0) {
    auto it_suggest = std::find_if(_list_suggest.begin(), _list_suggest.end(),
                                   [&](suggest *l_suggest) {
                                     return l_suggest->getDomain() == domain;
                                   });
    if (it_suggest != _list_suggest.end()) {
      to_return = (*it_suggest)->process_query_autocomplete(text, count);
    }
  }
  return to_return;
}

std::vector<std::pair<float, std::string>>
rest_server::askAutoSuggest(std::string &text, std::string &domain,
                               int count, float threshold) {
  std::vector<std::pair<float, std::string>> to_return;
  if ((int)text.size() > 0) {
    auto it_suggest = std::find_if(_list_suggest.begin(), _list_suggest.end(),
                                   [&](suggest *l_suggest) {
                                     return l_suggest->getDomain() == domain;
                                   });
    if (it_suggest != _list_suggest.end()) {
      to_return = (*it_suggest)->process_query_autosuggest(text, count);
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


