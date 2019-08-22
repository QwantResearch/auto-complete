#if !defined __TYPES_H
#define __TYPES_H

#include <utility>
#include <vector>
#include <string>
#include <string.h>
#include <assert.h>

#if !defined RMQ
#define RMQ SegmentTree
// #define RMQ SparseTable
#endif
struct phrase_t;
// struct StringProxy; 

typedef unsigned int uint_t;


typedef std::vector<phrase_t> vp_t;
typedef vp_t::iterator vpi_t;
typedef std::pair<vpi_t, vpi_t> pvpi_t;

typedef std::pair<uint_t, uint_t> pui_t;
typedef std::vector<uint_t> vui_t;
typedef std::vector<vui_t> vvui_t;
typedef std::vector<pui_t> vpui_t;
typedef std::vector<vpui_t> vvpui_t;

typedef std::pair<std::string, uint_t> psui_t;
typedef std::vector<psui_t> vpsui_t;
typedef std::vector<std::string> vs_t;
typedef std::pair<vs_t::iterator, vs_t::iterator> pvsi_t;
typedef std::pair<vpsui_t::iterator, vpsui_t::iterator> pvpsuii_t;

typedef std::vector<char> vc_t;
typedef std::vector<vc_t> vvc_t;
typedef std::vector<vvc_t> vvvc_t;

typedef vc_t   char_array_1d_t;
typedef vvc_t  char_array_2d_t;
typedef vvvc_t char_array_3d_t;


enum {
    // We are in a non-WS state
    ILP_BEFORE_NON_WS  = 0,

    // We are parsing the weight (integer)
    ILP_WEIGHT         = 1,

    // We are in the state after the weight but before the TAB
    // character separating the weight & the phrase
    ILP_BEFORE_PTAB    = 2,

    // We are in the state after the TAB character and potentially
    // before the phrase starts (or at the phrase)
    ILP_AFTER_PTAB     = 3,

    // The state parsing the phrase
    ILP_PHRASE         = 4,

    // The state after the TAB character following the phrase
    // (currently unused)
    ILP_AFTER_STAB     = 5,

    // The state in which we are parsing the snippet
    ILP_SNIPPET        = 6
};

enum { IMPORT_FILE_NOT_FOUND = 1,
       IMPORT_MUNMAP_FAILED  = 2,
       IMPORT_MMAP_FAILED    = 3
};




struct StringProxy {
    const char *mem_base;
    int len;

    StringProxy(const char *_mb = NULL, int _l = 0)
        : mem_base(_mb), len(_l)
    { }

    void
    assign(const char *_mb, int _l) {
        this->mem_base = _mb;
        this->len = _l;
    }

    size_t
    size() const {
        return this->len;
    }

    operator std::string() const {
        // Basic sanity checking. Make sure that this->len is in the
        // range [0..64k].
        assert(this->len >= 0 && this->len < 64000);
        return std::string(this->mem_base, this->len);
    }

    void
    swap(StringProxy &rhs) {
        std::swap(this->mem_base, rhs.mem_base);
        std::swap(this->len, rhs.len);
    }

};

struct phrase_t {
    uint_t weight;
    std::string phrase;
    StringProxy snippet;

    phrase_t(uint_t _w, std::string const& _p, StringProxy const& _s)
        : weight(_w), phrase(_p), snippet(_s) {
    }

    void
    swap(phrase_t& rhs) {
        std::swap(this->weight, rhs.weight);
        this->phrase.swap(rhs.phrase);
        this->snippet.swap(rhs.snippet);
    }

    bool
    operator<(phrase_t const& rhs) const {
        return this->phrase < rhs.phrase;
    }
};

// Specialize std::swap for our type
namespace std {
  
    inline void swap(phrase_t& lhs, phrase_t& rhs) {
        lhs.swap(rhs);
    }
}

struct BinaryTreeNode {
    BinaryTreeNode *left, *right;
    uint_t data;
    uint_t index;

    BinaryTreeNode(BinaryTreeNode *_left, BinaryTreeNode *_right, uint_t _data, uint_t _index)
        : left(_left), right(_right), data(_data), index(_index)
    { }

    BinaryTreeNode(uint_t _data, uint_t _index)
        : left(NULL), right(NULL), data(_data), index(_index)
    { }
};

struct PrefixFinder {
    bool
    operator()(std::string const& prefix, phrase_t const &target) {
#if 1
        const int ppos = target.phrase.compare(0, prefix.size(), prefix);
        return ppos > 0;
#else
        const int ppos = target.phrase.find(prefix);
        if (!ppos) {
            return false;
        }
        return prefix < target.phrase;
#endif
    }

    bool
    operator()(phrase_t const& target, std::string const &prefix) {
#if 1
        const int ppos = target.phrase.compare(0, prefix.size(), prefix);
        return ppos < 0;
#else
        const int ppos = target.phrase.find(prefix);
        if (!ppos) {
            return false;
        }
        return target.phrase < prefix;
#endif
    }
};



#endif // TYPES_H
