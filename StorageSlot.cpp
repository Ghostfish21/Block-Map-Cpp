//
// Created by Gf115 on 2026/2/24.
//

#include "StorageSlot.h"

#include <cstring>

#include "BlockIdFacingSet.h"
#include <new>      // placement new
#include <utility>  // std::move
#include <memory>   // std::addressof

namespace BlockMapCpp {
    using std::vector;
    StorageSlot::Value::Value() {}
    StorageSlot::Value::~Value() {}

    StorageSlot::StorageSlot(const Kind kind) {
        this->kind = kind;
        if (kind == StorageSlot::Kind::Inline)
            new (&this->value.inlineBifs) BlockIdFacingSet[InlineAmount];
        else value.fallback = new std::vector<BlockIdFacingSet>;
    }
    StorageSlot::~StorageSlot() {
        if (this->kind != StorageSlot::Kind::Fallback) return;
        delete value.fallback;
    }

    uint16_t StorageSlot::size() const {
        if (this->kind == StorageSlot::Kind::Fallback)
            return this->value.fallback->size();

        uint16_t ret = 0;
        for (auto bifs : this->value.inlineBifs) {
            if (bifs.getFacings() != FlatFacing::None)
                ret++;
        }
        return ret;
    }

    StorageSlot::StorageSlot(StorageSlot &&other) noexcept : kind(other.kind) {
        if (this->kind == StorageSlot::Kind::Inline) {
            for (uint8_t i = 0; i < InlineAmount; i++) {
                new (static_cast<void*>(std::addressof(value.inlineBifs[i]))) BlockIdFacingSet(other.value.inlineBifs[i]);
            }
            return;
        }

        value.fallback = other.value.fallback;
        other.value.fallback = nullptr;
    }


    StorageSlot &StorageSlot::operator=(StorageSlot &&other) noexcept {
        if (this == &other) return *this;
        this->~StorageSlot();
        new (this) StorageSlot(std::move(other));
        return *this;
    }

    void StorageSlot::addBifs(const uint16_t blockId, const FlatFacing facing) {
        // 如果当前 StorageSlot 的储存格式是通过 Inline 储存，那么直接存就可以了
        if (this->kind == StorageSlot::Kind::Inline) {
            // 在遍历中，检查是否有任何现存 bifs 的种类和给定的种类一致。如果有的话，合并它的面
            // 本次遍历也会检查是否存在空 bifs。如果存在，就将它带出来。如果遍历结束了没有发现相同 id
            // 的 bifs，就用空 bifs
            bool foundEmptyBifs = false;
            uint16_t emptyBifsIndex = StorageSlot::InlineAmount;
            for (uint8_t i = 0; i < StorageSlot::InlineAmount; i++) {
                BlockIdFacingSet bifs = this->value.inlineBifs[i];
                if (bifs.getBlockId() == blockId) {
                    bifs.addFacing(facing);
                    this->value.inlineBifs[i] = bifs;
                    return;
                }
                if (!foundEmptyBifs && bifs.getFacings() == FlatFacing::None) {
                    foundEmptyBifs = true;
                    emptyBifsIndex = i;
                }
            }
            // 如果列表里没有任何相同 id bifs。则检查是否存在空 bifs
            if (foundEmptyBifs) {
                BlockIdFacingSet bifs = this->value.inlineBifs[emptyBifsIndex];
                bifs.setBlockId(blockId);
                bifs.addFacing(facing);
                this->value.inlineBifs[emptyBifsIndex] = bifs;
                return;
            }
            // 如果列表里既没有任何相同 id bifs，也没有任何空 bifs，说明列表已经满了。
            // 临时保存当前列表里所有的 bifs，将自己的类型改变为 fallback 并创建 vector 数组
            // 将所有临时 bifs 添加至 vector 数组，随后添加新的 bifs (不需要检查 新加的 bifs 是否可以合并或占用
            // 空 bifs，因为上面已经检查过了)
            BlockIdFacingSet inlineBifs[StorageSlot::InlineAmount];
            std::memcpy(inlineBifs, this->value.inlineBifs, sizeof(inlineBifs));
            this->kind = StorageSlot::Kind::Fallback;
            this->value.fallback = new vector<BlockIdFacingSet>;
            for (uint8_t i = 0; i < StorageSlot::InlineAmount; i++) {
                BlockIdFacingSet bifs = inlineBifs[i];
                this->value.fallback->emplace_back(bifs.getBlockId(), bifs.getFacings());
            }
            this->value.fallback->emplace_back(blockId, facing);
            return;
        }
        
        // 如果当前 StorageSlot 的储存格式是 Fallback，遍历所有现存值，查找是否存在相同 id bifs，如果有直接合并
        for (auto& bifs : *this->value.fallback) {
            if (bifs.getBlockId() != blockId) continue;
            bifs.addFacing(facing);
            return;
        }
        // 如果不存在相同 id bifs，就添加一个新的
        this->value.fallback->emplace_back(blockId, facing);
    }
}
