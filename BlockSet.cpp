//
// Created by GUIYU on 2026/2/24.
//

#include "BlockSet.h"

#include "Chunk.h"

namespace BlockMapCpp {
    using std::size_t;

    // NOTE 先暂时尝试，从 BlockSet 的 StorageSlot 移除元素时不会自动退回先前形态
    // 例如，它不会因为 StorageSlot 中存储的方块种类从 2 变回了 1，就退化为普通 BIFS
    // 这样可能会导致更精准的预测 - 曾经是 StorageSlot 的地方，以后也更有可能是
    // 也可能导致内存的浪费。等到写完之后可以测试一下
    // 如果从 BlockSet 中删除元素会导致 BlockSet 退化，那么 empty 方法就不需要 Chunk 的输入了
    bool BlockSet::empty(const Chunk &chunk) const {
        if (bifs.isBIFS()) {
            if (bifs.getFacings() == FlatFacing::None) return true;
            return false;
        }

        if (chunk.sizeOfStorageSlot(bifs.getBlockId()) == 0) return true;
        return false;
    }

    // NOTE 同上
    BlockSet::size_type BlockSet::size(const Chunk &chunk) const {
        if (bifs.isBIFS()) {
            if (bifs.getFacings() == FlatFacing::None) return 0;
            return 1;
        }

        return chunk.sizeOfStorageSlot(bifs.getBlockId());
    }

    void BlockSet::addBifs(const uint16_t blockId, const FlatFacing facing, Chunk *chunk) {
        // 如果 bifs 本来是一个真的 BIFS
        if (bifs.isBIFS()) {
            // bifs 是空的，就直接占用它
            if (bifs.getFacings() == FlatFacing::None) {
                bifs.setBlockId(blockId);
                bifs.addFacing(facing);
            }
            // 虽然 bifs 不是空的，但是现有 bifs 和要添加的 bifs 种类一致，就合并它们的 FlatFacing
            else if (bifs.getBlockId() == blockId) {
                bifs.addFacing(facing);
            }
            // bifs 是一个非空，且 ID 不等于 要添加的方块的ID。也就是要添加第二种 bifs，则向 chunk 索要
            // 一个 StorageSlot，将原先 bifs 转化为本地临时值，新建 StorageIndex类bifs 并将 StorageIndex
            // 写入，随后将原本的 bifs 和 要添加的方块 都写入 StorageSlot 中
            else {
                const uint16_t storageIndex = chunk->allocStorageSlot();
                const uint16_t oldBlockId = bifs.getBlockId();
                const FlatFacing oldFacing = bifs.getFacings();
                bifs = BlockIdFacingSet(storageIndex);
                chunk->addBifsToStorageSlot(storageIndex, oldBlockId, oldFacing);
                chunk->addBifsToStorageSlot(storageIndex, blockId, facing);
            }
            return;
        }

        // 如果 bifs 其实是 StorageIndex
        chunk->addBifsToStorageSlot(bifs.getBlockId(), blockId, facing);
    }
}
