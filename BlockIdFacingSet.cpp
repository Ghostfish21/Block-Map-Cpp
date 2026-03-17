//
// Created by GUIYU on 2026/2/21.
//

#include "BlockIdFacingSet.h"

namespace BlockMapCpp {

    using std::uint16_t;

    void BlockIdFacingSet::setBlockId(uint16_t blockId) {
        this->blockId = blockId;
    }
    uint16_t BlockIdFacingSet::getBlockId() const {
        return this->blockId;
    }

    void BlockIdFacingSet::addFacing(const FlatFacing facing) {
        this->facingSet |= facing;
    }
    FlatFacing BlockIdFacingSet::getFacings() const {
        return this->facingSet;
    }

    bool BlockIdFacingSet::isBIFS() const {
        return this->isBifs;
    }

}