//
// Created by GUIYU on 2026/2/24.
//

#ifndef BLOCKMAPCPP_BLOCKIDFACINGSET_H
#define BLOCKMAPCPP_BLOCKIDFACINGSET_H
#include <vector>
#include <cstddef>
#include "FlatFacing.h"

namespace BlockMapCpp {
    // 存储在特定位置网格里的特定 BlockId 里，的这个 BlockId 是什么，以及这里存在哪些朝向的变体
    class BlockIdFacingSet {
    private:
        std::uint16_t blockId = 0;
        FlatFacing facingSet = FlatFacing::None;
        bool isBifs = false;

    public:
        void setBlockId(std::uint16_t blockId);
        std::uint16_t getBlockId() const;

        void addFacing(FlatFacing facing);
        FlatFacing getFacings() const;

        bool isBIFS() const;

        BlockIdFacingSet() = default;
        BlockIdFacingSet(const std::uint16_t blockId, const FlatFacing facing) : blockId(blockId),
            facingSet(facing), isBifs(true) {}
        explicit BlockIdFacingSet(const std::uint16_t storageIndex) : blockId(storageIndex),
            facingSet(FlatFacing::None), isBifs(false) {}
    };

}

#endif //BLOCKMAPCPP_BLOCKIDFACINGSET_H