//
// Created by Gf115 on 2026/2/24.
//

#ifndef BLOCKMAPCPP_STORAGESLOT_H
#define BLOCKMAPCPP_STORAGESLOT_H
#include "BlockIdFacingSet.h"
#include "FlatFacing.h"

namespace BlockMapCpp {

    class StorageSlot {
        static constexpr uint8_t InlineAmount = 6;

    public:
        enum Kind {
            Inline, Fallback
        };

        // 默认 构造 与 析构函数
        explicit StorageSlot(Kind kind);
        ~StorageSlot();

        uint16_t size() const;

        // 移动
        StorageSlot(StorageSlot &&other) noexcept;
        StorageSlot& operator=(StorageSlot &&other) noexcept;


        void addBifs(uint16_t blockId, FlatFacing facing);

        Kind kind = Inline;

        union Value {
            BlockIdFacingSet inlineBifs[InlineAmount];
            std::vector<BlockIdFacingSet> *fallback{};

            Value();
            ~Value();
        } value;
    private:

    };

}


#endif //BLOCKMAPCPP_STORAGESLOT_H