# Block-Map-Cpp
A C++ implementation of BlockMap, a 3D voxel grid that allows multiple blocks to exist in the same voxel cell.

## Introduction
BlockMap stores world data in Chunks.
- Each Chunk is a 16 × 16 × 16 voxel grid.
- Each voxel cell contains a BlockSet
- Each BlockSet can store multiple BlockIdFacingSet
- Each BlockIdFacingSet represents one block type with up to 8 facing directions

In most games, most voxel cells contain only one block type.
Some cells contain 2–5 block types, and only a small number contain more.

To keep memory usage efficient:
- BlockSet stays small and compact;
- Extra block data is stored in StorageSpace;
- StorageSpace keeps additional data continuous in memory for better cache locality

## Data structure
```
World
 └── Chunks
      └── Chunk (16×16×16)
           ├── Grid[4096]
           │    └── BlockSet
           │         ├── first block stored inline
           |         └── extra blocks may reference StorageSpace
           │             ├── first 1-6 blocks stored inline
           |             └── extra blocks stores in vector
           └── StorageSpace
                └── extra BlockIdFacingSet data
```

## Detailed relationship diagram
```
+-------------------+
|       World       |
+-------------------+
          |
          v
+-------------------+
|      Chunks       |
|   (many Chunk)    |
+-------------------+
          |
          v
+----------------------------------+
|              Chunk               |
|          16 x 16 x 16            |
|                                  |
|  +----------------------------+  |
|  |        Grid[4096]          |  |
|  |                            |  |
|  |  +----------------------+  |  |
|  |  |       BlockSet       |  |  |
|  |  |----------------------|  |  |
|  |  | inline first block   |  |  |
|  |  | optional extra ref   |--+------+
|  |  +----------------------+  |      |
|  +----------------------------+      |
|                                      |
|  +-------------------------------+   |
|  |         StorageSpace          |<--+
|  | continuous extra block data   |
|  | BlockIdFacingSet[]            |
|  +-------------------------------+
+----------------------------------+

BlockIdFacingSet
  - block id
  - up to 8 facing directions
```
