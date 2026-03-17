//
// Created by Gf115 on 2026/2/24.
//

// NOTE This cpp file is unused

#ifndef BLOCKMAPCPP_ARRAYSTACK_H
#define BLOCKMAPCPP_ARRAYSTACK_H
#include <cstdint>

namespace BlockMapCpp {
    class ArrayStack {
        static constexpr int CAPACITY = 4096;

    private:
        std::uint16_t nodes[CAPACITY] {};
        std::uint16_t cursor = 0;

    public:
        ArrayStack() = default;

        void push(std::uint16_t value);
        std::uint16_t pop();
    };
}

#endif //BLOCKMAPCPP_ARRAYSTACK_H