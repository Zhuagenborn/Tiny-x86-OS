/**
 * Memory management.
 */

#pragma once

#include "kernel/stl/array.h"
#include "kernel/stl/mutex.h"
#include "kernel/util/bitmap.h"
#include "kernel/util/metric.h"
#include "kernel/util/tag_list.h"

namespace mem {

enum class PoolType { Kernel, User };

/**
 * @brief The virtual address pool that allocates virtual addresses in pages.
 *
 * @warning
 * This class only manages virtual addresses. They cannot be accessed directly after allocation.
 * Each address should be mapped to a physical page allocated by @p PhyMemPagePool using @p VrAddr::MapToPhyAddr.
 */
class VrAddrPool {
public:
    VrAddrPool() noexcept = default;

    /**
     * @brief Create a virtual address pool.
     *
     * @param start_vr_addr The virtual start address.
     * @param bitmap A bitmap for address management.
     */
    VrAddrPool(stl::uintptr_t start_vr_addr, Bitmap bitmap) noexcept;

    VrAddrPool(const VrAddrPool&) = delete;

    VrAddrPool& Init(stl::uintptr_t start_vr_addr, Bitmap bitmap) noexcept;

    /**
     * @brief Allocate a number of continuous virtual page addresses.
     *
     * @param count The number of pages.
     * @return The virtual start address.
     */
    stl::uintptr_t AllocPages(stl::size_t count = 1) noexcept;

    /**
     * @brief Allocate a virtual page address at a specific virtual address.
     *
     * @param vr_addr A virtual address. It will be aligned to a page base.
     * @return The virtual address.
     */
    stl::uintptr_t AllocPageAtAddr(stl::uintptr_t vr_addr) noexcept;

    VrAddrPool& FreePages(stl::uintptr_t vr_base, stl::size_t count = 1) noexcept;

    stl::size_t GetFreeCount() const noexcept;

    stl::uintptr_t GetStartAddr() const noexcept;

    const Bitmap& GetBitmap() const noexcept;

private:
    stl::uintptr_t start_vr_addr_ {0};
    stl::size_t free_count_ {0};
    Bitmap bitmap_;
};

/**
 * @brief The physical memory pool that allocates physical memory in pages.
 *
 * @warning
 * This class only manages physical pages. They cannot be accessed directly after allocation.
 * Each page should be associated with a virtual address allocated by @p VrAddrPool using @p VrAddr::MapToPhyAddr.
 */
class PhyMemPagePool {
public:
    PhyMemPagePool() noexcept = default;

    /**
     * @brief Create a physical memory pool.
     *
     * @param start_vr_addr The physical start address.
     * @param bitmap A bitmap for pages management.
     */
    PhyMemPagePool(stl::uintptr_t start_phy_addr, Bitmap bitmap) noexcept;

    PhyMemPagePool(const PhyMemPagePool&) = delete;

    PhyMemPagePool& Init(stl::uintptr_t start_phy_addr, Bitmap bitmap) noexcept;

    /**
     * @brief Allocate a number of continuous physical pages.
     *
     * @param count The number of pages.
     * @return The physical start address.
     */
    stl::uintptr_t AllocPages(stl::size_t count = 1) noexcept;

    stl::size_t GetFreeCount() const noexcept;

    PhyMemPagePool& FreePages(stl::uintptr_t phy_base, stl::size_t count = 1) noexcept;

    /**
     * Get a lock.
     * Before allocation or release, it must be locked.
     */
    stl::mutex& GetLock() const noexcept;

    stl::uintptr_t GetStartAddr() const noexcept;

private:
    mutable stl::mutex mtx_;
    stl::uintptr_t start_phy_addr_ {0};
    stl::size_t free_count_ {0};
    Bitmap bitmap_;
};

/**
 * @brief The memory block descriptor.
 *
 * @details
 * A memory block descriptor manages fix-sized block @p MemBlock arranged in memory arenas @p MemArena.
 * A user process has multiple memory block descriptors for heap memory allocations of different sizes.
 * ```
 *           Arena                     Arena
 *       ┌────────────┐            ┌────────────┐
 *       │ Descriptor │ ──────┬─── │ Descriptor │
 *       ├────────────┤       │    ├────────────┤
 *       │ Free Count │       │    │ Free Count │
 *       ├────────────┤       │    ├────────────┤
 *       │   Blocks   │       │    │   Blocks   │
 *       ├────────────┤       │    ├────────────┤
 * ┌───► │    ....    │       │    │    ....    │ ◄──┐
 * │     ├────────────┤       │    ├────────────┤    │
 * │     │   Blocks   │       │    │   Blocks   │    │
 * │     └────────────┘       │    └────────────┘    │
 * │                          │                      │
 * │     Block Descriptor ◄───┘                      │
 * │     ┌─────────────┐                             │
 * └──── │ Free Blocks │ ────────────────────────────┘
 *       ├─────────────┤
 *       │    Size     │
 *       └─────────────┘
 * ```
 */
class MemBlockDesc {
public:
    MemBlockDesc() noexcept = default;

    explicit MemBlockDesc(stl::size_t block_size) noexcept;

    MemBlockDesc(const MemBlockDesc&) = delete;

    MemBlockDesc& Init(stl::size_t block_size) noexcept;

    stl::size_t GetBlockSize() const noexcept;

    stl::size_t GetBlockCountPerArena() const noexcept;

    const TagList& GetFreeBlockList() const noexcept;

    TagList& GetFreeBlockList() noexcept;

private:
    //! The block size managed by the memory block descriptor.
    stl::size_t block_size_ {0};

    //! The maximum number of blocks in an arena.
    stl::size_t block_count_per_arena_ {0};

    //! The free blocks in the current arena.
    TagList free_blocks_;
};

/**
 * @brief The memory block descriptor table.
 *
 * @details
 * The memory block descriptor table contains memory block descriptors for different block sizes:
 * - 16 bytes.
 * - 32 bytes.
 * - 64 bytes.
 * - 128 bytes.
 * - 256 bytes.
 * - 512 bytes.
 * - 1024 bytes.
 * A user process has a memory block descriptor table for heap memory management.
 */
class MemBlockDescTab {
public:
    static constexpr stl::size_t min_block_size {16};
    static constexpr stl::size_t max_block_size {1024};

    MemBlockDescTab() noexcept;

    MemBlockDescTab& Init() noexcept;

    const MemBlockDesc& operator[](stl::size_t) const noexcept;

    MemBlockDesc& operator[](stl::size_t) noexcept;

    MemBlockDesc* GetMinDesc(stl::size_t) noexcept;

    //! Get the smallest descriptor that satisfies the required size.
    const MemBlockDesc* GetMinDesc(stl::size_t) const noexcept;

    constexpr stl::size_t size() const noexcept {
        return count;
    }

    constexpr MemBlockDesc* begin() noexcept {
        return const_cast<MemBlockDesc*>(const_cast<const MemBlockDescTab&>(*this).begin());
    }

    constexpr const MemBlockDesc* begin() const noexcept {
        return descs_.begin();
    }

    constexpr MemBlockDesc* end() noexcept {
        return const_cast<MemBlockDesc*>(const_cast<const MemBlockDescTab&>(*this).end());
    }

    constexpr const MemBlockDesc* end() const noexcept {
        return descs_.end();
    }

    constexpr const MemBlockDesc* cbegin() const noexcept {
        return begin();
    }

    constexpr const MemBlockDesc* cend() const noexcept {
        return end();
    }

private:
    static constexpr stl::size_t count {7};

    stl::array<MemBlockDesc, count> descs_;
};

//! Initialize memory management.
void InitMem() noexcept;

//! Whether memory management has been initialized.
bool IsMemInited() noexcept;

//! Get the total memory size in bytes.
stl::size_t GetTotalMemSize() noexcept;

//! Get the virtual address pool.
VrAddrPool& GetVrAddrPool(PoolType) noexcept;

//! Get the physical memory page pool.
PhyMemPagePool& GetPhyMemPagePool(PoolType) noexcept;

//! Get the memory pool type by a physical address.
PoolType GetSrcMemPool(stl::uintptr_t phy_addr) noexcept;

//! Get the memory pool type by a virtual address.
PoolType GetSrcMemPool(const void* vr_addr) noexcept;

/**
 * @brief Allocate a number of virtual pages from a memory pool.
 *
 * @details
 * This method combines @p PhyMemPagePool::AllocPages and @p VrAddr::MapToPhyAddr.
 * The allocated physical pages are associated with virtual addresses.
 */
void* AllocPages(PoolType, stl::size_t count = 1) noexcept;

/**
 * @brief Allocate a virtual page from a memory pool at a specific virtual address.
 *
 * @details
 * This method combines @p PhyMemPagePool::AllocPageAtAddr and @p VrAddr::MapToPhyAddr.
 * The allocated physical page is associated with a virtual address.
 */
void* AllocPageAtAddr(PoolType, stl::uintptr_t vr_addr) noexcept;

void* AllocPageAtAddr(PoolType, VrAddrPool& addr_pool, stl::uintptr_t vr_addr) noexcept;

/**
 * @brief Allocate virtual memory in bytes.
 *
 * @details
 * - If the required size is larger than the maximum block size:
 *     1. Allocate memory pages.
 *     2. Initialize the arena @p MemArena at the begging of the first page.
 *     3. Return the address of the available memory behind the arena.
 * - Otherwise:
 *     1. Find the suitable block descriptor @p MemBlockDesc.
 *     2. If the free block list of the descriptor is empty:
 *         1. Allocate and initialize an arena @p MemArena.
 *         2. Add all blocks in the arena to the free block list.
 *     3. Remove a block @p MemBlock from the free block list and return its address.
 */
void* Allocate(stl::size_t size) noexcept;

//! Allocate virtual memory from a memory pool in bytes.
void* Allocate(PoolType, stl::size_t size) noexcept;

template <typename T>
T* AllocPages(const PoolType type, const stl::size_t count = 1) noexcept {
    return static_cast<T*>(AllocPages(type, count));
}

template <typename T>
T* AllocPageAtAddr(const PoolType type, const stl::uintptr_t vr_addr) noexcept {
    return static_cast<T*>(AllocPageAtAddr(type, vr_addr));
}

template <typename T>
T* Allocate(const stl::size_t size) noexcept {
    return static_cast<T*>(Allocate(size));
}

template <typename T>
T* Allocate(const PoolType type, const stl::size_t size) noexcept {
    return static_cast<T*>(Allocate(type, size));
}

/**
 * @brief Free virtual pages.
 *
 * @details
 * This method combines @p PhyMemPagePool::FreePages and @p VrAddr::Unmap.
 */
void FreePages(void* vr_base, stl::size_t count = 1) noexcept;

/**
 * @brief Free virtual memory.
 *
 * @details
 * 1. Get the arena @p MemArena at the begging of the memory page.
 * - If the arena is a large arena:
 *     2. Directly free pages.
 * - Otherwise:
 *     2. Get the block descriptor @p MemBlockDesc from the arena.
 *     3. Add the block @p MemBlock to the free block list of the descriptor.
 *     4. If all blocks in the arena are free, remove them from the free block list and free the arena.
 */
void Free(void* vr_base) noexcept;

//! Free virtual memory from a memory pool.
void Free(PoolType, void* vr_base) noexcept;

//! Assert that an allocated address is not @p nullptr.
void AssertAlloc(const void*) noexcept;

void AssertAlloc(stl::uintptr_t) noexcept;

}  // namespace mem