// Copyright 2019 Qwant Research. Licensed under the terms of the Apache 2.0
// license. See LICENSE in the project root.

#include "utils.h"

using namespace std;

void printCookies(const Pistache::Http::Request &req) {
  auto cookies = req.cookies();
  const std::string indent(4, ' ');

  std::cout << "Cookies: [" << std::endl;
  for (const auto &c : cookies) {
    std::cout << indent << c.name << " = " << c.value << std::endl;
  }
  std::cout << "]" << std::endl;
}

void Split(const std::string &line, std::vector<std::string> &pieces,
           const std::string del) {
  size_t begin = 0;
  size_t pos = 0;
  std::string token;

  while ((pos = line.find(del, begin)) != std::string::npos) {
    if (pos > begin) {
      token = line.substr(begin, pos - begin);
      if (token.size() > 0)
        pieces.push_back(token);
    }
    begin = pos + del.size();
  }
  if (pos > begin) {
    token = line.substr(begin, pos - begin);
  }
  if (token.size() > 0)
    pieces.push_back(token);
}

const std::string currentDateTime() {
  time_t now = time(0);
  struct tm tstruct;
  char buf[80];
  tstruct = *localtime(&now);
  // Visit http://en.cppreference.com/w/cpp/chrono/c/strftime
  // for more information about date/time format
  strftime(buf, sizeof(buf), "%Y-%m-%d.%X", &tstruct);

  return buf;
}

int edit_distance(std::string const& lhs, std::string const& rhs) {
    const int n = lhs.size();
    const int m = rhs.size();
    int *mem = (int*)malloc((m + m + 2) * sizeof(int));
    int *dp0 = mem+1, *dp1 = mem+m+2;
    dp0[-1] = 0;
    for (int i = 0; i < m; ++i) {
        dp0[i] = i + 1;
    }

    for (int i = 0; i < n; ++i) {
        dp1[-1] = i+1;
        for (int j = 0; j < m; ++j) {
            if (lhs[i] == rhs[j]) {
                dp1[j] = dp0[j-1];
            }
            else {
                int m1 = dp0[j] + 1;
                int m2 = dp1[j-1] + 1;
                int m3 = dp0[j-1] + 1;
                m1 = m1 < m2 ? m1 : m2;
                m1 = m1 < m3 ? m1 : m3;
                dp1[j] = m1;
            }
        }
        std::swap(dp0, dp1);
    }
    int ret = dp0[m-1];
    free(mem);
    return ret;
}

std::string bitmap_str(uint_t i) 
{
    std::string out;
    for (int x = 17; x >= 0; --x) {
        out += '0' + ((i & (1L << x)) ? 1 : 0);
    }
    return out;
}


off_t
file_size(const char *path) {
    struct stat sbuf;
    int r = stat(path, &sbuf);

    assert(r == 0);
    if (r < 0) {
        return 0;
    }

    return sbuf.st_size;
}

// TODO: Fix for > 2GiB memory usage by using uint64_t
int
get_memory_usage(pid_t pid) {
    char cbuff[4096];
    sprintf(cbuff, "pmap -x %d | tail -n +3 | awk 'BEGIN { S=0;T=0 } { if (match($3, /\\-/)) {S=1} if (S==0) {T+=$3} } END { print T }'", pid);
    FILE *pf = popen(cbuff, "r");
    if (!pf) {
        return -1;
    }
    int r = fread(cbuff, 1, 100, pf);
    if (r < 0) {
        fclose(pf);
        return -1;
    }
    cbuff[r-1] = '\0';
    r = atoi(cbuff);
    fclose(pf);
    return r;
}

char
to_lowercase(char c) {
    return std::tolower(c);
}

inline void
str_lowercase(std::string &str) {
    std::transform(str.begin(), str.end(), 
                   str.begin(), to_lowercase);

}


std::string
unescape_query(std::string const &query) {
    enum {
        QP_DEFAULT = 0,
        QP_ESCAPED1 = 1,
        QP_ESCAPED2 = 2
    };
    std::string ret;
    unsigned char echar = 0;
    int state = QP_DEFAULT;
    for (size_t i = 0; i < query.size(); ++i) {
        switch (state) {
        case QP_DEFAULT:
            if (query[i] != '%') {
                ret += query[i];
            } else {
                state = QP_ESCAPED1;
                echar = 0;
            }
            break;
        case QP_ESCAPED1:
        case QP_ESCAPED2:
            echar *= 16;
            echar += hex2dec(query[i]);
            if (state == QP_ESCAPED2) {
                ret += echar;
            }
            state = (state + 1) % 3;
        }
    }
    return ret;
}

void escape_special_chars(std::string& str) 
{
    std::string ret;
    ret.reserve(str.size() + 10);
    for (size_t j = 0; j < str.size(); ++j) {
        switch (str[j]) {
        case '"':
            ret += "\\\"";
            break;

        case '\\':
            ret += "\\\\";
            break;

        case '\n':
            ret += "\\n";
            break;

        case '\t':
            ret += "\\t";
            break;

        default:
            ret += str[j];
        }
    }
    ret.swap(str);
}

std::string rich_suggestions_json_array(vp_t& suggestions) 
{
    std::string ret = "[";
    ret.reserve(OUTPUT_SIZE_RESERVE);
    for (vp_t::iterator i = suggestions.begin(); i != suggestions.end(); ++i) {
        escape_special_chars(i->phrase);
        std::string snippet = i->snippet;
        escape_special_chars(snippet);

        std::string trailer = i + 1 == suggestions.end() ? "\n" : ",\n";
        ret += " { \"phrase\": \"" + i->phrase + "\", \"score\": " + uint_to_string(i->weight) + 
            (snippet.empty() ? "" : ", \"snippet\": \"" + snippet + "\"") + " }" + trailer;
    }
    ret += "]";
    return ret;
}

std::string suggestions_json_array(vp_t& suggestions) 
{
    std::string ret = "[";
    ret.reserve(OUTPUT_SIZE_RESERVE);
    for (vp_t::iterator i = suggestions.begin(); i != suggestions.end(); ++i) {
        escape_special_chars(i->phrase);

        std::string trailer = i + 1 == suggestions.end() ? "\n" : ",\n";
        ret += "\"" + i->phrase + "\"" + trailer;
    }
    ret += "]";
    return ret;
}

std::string results_json(std::string q, vp_t& suggestions, std::string const& type) 
{
    if (type == "list") {
        escape_special_chars(q);
        return "[ \"" + q + "\", " + suggestions_json_array(suggestions) + " ]";
    }
    else {
        return rich_suggestions_json_array(suggestions);
    }
}

std::string pluralize(std::string s, int n) 
{
    return n>1 ? s+"s" : s;
}

std::string humanized_time_difference(time_t prev, time_t curr) 
{
    std::string ret = "";
    if (prev > curr) {
        std::swap(prev, curr);
    }

    if (prev == curr) {
        return "just now";
    }

    int sec = curr - prev;
    ret = uint_to_string(sec % 60, 2) + ret;

    int minute = sec / 60;
    ret = uint_to_string(minute % 60, 2) + ":" + ret;

    int hour = minute / 60;
    ret = uint_to_string(hour % 24, 2) + ":" + ret;

    int day = hour / 24;
    if (day) {
        ret = uint_to_string(day % 7) + pluralize(" day", day%7) + " " + ret;
    }

    int week = day / 7;
    if (week) {
        ret = uint_to_string(week % 4) + pluralize(" day", week%4) + " " + ret;
    }

    int month = week / 4;
    if (month) {
        ret = uint_to_string(month) + pluralize(" month", month) + " " + ret;
    }

    return ret;
}


std::string get_uptime(time_t started_at) 
{
    return humanized_time_difference(started_at, time(NULL));
}

bool is_EOF(FILE *pf) { return feof(pf); }
bool is_EOF(std::ifstream fin) { return !!fin; }

void get_line(FILE *pf, char *buff, int buff_len, int &read_len) {
    char *got = fgets(buff, INPUT_LINE_SIZE, pf);
    if (!got) {
        read_len = -1;
        return;
    }
    read_len = strlen(buff);
    if (read_len > 0 && buff[read_len - 1] == '\n') {
        buff[read_len - 1] = '\0';
    }
}

void get_line(std::ifstream fin, char *buff, int buff_len, int &read_len) 
{
    fin.getline(buff, buff_len);
    read_len = fin.gcount();
    buff[INPUT_LINE_SIZE - 1] = '\0';
}


bool mySortingFunctionFloatString ( const std::pair<float, std::string>& i, const std::pair<float, std::string>& j )
{
    if ( i.first < j.first ) return false;
    if ( j.first <= i.first ) return true;
    return true;
//      return j.second < i.second;
}

std::string trim(std::string& str)
{
    str.erase(0, str.find_first_not_of(' '));       //prefixing spaces
    str.erase(str.find_last_not_of(' ')+1);         //surfixing spaces
    return str;
}


namespace Generic {
void handleReady(const Pistache::Rest::Request &req, Pistache::Http::ResponseWriter response) {
  response.send(Pistache::Http::Code::Ok, "1");
}
} // namespace Generic
