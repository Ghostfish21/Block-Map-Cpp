//
// Created by Gf115 on 2026/2/28.
//

#include "ArrayStack.h"

// NOTE This cpp file is unused

namespace BlockMapCpp {
    using std::uint16_t;

    void ArrayStack::push(const uint16_t elem) {
        this->nodes[this->cursor] = elem;
        this->cursor++;
    }

    uint16_t ArrayStack::pop() {
        this->cursor--;
        const uint16_t result = this->nodes[this->cursor];
        return result;
    }

}