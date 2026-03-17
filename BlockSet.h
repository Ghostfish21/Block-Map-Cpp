//
// Created by GUIYU on 2026/2/24.
//

#ifndef BLOCKMAPCPP_BLOCKSET_H
#define BLOCKMAPCPP_BLOCKSET_H
#include <cstdint>
#include "FlatFacing.h"
#include "BlockIdFacingSet.h"

namespace BlockMapCpp {
    class Chunk;

    class BlockSet {
    private:
        BlockIdFacingSet bifs;

    public:
        using size_type = std::uint16_t;

        size_type size(const Chunk &chunk) const;
        bool empty(const Chunk &chunk) const;

        void addBifs(uint16_t blockId, FlatFacing facing, Chunk *chunk);

        BlockSet() : bifs(0, FlatFacing::None) {}
    };

}

#endif //BLOCKMAPCPP_BLOCKSET_H