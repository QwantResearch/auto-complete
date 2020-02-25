// Copyright 2019 Qwant Research. Licensed under the terms of the Apache 2.0
// license. See LICENSE in the project root.

#ifndef __UTILS_H
#define __UTILS_H

#include <string>
#include <vector>
#include <time.h>
#include <ctime>    
#include <iostream>
#include <istream>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <fstream>

#include <pistache/client.h>
#include <pistache/endpoint.h>
#include <pistache/http.h>
#include <pistache/router.h>

#include "segtree.h"
#include "types.h"

#if defined DEBUG
#define DCERR(X) std::cerr<<X;
#define DPRINTF(ARGS...) fprintf(stderr, ARGS);
#else
#define DCERR(X)
#define DPRINTF(ARGS...)
#endif

#define assert_lt(X,Y) if (!((X)<(Y))) { fprintf(stderr, "%d < %d FAILED\n", (X), (Y)); assert((X)<(Y)); }
#define assert_gt(X,Y) if (!((X)>(Y))) { fprintf(stderr, "%d > %d FAILED\n", (X), (Y)); assert((X)>(Y)); }
#define assert_le(X,Y) if (!((X)<=(Y))) { fprintf(stderr, "%d <= %d FAILED\n", (X), (Y)); assert((X)<=(Y)); }
#define assert_eq(X,Y) if (!((X)==(Y))) { fprintf(stderr, "%d == %d FAILED\n", (X), (Y)); assert((X)==(Y)); }
#define assert_ne(X,Y) if (!((X)!=(Y))) { fprintf(stderr, "%d != %d FAILED\n", (X), (Y)); assert((X)!=(Y)); }



#if !defined NMAX
#define NMAX 32
#endif


#if !defined INPUT_LINE_SIZE
// Max. line size is 8191 bytes.
#define INPUT_LINE_SIZE 8192
#endif

// How many bytes to reserve for the output string
#define OUTPUT_SIZE_RESERVE 4096

#define BOUNDED_RETURN(CH,LB,UB,OFFSET) if (ch >= LB && CH <= UB) { return CH - LB + OFFSET; }

inline int hex2dec(unsigned char ch) 
{
    BOUNDED_RETURN(ch, '0', '9', 0);
    BOUNDED_RETURN(ch, 'A', 'F', 10);
    BOUNDED_RETURN(ch, 'a', 'f', 10);
    return 0;
}

#undef BOUNDED_RETURN


struct InputLineParser {
    int state;            // Current parsing state
    const char *mem_base; // Base address of the mmapped file
    const char *buff;     // A pointer to the current line to be parsed
    size_t buff_offset;   // Offset of 'buff' [above] relative to the beginning of the file. Used to index into mem_base
    int *pn;              // A pointer to any integral field being parsed
    std::string *pphrase; // A pointer to a string field being parsed

    // The input file is mmap()ped in the process' address space.

    StringProxy *psnippet_proxy; // The psnippet_proxy is a pointer to a Proxy String object that points to memory in the mmapped region

    InputLineParser(const char *_mem_base, size_t _bo, 
                    const char *_buff, int *_pn, 
                    std::string *_pphrase, StringProxy *_psp)
        : state(ILP_BEFORE_NON_WS), mem_base(_mem_base), buff(_buff), 
          buff_offset(_bo), pn(_pn), pphrase(_pphrase), psnippet_proxy(_psp)
    { }

    void
    start_parsing(char * if_mmap_addr, off_t if_length) {
        int i = 0;                  // The current record byte-offset.
        int n = 0;                  // Temporary buffer for numeric (integer) fields.
        const char *p_start = NULL; // Beginning of the phrase.
        const char *s_start = NULL; // Beginning of the snippet.
        int p_len = 0;              // Phrase Length.
        int s_len = 0;              // Snippet length.

        while (this->buff[i]) {
            char ch = this->buff[i];
            DCERR("["<<this->state<<":"<<ch<<"]");

            switch (this->state) {
            case ILP_BEFORE_NON_WS:
                if (!isspace(ch)) {
                    this->state = ILP_WEIGHT;
                }
                else {
                    ++i;
                }
                break;

            case ILP_WEIGHT:
                if (isdigit(ch)) {
                    n *= 10;
                    n += (ch - '0');
                    ++i;
                }
                else {
                    this->state = ILP_BEFORE_PTAB;
                    on_weight(n);
                }
                break;

            case ILP_BEFORE_PTAB:
                if (ch == '\t') {
                    this->state = ILP_AFTER_PTAB;
                }
                ++i;
                break;

            case ILP_AFTER_PTAB:
                if (isspace(ch)) {
                    ++i;
                }
                else {
                    p_start = this->buff + i;
                    this->state = ILP_PHRASE;
                }
                break;

            case ILP_PHRASE:
                // DCERR("State: ILP_PHRASE: "<<buff[i]<<endl);
                if (ch != '\t') {
                    ++p_len;
                }
                else {
                    // Note: Skip to ILP_SNIPPET since the snippet may
                    // start with a white-space that we wish to
                    // preserve.
                    // 
                    this->state = ILP_SNIPPET;
                    s_start = this->buff + i + 1;
                }
                ++i;
                break;

            case ILP_SNIPPET:
                ++i;
                ++s_len;
                break;

            };
        }
        DCERR("\n");
        on_phrase(p_start, p_len);
        on_snippet(s_start, s_len,if_mmap_addr,if_length);
    }

    void
    on_weight(int n) {
        *(this->pn) = n;
    }

    void
    on_phrase(const char *data, int len) {
        if (len && this->pphrase) {
            // DCERR("on_phrase("<<data<<", "<<len<<")\n");
            this->pphrase->assign(data, len);
        }
    }

    void
    on_snippet(const char *data, int len, char * if_mmap_addr, off_t if_length) 
    {
        if (len && this->psnippet_proxy) {
            const char *base = this->mem_base + this->buff_offset + 
                (data - this->buff);
            if (base < if_mmap_addr || base + len > if_mmap_addr + if_length) {
                fprintf(stderr, "base: %p, if_mmap_addr: %p, if_mmap_addr+if_length: %p\n", base, if_mmap_addr, if_mmap_addr + if_length);
                assert(base >= if_mmap_addr);
                assert(base <= if_mmap_addr + if_length);
                assert(base + len <= if_mmap_addr + if_length);
            }
            DCERR("on_snippet::base: "<<(void*)base<<", len: "<<len<<"\n");
            this->psnippet_proxy->assign(base, len);
        }
    }

};




inline std::string uint_to_string(uint_t n, uint_t pad = 0) 
{
    std::string ret;
    if (!n) {
        ret = "0";
    }
    else {
        while (n) {
            ret.insert(0, 1, ('0' + (n % 10)));
            n /= 10;
        }
    }
    while (pad && ret.size() < pad) {
        ret = "0" + ret;
    }

    return ret;
}


inline uint_t log2(uint_t n) 
{
    uint_t lg2 = 0;
    while (n > 1) {
        n /= 2;
        ++lg2;
    }
    return lg2;
}

const uint_t minus_one = (uint_t)0 - 1;

std::string results_json(std::string q, vp_t& suggestions, std::string const& type) ;
std::string suggestions_json_array(vp_t& suggestions) ;
std::string rich_suggestions_json_array(vp_t& suggestions) ;



template <typename T> std::ostream& operator<<(std::ostream& out, std::vector<T> const& vec) 
{
    for (size_t i = 0; i < vec.size(); ++i) {
        out<<vec[i]<<std::endl;
    }
    return out;
}

template <typename T, typename U> std::ostream& operator<<(std::ostream& out, std::pair<T, U> const& p) 
{
    out<<"("<<p.first<<", "<<p.second<<")";
    return out;
}

inline std::ostream& operator<<(std::ostream& out, phrase_t const& p) 
{
    out<<"("<<p.phrase<<", "<<p.weight<<")";
    return out;
}


std::string trim(std::string& str);

int edit_distance(std::string const& lhs, std::string const& rhs);

std::string bitmap_str(uint_t i);

void Split(const std::string &line, std::vector<std::string> &pieces,
           const std::string del);

void printCookies(const Pistache::Http::Request &req);

const std::string currentDateTime();

bool mySortingFunctionFloatString ( const std::pair<float, std::string>& i, const std::pair<float, std::string>& j );

namespace Generic {
void handleReady(const Pistache::Rest::Request &req, Pistache::Http::ResponseWriter response);
} // namespace Generic

#endif // __UTILS_H
