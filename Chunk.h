//
// Created by GUIYU on 2026/2/24.
//

#ifndef BLOCKMAPCPP_CHUNK_H
#define BLOCKMAPCPP_CHUNK_H
#include "BlockSet.h"
#include "StorageSlot.h"
#include <vector>

namespace BlockMapCpp {
    class Chunk {
    private:

    public:
        BlockSet grid[16][16][16];
        std::vector<StorageSlot> storageSpace;
        Chunk();

        uint16_t sizeOfStorageSlot(uint16_t storageIndex) const;
        void addBifsToStorageSlot(uint16_t storageIndex, uint16_t blockId, FlatFacing facing);

        uint16_t allocStorageSlot();

        void addBifsToPos(std::uint8_t x, std::uint8_t y, std::uint8_t z, std::uint16_t blockId, FlatFacing facing);
    };
}

#endif //BLOCKMAPCPP_CHUNK_H