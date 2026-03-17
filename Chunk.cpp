//
// Created by GUIYU on 2026/2/24.
//

#include "Chunk.h"

namespace BlockMapCpp {

    Chunk::Chunk() {
        this->storageSpace.reserve(128);
    }

    uint16_t Chunk::sizeOfStorageSlot(uint16_t storageIndex) const {
        return this->storageSpace[storageIndex].size();
    }

    void Chunk::addBifsToStorageSlot(uint16_t storageIndex, uint16_t blockId, FlatFacing facing) {
        this->storageSpace[storageIndex].addBifs(blockId, facing);
    }

    uint16_t Chunk::allocStorageSlot() {
        uint16_t size = this->storageSpace.size();
        this->storageSpace.emplace_back(StorageSlot::Inline);
        return size;
    }

    void Chunk::addBifsToPos(std::uint8_t x, std::uint8_t y, std::uint8_t z, std::uint16_t blockId, FlatFacing facing) {
        Chunk *final = this;
        this->grid[x][y][z].addBifs(blockId, facing, final);
    }

}