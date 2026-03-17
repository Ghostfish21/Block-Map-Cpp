#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <iomanip>
#include <iostream>
#include <limits>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

// IMPORTANT NOTE
// LLM USAGE:
// This cpp file is written by ChatGPT

// 先包含标准库，再临时把 private 提升为 public，避免污染标准头。
#define private public
#include "Chunk.h"
#include "BlockSet.h"
#include "BlockIdFacingSet.h"
#include "FlatFacing.h"
#include "StorageSlot.h"
#include "benchmark.h"
#undef private

namespace bench {
using namespace BlockMapCpp;

static const std::size_t kChunkEdge = 16;
static const std::size_t kChunkVoxelCount = kChunkEdge * kChunkEdge * kChunkEdge; // 4096
static const std::size_t kDefaultRepeats = 5;
static const std::uint32_t kDefaultSeed = 20260317u;
static const std::uint8_t kStorageSlotInlineAmount = 6;

static volatile std::uint64_t g_sink = 0;

static const FlatFacing kFacings[8] = {
    FlatFacing::North,
    FlatFacing::South,
    FlatFacing::West,
    FlatFacing::East,
    FlatFacing::NorthEast,
    FlatFacing::NorthWest,
    FlatFacing::SouthEast,
    FlatFacing::SouthWest
};

struct Options {
    std::size_t repeats;
    std::uint32_t seed;

    Options() : repeats(kDefaultRepeats), seed(kDefaultSeed) {}
};

struct SampleResult {
    std::uint64_t logicalOps;
    std::size_t estimatedBytes;
    std::uint64_t checksum;

    SampleResult() : logicalOps(0), estimatedBytes(0), checksum(0) {}
    SampleResult(std::uint64_t logicalOps_, std::size_t estimatedBytes_, std::uint64_t checksum_)
        : logicalOps(logicalOps_), estimatedBytes(estimatedBytes_), checksum(checksum_) {}
};

struct BenchResult {
    std::string name;
    std::string note;
    std::size_t repeats;
    double avgMs;
    double avgNsPerOp;
    double avgOpsPerSec;
    double avgBytes;
    double minMs;
    double maxMs;
    std::uint64_t avgLogicalOps;
    bool ok;
    std::string error;

    BenchResult()
        : repeats(0), avgMs(0.0), avgNsPerOp(0.0), avgOpsPerSec(0.0), avgBytes(0.0),
          minMs(0.0), maxMs(0.0), avgLogicalOps(0), ok(true) {}
};

static std::string formatBytes(const double bytes) {
    static const char* kUnits[] = {"B", "KiB", "MiB", "GiB", "TiB"};
    double value = bytes;
    std::size_t unitIndex = 0;
    while (value >= 1024.0 && unitIndex + 1 < sizeof(kUnits) / sizeof(kUnits[0])) {
        value /= 1024.0;
        ++unitIndex;
    }

    std::ostringstream oss;
    oss << std::fixed << std::setprecision(value >= 100.0 ? 1 : 2) << value << ' ' << kUnits[unitIndex];
    return oss.str();
}

static std::uint64_t parseUnsigned(const char* text, const char* argName) {
    char* end = NULL;
    const unsigned long long value = std::strtoull(text, &end, 10);
    if (text == end || (end != NULL && *end != '\0')) {
        throw std::runtime_error(std::string("invalid value for ") + argName + ": " + text);
    }
    return static_cast<std::uint64_t>(value);
}

static Options parseOptions(int argc, char** argv) {
    Options options;
    for (int i = 1; i < argc; ++i) {
        const std::string arg(argv[i]);
        if (arg == "--repeats") {
            if (i + 1 >= argc) throw std::runtime_error("--repeats requires a value");
            options.repeats = static_cast<std::size_t>(parseUnsigned(argv[++i], "--repeats"));
        } else if (arg == "--seed") {
            if (i + 1 >= argc) throw std::runtime_error("--seed requires a value");
            options.seed = static_cast<std::uint32_t>(parseUnsigned(argv[++i], "--seed"));
        } else if (arg == "--help" || arg == "-h") {
            std::cout
                << "Usage: BlockMapCPPBenchmark [--repeats N] [--seed N]\n\n"
                << "说明:\n"
                << "  --repeats N   每个场景重复 N 次并输出平均值，默认 5\n"
                << "  --seed N      随机种子，默认 20260317\n\n"
                << "注意:\n"
                << "  1) 本程序会真正分配大量内存，100000 Chunk 场景在 64 位进程中通常需要约 1.9 GiB 基础内存。\n"
                << "  2) 第 8 个场景按“10000 个 Chunk 总共随机添加 100000 次”解释；不是每个 Chunk 各加 100000 次。\n";
            std::exit(0);
        } else {
            throw std::runtime_error(std::string("unknown argument: ") + arg);
        }
    }
    return options;
}

static inline BlockSet& voxelAt(Chunk& chunk, const std::uint16_t linearIndex) {
    const std::uint16_t x = static_cast<std::uint16_t>(linearIndex & 15u);
    const std::uint16_t y = static_cast<std::uint16_t>((linearIndex >> 4u) & 15u);
    const std::uint16_t z = static_cast<std::uint16_t>((linearIndex >> 8u) & 15u);
    return chunk.grid[x][y][z];
}

static inline const BlockSet& voxelAt(const Chunk& chunk, const std::uint16_t linearIndex) {
    const std::uint16_t x = static_cast<std::uint16_t>(linearIndex & 15u);
    const std::uint16_t y = static_cast<std::uint16_t>((linearIndex >> 4u) & 15u);
    const std::uint16_t z = static_cast<std::uint16_t>((linearIndex >> 8u) & 15u);
    return chunk.grid[x][y][z];
}

static inline void addToVoxel(Chunk& chunk, const std::uint16_t linearIndex,
                              const std::uint16_t blockId, const FlatFacing facing) {
    voxelAt(chunk, linearIndex).addBifs(blockId, facing, &chunk);
}

static bool storageSlotContainsBlockId(const StorageSlot& slot, const std::uint16_t blockId) {
    if (slot.kind == StorageSlot::Inline) {
        for (std::uint8_t i = 0; i < kStorageSlotInlineAmount; ++i) {
            const BlockIdFacingSet& bifs = slot.value.inlineBifs[i];
            if (bifs.getFacings() != FlatFacing::None && bifs.getBlockId() == blockId) {
                return true;
            }
        }
        return false;
    }

    if (slot.value.fallback == NULL) return false;

    for (std::size_t i = 0; i < slot.value.fallback->size(); ++i) {
        const BlockIdFacingSet& bifs = (*slot.value.fallback)[i];
        if (bifs.getFacings() != FlatFacing::None && bifs.getBlockId() == blockId) {
            return true;
        }
    }
    return false;
}

static bool blockSetContainsBlockId(const BlockSet& blockSet, const Chunk& chunk, const std::uint16_t blockId) {
    if (blockSet.bifs.isBIFS()) {
        return blockSet.bifs.getFacings() != FlatFacing::None && blockSet.bifs.getBlockId() == blockId;
    }

    const std::uint16_t storageIndex = blockSet.bifs.getBlockId();
    if (storageIndex >= chunk.storageSpace.size()) return false;
    return storageSlotContainsBlockId(chunk.storageSpace[storageIndex], blockId);
}

static std::size_t estimateStorageSlotExtraBytes(const StorageSlot& slot) {
    if (slot.kind != StorageSlot::Fallback || slot.value.fallback == NULL) {
        return 0;
    }

    return sizeof(std::vector<BlockIdFacingSet>)
        + slot.value.fallback->capacity() * sizeof(BlockIdFacingSet);
}

static std::size_t estimateChunkBytes(const Chunk& chunk) {
    std::size_t total = sizeof(Chunk);
    total += chunk.storageSpace.capacity() * sizeof(StorageSlot);

    for (std::size_t i = 0; i < chunk.storageSpace.size(); ++i) {
        total += estimateStorageSlotExtraBytes(chunk.storageSpace[i]);
    }
    return total;
}

static std::size_t estimateChunksBytes(const std::vector<Chunk>& chunks) {
    std::size_t total = sizeof(std::vector<Chunk>);
    total += chunks.capacity() * sizeof(Chunk);

    for (std::size_t i = 0; i < chunks.size(); ++i) {
        const Chunk& chunk = chunks[i];
        total += chunk.storageSpace.capacity() * sizeof(StorageSlot);
        for (std::size_t j = 0; j < chunk.storageSpace.size(); ++j) {
            total += estimateStorageSlotExtraBytes(chunk.storageSpace[j]);
        }
    }
    return total;
}

static std::uint64_t chunkChecksum(const Chunk& chunk) {
    std::uint64_t checksum = 1469598103934665603ull;
    for (std::size_t i = 0; i < kChunkVoxelCount; ++i) {
        const BlockSet& blockSet = voxelAt(chunk, static_cast<std::uint16_t>(i));
        checksum ^= static_cast<std::uint64_t>(blockSet.bifs.getBlockId()) + 0x9e3779b97f4a7c15ull;
        checksum *= 1099511628211ull;
        checksum ^= static_cast<std::uint64_t>(blockSet.bifs.getFacings());
        checksum *= 1099511628211ull;
    }
    checksum ^= static_cast<std::uint64_t>(chunk.storageSpace.size());
    checksum *= 1099511628211ull;
    return checksum;
}

static std::uint64_t chunksChecksum(const std::vector<Chunk>& chunks) {
    std::uint64_t checksum = 2166136261u;
    for (std::size_t i = 0; i < chunks.size(); ++i) {
        checksum ^= chunkChecksum(chunks[i]) + static_cast<std::uint64_t>(i);
        checksum *= 16777619u;
    }
    return checksum;
}

static FlatFacing randomFacing(std::mt19937& rng) {
    std::uniform_int_distribution<int> facingDist(0, 7);
    return kFacings[facingDist(rng)];
}

static std::uint16_t randomBlockId0To1000(std::mt19937& rng) {
    std::uniform_int_distribution<int> blockIdDist(0, 1000);
    return static_cast<std::uint16_t>(blockIdDist(rng));
}

static std::uint16_t randomVoxelIndex(std::mt19937& rng) {
    std::uniform_int_distribution<int> voxelDist(0, static_cast<int>(kChunkVoxelCount - 1));
    return static_cast<std::uint16_t>(voxelDist(rng));
}

static void addRandomToSingleChunk(Chunk& chunk, const std::size_t count, std::mt19937& rng) {
    for (std::size_t i = 0; i < count; ++i) {
        addToVoxel(chunk, randomVoxelIndex(rng), randomBlockId0To1000(rng), randomFacing(rng));
    }
}

static void fillChunkAllVoxels(Chunk& chunk, const std::uint16_t blockId, const FlatFacing facing) {
    for (std::uint16_t i = 0; i < static_cast<std::uint16_t>(kChunkVoxelCount); ++i) {
        addToVoxel(chunk, i, blockId, facing);
    }
}

template <typename Fn>
static BenchResult runBenchmark(const std::string& name, const std::string& note,
                                const std::size_t repeats, Fn fn) {
    BenchResult result;
    result.name = name;
    result.note = note;
    result.repeats = repeats;

    if (repeats == 0) {
        result.ok = false;
        result.error = "repeats must be > 0";
        return result;
    }

    double totalMs = 0.0;
    double minMs = std::numeric_limits<double>::infinity();
    double maxMs = 0.0;
    std::uint64_t totalLogicalOps = 0;
    double totalBytes = 0.0;

    try {
        for (std::size_t i = 0; i < repeats; ++i) {
            const std::chrono::steady_clock::time_point t0 = std::chrono::steady_clock::now();
            const SampleResult sample = fn(i);
            const std::chrono::steady_clock::time_point t1 = std::chrono::steady_clock::now();

            const double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
            totalMs += ms;
            if (ms < minMs) minMs = ms;
            if (ms > maxMs) maxMs = ms;
            totalLogicalOps += sample.logicalOps;
            totalBytes += static_cast<double>(sample.estimatedBytes);
            g_sink ^= sample.checksum + 0x9e3779b97f4a7c15ull;
        }
    } catch (const std::bad_alloc&) {
        result.ok = false;
        result.error = "std::bad_alloc: 内存不足，当前场景可能超出机器可用内存。";
        return result;
    } catch (const std::exception& ex) {
        result.ok = false;
        result.error = ex.what();
        return result;
    } catch (...) {
        result.ok = false;
        result.error = "unknown exception";
        return result;
    }

    result.avgMs = totalMs / static_cast<double>(repeats);
    result.avgLogicalOps = totalLogicalOps / repeats;
    result.avgBytes = totalBytes / static_cast<double>(repeats);
    result.minMs = minMs;
    result.maxMs = maxMs;

    if (totalLogicalOps == 0 || totalMs <= 0.0) {
        result.avgNsPerOp = 0.0;
        result.avgOpsPerSec = 0.0;
    } else {
        result.avgNsPerOp = (totalMs * 1000000.0) / static_cast<double>(totalLogicalOps);
        result.avgOpsPerSec = (static_cast<double>(totalLogicalOps) * 1000.0) / totalMs;
    }

    return result;
}

static void printTypeMemorySummary() {
    std::cout << "===== Static footprint (当前 ABI 下的 sizeof) =====\n";
    std::cout << "sizeof(BlockIdFacingSet) = " << sizeof(BlockIdFacingSet) << " B\n";
    std::cout << "sizeof(BlockSet)         = " << sizeof(BlockSet) << " B\n";
    std::cout << "sizeof(StorageSlot)      = " << sizeof(StorageSlot) << " B\n";
    std::cout << "sizeof(Chunk)           = " << sizeof(Chunk) << " B\n";
    std::cout << "Chunk ctor reserve(128) 额外预留 = "
              << (128ull * sizeof(StorageSlot)) << " B (不含分配器额外开销)\n";
    std::cout << "估算单个 Chunk 基础内存约     = "
              << (sizeof(Chunk) + 128ull * sizeof(StorageSlot)) << " B ("
              << formatBytes(static_cast<double>(sizeof(Chunk) + 128ull * sizeof(StorageSlot))) << ")\n\n";
}

static void printScenarioMemoryEstimates() {
    const double perChunk = static_cast<double>(sizeof(Chunk) + 128ull * sizeof(StorageSlot));
    const std::size_t counts[] = {10u, 100u, 1000u, 10000u, 100000u};

    std::cout << "===== Baseline memory estimate for fill-only chunk groups =====\n";
    std::cout << "说明: 这里的估算针对“把每个体素设为 1, North”的场景。\n"
              << "      因为每个体素只有 1 种方块，不会额外分配 StorageSlot，也不会产生 fallback vector。\n";
    for (std::size_t i = 0; i < sizeof(counts) / sizeof(counts[0]); ++i) {
        const double total = perChunk * static_cast<double>(counts[i]);
        std::cout << "  " << std::setw(6) << counts[i] << " chunks -> "
                  << formatBytes(total) << "\n";
    }
    std::cout << "\n";
}

static void printBenchResult(const BenchResult& result) {
    std::cout << "===== " << result.name << " =====\n";
    if (!result.ok) {
        std::cout << "status               : ERROR\n";
        std::cout << "message              : " << result.error << "\n\n";
        return;
    }

    std::cout << "status               : OK\n";
    std::cout << "repeats              : " << result.repeats << "\n";
    std::cout << "avg logical ops      : " << result.avgLogicalOps << "\n";
    std::cout << "avg total time       : " << std::fixed << std::setprecision(3) << result.avgMs << " ms\n";
    std::cout << "min / max            : " << result.minMs << " ms / " << result.maxMs << " ms\n";
    std::cout << "avg ns per op        : " << result.avgNsPerOp << " ns\n";
    std::cout << "avg ops per second   : " << result.avgOpsPerSec << "\n";
    std::cout << "avg estimated memory : " << formatBytes(result.avgBytes)
              << " (仅估算容器与元素内存，不含 allocator metadata)\n";
    if (!result.note.empty()) {
        std::cout << "note                 : " << result.note << "\n";
    }
    std::cout << "\n";
}

static SampleResult benchmarkRandomSingleChunk(const std::size_t addCount, const std::uint32_t seed) {
    Chunk chunk;
    std::mt19937 rng(seed);

    addRandomToSingleChunk(chunk, addCount, rng);

    return SampleResult(
        static_cast<std::uint64_t>(addCount),
        estimateChunkBytes(chunk),
        chunkChecksum(chunk)
    );
}

static SampleResult benchmarkFillChunks(const std::size_t chunkCount) {
    std::vector<Chunk> chunks(chunkCount);
    for (std::size_t i = 0; i < chunkCount; ++i) {
        fillChunkAllVoxels(chunks[i], 1u, FlatFacing::North);
    }

    return SampleResult(
        static_cast<std::uint64_t>(chunkCount) * static_cast<std::uint64_t>(kChunkVoxelCount),
        estimateChunksBytes(chunks),
        chunksChecksum(chunks)
    );
}

static SampleResult benchmarkMassUpdateAfterRandomAdds(const std::size_t chunkCount,
                                                       const std::size_t randomAddCount,
                                                       const std::uint32_t seed) {
    // 这里按“10000 个 Chunk 总共随机添加 100000 次”解释，避免误解成“每个 Chunk 各 100000 次”。
    std::vector<Chunk> chunks(chunkCount);
    std::mt19937 rng(seed);
    std::uniform_int_distribution<std::size_t> chunkDist(0, chunkCount - 1);

    for (std::size_t i = 0; i < randomAddCount; ++i) {
        Chunk& chunk = chunks[chunkDist(rng)];
        addToVoxel(chunk, randomVoxelIndex(rng), randomBlockId0To1000(rng), randomFacing(rng));
    }

    std::uint64_t visitedCells = 0;
    std::uint64_t updatedCells = 0;
    for (std::size_t chunkIndex = 0; chunkIndex < chunks.size(); ++chunkIndex) {
        Chunk& chunk = chunks[chunkIndex];
        for (std::uint16_t linear = 0; linear < static_cast<std::uint16_t>(kChunkVoxelCount); ++linear) {
            ++visitedCells;
            BlockSet& blockSet = voxelAt(chunk, linear);
            if (!blockSetContainsBlockId(blockSet, chunk, 1u)) {
                continue;
            }
            blockSet.addBifs(1001u, FlatFacing::North, &chunk);
            ++updatedCells;
        }
    }

    return SampleResult(
        static_cast<std::uint64_t>(randomAddCount) + visitedCells + updatedCells,
        estimateChunksBytes(chunks),
        chunksChecksum(chunks) ^ (updatedCells << 1u)
    );
}

static void runAll(const Options& options) {
    printTypeMemorySummary();
    printScenarioMemoryEstimates();

    printBenchResult(runBenchmark(
        "1) 1 个 Chunk，随机添加 1000 个 (block, facing)",
        "blockId 随机范围 [0, 1000]。平均速度指标按 1000 次 addBifs 统计。",
        options.repeats,
        [&](const std::size_t repeatIndex) {
            return benchmarkRandomSingleChunk(1000u, options.seed + static_cast<std::uint32_t>(repeatIndex));
        }
    ));

    printBenchResult(runBenchmark(
        "2) 1 个 Chunk，随机添加 10000 个 (block, facing)",
        "blockId 随机范围 [0, 1000]。平均速度指标按 10000 次 addBifs 统计。",
        options.repeats,
        [&](const std::size_t repeatIndex) {
            return benchmarkRandomSingleChunk(10000u, options.seed + 101u + static_cast<std::uint32_t>(repeatIndex));
        }
    ));

    printBenchResult(runBenchmark(
        "3) 1 个 Chunk，随机添加 100000 个 (block, facing)",
        "blockId 随机范围 [0, 1000]。高碰撞概率下，StorageSlot / fallback 开销会显著增大。",
        options.repeats,
        [&](const std::size_t repeatIndex) {
            return benchmarkRandomSingleChunk(100000u, options.seed + 202u + static_cast<std::uint32_t>(repeatIndex));
        }
    ));

    printBenchResult(runBenchmark(
        "4) 10 个 Chunk，所有体素设为 (1, North)",
        "平均速度指标按 10 * 4096 次 addBifs 统计。",
        options.repeats,
        [&](const std::size_t) { return benchmarkFillChunks(10u); }
    ));

    printBenchResult(runBenchmark(
        "5) 100 个 Chunk，所有体素设为 (1, North)",
        "平均速度指标按 100 * 4096 次 addBifs 统计。",
        options.repeats,
        [&](const std::size_t) { return benchmarkFillChunks(100u); }
    ));

    printBenchResult(runBenchmark(
        "6) 1000 个 Chunk，所有体素设为 (1, North)",
        "平均速度指标按 1000 * 4096 次 addBifs 统计。",
        options.repeats,
        [&](const std::size_t) { return benchmarkFillChunks(1000u); }
    ));

    printBenchResult(runBenchmark(
        "7) 10000 个 Chunk，所有体素设为 (1, North)",
        "平均速度指标按 10000 * 4096 次 addBifs 统计。",
        options.repeats,
        [&](const std::size_t) { return benchmarkFillChunks(10000u); }
    ));

    printBenchResult(runBenchmark(
        "8) 100000 个 Chunk，所有体素设为 (1, North)",
        "平均速度指标按 100000 * 4096 次 addBifs 统计；该场景通常需要约 1.9 GiB 基础内存。",
        options.repeats,
        [&](const std::size_t) { return benchmarkFillChunks(100000u); }
    ));

    printBenchResult(runBenchmark(
        "9) 10000 个 Chunk，先随机添加 100000 次，再给所有含 ID=1 的体素追加 (1001, North)",
        "这里把扫描 10000 * 4096 个体素也计入 logical ops；blockId=1001 是定向追加值，不参与随机范围限制。",
        options.repeats,
        [&](const std::size_t repeatIndex) {
            return benchmarkMassUpdateAfterRandomAdds(10000u, 100000u,
                options.seed + 303u + static_cast<std::uint32_t>(repeatIndex));
        }
    ));

    std::cout << "blackhole checksum    : " << g_sink << "\n";
}

} // namespace bench

int main1() {
    try {
        bench::Options options;
        // 如需固定配置，可以直接在这里改：
        options.repeats = 100;
        // options.seed = 20260317u;

        bench::runAll(options);
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "fatal: " << ex.what() << "\n";
        return 1;
    }
}
