// -*- mode:c++; c-basic-offset:4 -*-
#if !defined __SIMPLEFIXEDOBJECTALLOCATOR_H
#define __SIMPLEFIXEDOBJECTALLOCATOR_H

#include <iostream>
#include <vector>
#include <utility>
#include <algorithm>
#include <stack>
#include <stdio.h>
#include <sstream>
#include <assert.h>

#include "types.h"
#include "utils.h"

template <typename T>
class SimpleFixedObjectAllocator {
    T *memory;
    uint_t n;
    uint_t start;

public:
    SimpleFixedObjectAllocator(uint_t _n)
        : memory(NULL), n(_n), start(0) {
        memory = (T*)operator new(sizeof(T) * n);
    }

    T *get() {
        assert_lt(start, n);
        return memory + (start++);
    }

    void put(T *mem) { }

    void clear() {
        operator delete(memory);
        memory = NULL;
    }

    ~SimpleFixedObjectAllocator() {
        clear();
    }
};

#endif // __SIMPLEFIXEDOBJECTALLOCATOR_H
