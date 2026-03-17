//
// Created by GUIYU on 2026/2/24.
//

#ifndef BLOCKMAPCPP_FLATFACING_H
#define BLOCKMAPCPP_FLATFACING_H
#include <cstdint>
#include <type_traits>

namespace BlockMapCpp {
    enum FlatFacing : std::uint8_t {
        None = 0,
        North = 1 << 0,
        South = 1 << 1,
        West  = 1 << 2,
        East  = 1 << 3,
        NorthEast = 1 << 4,
        NorthWest = 1 << 5,
        SouthEast = 1 << 6,
        SouthWest = 1 << 7
    };

    inline FlatFacing operator|(const FlatFacing a, const FlatFacing b) {
        using U = std::underlying_type<FlatFacing>::type;
        return static_cast<FlatFacing>(static_cast<U>(a) | static_cast<U>(b));
    }

    inline FlatFacing operator|=(FlatFacing& a, const FlatFacing b) {
        a = a | b;
        return a;
    }
}

#endif //BLOCKMAPCPP_FLATFACING_H