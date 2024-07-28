#include "kernel/memory/pool.h"
#include "kernel/debug/assert.h"
#include "kernel/descriptor/desc.h"
#include "kernel/descriptor/gdt/idx.h"
#include "kernel/io/video/print.h"
#include "kernel/krnl.h"
#include "kernel/memory/page.h"
#include "kernel/process/proc.h"
#include "kernel/stl/utility.h"
#include "kernel/thread/thd.h"
#include "kernel/util/bit.h"

namespace mem {

namespace {

//! The base address of bitmap memory.
inline constexpr stl::uintptr_t bitmap_base {krnl_base + 0x0009A000};

//! The base address of kernel heap memory.
inline constexpr stl::uintptr_t krnl_heap_base {krnl_base + 0x00100000};

class MemArena;

/**
 * The fix-sized memory block arranged in a memory arena @p MemArena.
 * They are linked together in the free block list of a memory block descriptor @p MemBlockDesc.
 */
class MemBlock {
public:
    MemBlock() noexcept = default;

    MemBlock(const MemBlock&) = delete;

    static MemBlock& GetByTag(const TagList::Tag& tag) noexcept {
        return tag.GetElem<MemBlock>();
    }

    //! Get the arena at the beginning of the memory page.
    MemArena& GetArena() const noexcept {
        return *reinterpret_cast<MemArena*>(VrAddr {this}.GetPageAddr());
    }

    const TagList::Tag& GetTag() const noexcept {
        return tag_;
    }

    TagList::Tag& GetTag() noexcept {
        return const_cast<TagList::Tag&>(const_cast<const MemBlock&>(*this).GetTag());
    }

private:
    //! The tag for the free block list.
    TagList::Tag tag_;
};

/**
 * The momory arena is a memory page containing metadata,
 * and a number of fix-sized memory blocks @p MemBlock, like a storage.
 */
class MemArena {
public:
    MemArena() noexcept = default;

    MemArena(const MemArena&) = delete;

    MemBlock& GetBlock(const stl::size_t idx) noexcept {
        dbg::Assert(!large && desc);
        dbg::Assert(idx < count);
        return *reinterpret_cast<MemBlock*>(reinterpret_cast<stl::byte*>(this) + sizeof(MemArena)
                                            + idx * desc->GetBlockSize());
    }

    /**
     * If the arena is not a large arena, it refers to the block descriptor,
     * otherwise @p nullptr.
     */
    MemBlockDesc* desc {nullptr};

    /**
     * @brief The number of memory pages or @p MemBlock blocks behind the arena.
     *
     * @details
     * - If the arena is a large arena, it refers to the number of memory pages.
     * - Otherwise, it refers to the number of blocks.
     */
    stl::size_t count {0};

    /**
     * @brief Whether the arena is a large arena.
     *
     * @details
     * If the required size is larger than the maximum block size,
     * the arena directly manages memory pages instead of blocks.
     */
    bool large {false};
};

//! Get the user physical memory page pool.
PhyMemPagePool& GetUsrPhyMemPagePool() noexcept {
    static PhyMemPagePool pool;
    return pool;
}

//! Get the kernel physical memory page pool.
PhyMemPagePool& GetKrnlPhyMemPagePool() noexcept {
    static PhyMemPagePool pool;
    return pool;
}

//! Get the kernel virtual address pool.
VrAddrPool& GetKrnlVrAddrPool() noexcept {
    static VrAddrPool pool;
    return pool;
}

//! Get the user virtual address pool from the current process.
VrAddrPool& GetUsrVrAddrPool() noexcept {
    const auto proc {tsk::Thread::GetCurrent().GetProcess()};
    dbg::Assert(proc);
    return proc->GetVrAddrPool();
}

//! Get the kernel memory block descriptor table.
MemBlockDescTab& GetKrnlMemBlockDescTab() noexcept {
    static MemBlockDescTab descs;
    return descs;
}

//! Get the user memory block descriptor table from the current process.
MemBlockDescTab& GetUsrMemBlockDescTab() noexcept {
    const auto proc {tsk::Thread::GetCurrent().GetProcess()};
    dbg::Assert(proc);
    return proc->GetMemBlockDescTab();
}

//! Get the memory block descriptor table.
MemBlockDescTab& GetMemBlockDescTab(const PoolType type) noexcept {
    return type == PoolType::Kernel ? GetKrnlMemBlockDescTab() : GetUsrMemBlockDescTab();
}

//! Whether a physical address belongs to the kernel.
bool IsKrnlMem(const stl::uintptr_t phy_addr) noexcept {
    return phy_addr < GetUsrPhyMemPagePool().GetStartAddr();
}

//! Whether a virtual address belongs to the kernel.
bool IsKrnlMem(const void* const vr_addr) noexcept {
    return IsKrnlMem(VrAddr {vr_addr}.GetPhyAddr());
}

void FreePages(PhyMemPagePool& mem_pool, VrAddrPool& addr_pool, void* const vr_base,
               const stl::size_t count = 1) noexcept {
    dbg::Assert(vr_base && count > 0);
    // Virtual addresses are continous.
    for (stl::size_t i {0}; i != count; ++i) {
        const VrAddr vr_addr {reinterpret_cast<stl::uintptr_t>(vr_base) + i * page_size};
        // Get the mapped physical address.
        const auto phy_addr {vr_addr.GetPhyAddr()};
        dbg::Assert(phy_addr % page_size == 0);
        // Free the mapped physical page.
        mem_pool.FreePages(phy_addr);
        // Clear the page table entry.
        vr_addr.Unmap();
    }

    // Free continous virtual addresses.
    addr_pool.FreePages(reinterpret_cast<stl::uintptr_t>(vr_base), count);
}

void* AllocPages(PhyMemPagePool& mem_pool, VrAddrPool& addr_pool,
                 const stl::size_t count = 1) noexcept {
    // Allocate continous virtual addresses.
    const auto vr_base {addr_pool.AllocPages(count)};
    if (!vr_base) {
        return nullptr;
    }

    for (stl::size_t i {0}; i != count; ++i) {
        // Allocate a physical page.
        if (const auto phy_page {mem_pool.AllocPages()}; phy_page) {
            // Map the virtual address to the physical page.
            const auto vr_addr {vr_base + i * page_size};
            VrAddr {vr_addr}.MapToPhyAddr(phy_page);
        } else {
            // Failed to allocate.
            if (i > 0) {
                // Free allocated physical pages and virtual addresses.
                FreePages(mem_pool, addr_pool, reinterpret_cast<void*>(vr_base), i);
                const auto unmap_vr_base {vr_base + i * page_size};
                addr_pool.FreePages(unmap_vr_base, count - i);
            }

            return nullptr;
        }
    }

    stl::memset(reinterpret_cast<void*>(vr_base), 0, page_size * count);
    return reinterpret_cast<void*>(vr_base);
}

void* AllocPageAtAddr(PhyMemPagePool& mem_pool, VrAddrPool& addr_pool,
                      const stl::uintptr_t vr_addr) noexcept {
    dbg::Assert(!VrAddr {AlignToPageBase(vr_addr)}.IsMapped());
    const auto align_vr_addr {addr_pool.AllocPageAtAddr(vr_addr)};
    if (const auto phy_page {mem_pool.AllocPages()}; phy_page) {
        VrAddr {align_vr_addr}.MapToPhyAddr(phy_page);
        return reinterpret_cast<void*>(align_vr_addr);
    } else {
        return nullptr;
    }
}

//! Get the pool type according to the current thread privilege.
PoolType GetDefaultPoolType() noexcept {
    return tsk::Thread::GetCurrent().IsKrnlThread() ? PoolType::Kernel : PoolType::User;
}

/**
 * @brief A wrapper of a global @p bool variable representing whether memory management has been initialized.
 *
 * @details
 * Global variables cannot be initialized properly.
 * We use @p static variables in methods instead since they can be initialized by methods.
 */
bool& IsMemInitedImpl() noexcept {
    static bool inited {false};
    return inited;
}

}  // namespace

PoolType GetSrcMemPool(const stl::uintptr_t phy_addr) noexcept {
    return IsKrnlMem(phy_addr) ? PoolType::Kernel : PoolType::User;
}

PoolType GetSrcMemPool(const void* const vr_addr) noexcept {
    return GetSrcMemPool(VrAddr {vr_addr}.GetPhyAddr());
}

const MemBlockDesc& MemBlockDescTab::operator[](const stl::size_t idx) const noexcept {
    dbg::Assert(idx < count);
    return descs_[idx];
}

MemBlockDesc* MemBlockDescTab::GetMinDesc(const stl::size_t size) noexcept {
    return const_cast<MemBlockDesc*>(const_cast<const MemBlockDescTab&>(*this).GetMinDesc(size));
}

const MemBlockDesc* MemBlockDescTab::GetMinDesc(const stl::size_t size) const noexcept {
    for (const auto& desc : descs_) {
        if (size <= desc.GetBlockSize()) {
            return &desc;
        }
    }

    return nullptr;
}

MemBlockDesc& MemBlockDescTab::operator[](const stl::size_t idx) noexcept {
    return const_cast<MemBlockDesc&>(const_cast<const MemBlockDescTab&>(*this)[idx]);
}

PhyMemPagePool& GetPhyMemPagePool(const PoolType type) noexcept {
    return type == PoolType::Kernel ? GetKrnlPhyMemPagePool() : GetUsrPhyMemPagePool();
}

VrAddrPool& GetVrAddrPool(const PoolType type) noexcept {
    return type == PoolType::Kernel ? GetKrnlVrAddrPool() : GetUsrVrAddrPool();
}

stl::size_t GetTotalMemSize() noexcept {
    constexpr stl::uintptr_t loader_base {0x900};
    constexpr stl::uintptr_t size_addr {loader_base + gdt::count * sizeof(desc::SegDesc)};
    const auto size {*reinterpret_cast<const stl::size_t*>(size_addr)};
    dbg::Assert(size > 0);
    return size;
}

bool IsMemInited() noexcept {
    return IsMemInitedImpl();
}

void InitMem() noexcept {
    dbg::Assert(!IsMemInited());
    const auto total_mem_size {GetTotalMemSize()};
    dbg::Assert(total_mem_size > 0);

    const auto page_dir_size {page_size};
    const auto krnl_page_tab_size {page_size * krnl_page_dir_count};
    const auto used_mem_size {page_dir_size + krnl_page_tab_size + krnl_size};
    const auto free_mem_size {total_mem_size - used_mem_size};

    const auto free_page_count {free_mem_size / page_size};
    // The kernel uses half of memory and users use the other half.
    const auto krnl_free_page_count {free_page_count / 2};
    const auto usr_free_page_count {free_page_count - krnl_free_page_count};

    const auto krnl_mem_size {krnl_free_page_count * page_size};
    const auto krnl_mem_base {used_mem_size};
    const auto usr_mem_base {krnl_mem_base + krnl_mem_size};

    const auto krnl_bitmap_len {krnl_free_page_count / bit::byte_len};
    const auto usr_bitmap_len {usr_free_page_count / bit::byte_len};

    const auto krnl_bitmap_base {bitmap_base};
    const auto usr_bitmap_base {krnl_bitmap_base + krnl_bitmap_len};

    auto& krnl_mem_pool {GetKrnlPhyMemPagePool()};
    krnl_mem_pool.Init(krnl_mem_base, {reinterpret_cast<void*>(krnl_bitmap_base), krnl_bitmap_len});

    auto& usr_mem_pool {GetUsrPhyMemPagePool()};
    usr_mem_pool.Init(usr_mem_base, {reinterpret_cast<void*>(usr_bitmap_base), usr_bitmap_len});

    auto& krnl_addr_pool {GetKrnlVrAddrPool()};
    krnl_addr_pool.Init(krnl_heap_base, {reinterpret_cast<void*>(usr_bitmap_base + usr_bitmap_len),
                                         krnl_bitmap_len});

    IsMemInitedImpl() = true;
    io::PrintlnStr("Memory pools have been initialized.");
    io::Printf("\tThe memory size is 0x{}.\n", total_mem_size);
    io::Printf("\tThe kernel physical memory addresses start from 0x{}.\n", krnl_mem_base);
    io::Printf("\tThe user physical memory addresses start from 0x{}.\n", usr_mem_base);
}

stl::uintptr_t VrAddrPool::AllocPages(const stl::size_t count) noexcept {
    dbg::Assert(count > 0);
    if (const auto bit_begin {bitmap_.Alloc(count)}; bit_begin != npos) {
        dbg::Assert(free_count_ >= count);
        free_count_ -= count;
        return start_vr_addr_ + bit_begin * page_size;
    } else {
        return 0;
    }
}

stl::uintptr_t VrAddrPool::AllocPageAtAddr(const stl::uintptr_t vr_addr) noexcept {
    const auto align_vr_addr {AlignToPageBase(vr_addr)};
    const auto bit_idx {(align_vr_addr - start_vr_addr_) / page_size};
    bitmap_.ForceAlloc(bit_idx, 1);
    dbg::Assert(free_count_ >= 1);
    free_count_ -= 1;
    return align_vr_addr;
}

stl::size_t VrAddrPool::GetFreeCount() const noexcept {
    return free_count_;
}

stl::uintptr_t VrAddrPool::GetStartAddr() const noexcept {
    return start_vr_addr_;
}

VrAddrPool::VrAddrPool(const stl::uintptr_t start_vr_addr, Bitmap bitmap) noexcept {
    Init(start_vr_addr, stl::move(bitmap));
}

VrAddrPool& VrAddrPool::Init(const stl::uintptr_t start_vr_addr, Bitmap bitmap) noexcept {
    start_vr_addr_ = start_vr_addr;
    bitmap_ = stl::move(bitmap);
    free_count_ = bitmap_.GetCapacity();
    return *this;
}

VrAddrPool& VrAddrPool::FreePages(const stl::uintptr_t vr_base, const stl::size_t count) noexcept {
    dbg::Assert(count > 0);
    dbg::Assert(vr_base >= start_vr_addr_ && vr_base % page_size == 0);
    const auto bit_idx {(vr_base - start_vr_addr_) / page_size};
    bitmap_.Free(bit_idx, count);
    free_count_ += count;
    return *this;
}

const Bitmap& VrAddrPool::GetBitmap() const noexcept {
    return bitmap_;
}

stl::uintptr_t PhyMemPagePool::AllocPages(const stl::size_t count) noexcept {
    dbg::Assert(count > 0);
    if (const auto bit_begin {bitmap_.Alloc(count)}; bit_begin != npos) {
        dbg::Assert(free_count_ >= count);
        free_count_ -= count;
        return start_phy_addr_ + bit_begin * page_size;
    } else {
        return 0;
    }
}

stl::size_t PhyMemPagePool::GetFreeCount() const noexcept {
    return free_count_;
}

stl::uintptr_t PhyMemPagePool::GetStartAddr() const noexcept {
    return start_phy_addr_;
}

PhyMemPagePool& PhyMemPagePool::FreePages(const stl::uintptr_t phy_base,
                                          const stl::size_t count) noexcept {
    dbg::Assert(count > 0);
    dbg::Assert(phy_base >= start_phy_addr_ && phy_base % page_size == 0);
    const auto bit_idx {(phy_base - start_phy_addr_) / page_size};
    bitmap_.Free(bit_idx, count);
    free_count_ += count;
    return *this;
}

PhyMemPagePool& PhyMemPagePool::Init(const stl::uintptr_t start_phy_addr, Bitmap bitmap) noexcept {
    start_phy_addr_ = start_phy_addr;
    bitmap_ = stl::move(bitmap);
    bitmap_.Clear();
    free_count_ = bitmap_.GetCapacity();
    return *this;
}

PhyMemPagePool::PhyMemPagePool(const stl::uintptr_t start_phy_addr, Bitmap bitmap) noexcept {
    Init(start_phy_addr, stl::move(bitmap));
}

stl::mutex& PhyMemPagePool::GetLock() const noexcept {
    return mtx_;
}

MemBlockDesc::MemBlockDesc(const stl::size_t block_size) noexcept {
    Init(block_size);
}

MemBlockDesc& MemBlockDesc::Init(const stl::size_t block_size) noexcept {
    dbg::Assert(block_size > 0);
    block_size_ = block_size;
    block_count_per_arena_ = (page_size - sizeof(MemArena)) / block_size;
    free_blocks_.Init();
    return *this;
}

stl::size_t MemBlockDesc::GetBlockSize() const noexcept {
    return block_size_;
}

stl::size_t MemBlockDesc::GetBlockCountPerArena() const noexcept {
    return block_count_per_arena_;
}

const TagList& MemBlockDesc::GetFreeBlockList() const noexcept {
    return free_blocks_;
}

TagList& MemBlockDesc::GetFreeBlockList() noexcept {
    return const_cast<TagList&>(const_cast<const MemBlockDesc&>(*this).GetFreeBlockList());
}

MemBlockDescTab::MemBlockDescTab() noexcept {
    Init();
}

MemBlockDescTab& MemBlockDescTab::Init() noexcept {
    stl::size_t block_size {min_block_size};
    for (auto& desc : descs_) {
        desc.Init(block_size);
        block_size *= 2;
    }

    dbg::Assert(block_size / 2 == max_block_size);
    return *this;
}

void* AllocPages(const PoolType type, const stl::size_t count) noexcept {
    dbg::Assert(count > 0);
    auto& addr_pool {GetVrAddrPool(type)};
    auto& mem_pool {GetPhyMemPagePool(type)};
    const stl::lock_guard guard {mem_pool.GetLock()};
    return AllocPages(mem_pool, addr_pool, count);
}

void FreePages(void* const vr_base, const stl::size_t count) noexcept {
    dbg::Assert(vr_base && count > 0);
    const auto type {GetSrcMemPool(vr_base)};
    auto& addr_pool {GetVrAddrPool(type)};
    auto& mem_pool {GetPhyMemPagePool(type)};
    const stl::lock_guard guard {mem_pool.GetLock()};
    FreePages(mem_pool, addr_pool, vr_base, count);
}

void* AllocPageAtAddr(const PoolType type, const stl::uintptr_t vr_addr) noexcept {
    auto& mem_pool {GetPhyMemPagePool(type)};
    const stl::lock_guard guard {mem_pool.GetLock()};
    return AllocPageAtAddr(mem_pool, GetVrAddrPool(type), vr_addr);
}

void* AllocPageAtAddr(const PoolType type, VrAddrPool& addr_pool,
                      const stl::uintptr_t vr_addr) noexcept {
    auto& mem_pool {GetPhyMemPagePool(type)};
    const stl::lock_guard guard {mem_pool.GetLock()};
    return AllocPageAtAddr(mem_pool, addr_pool, vr_addr);
}

void Free(const PoolType type, void* const vr_base) noexcept {
    if (!vr_base) {
        return;
    }

    auto& mem_pool {GetPhyMemPagePool(type)};
    const stl::lock_guard guard {mem_pool.GetLock()};
    const auto block {reinterpret_cast<MemBlock*>(vr_base)};

    // Get the arena at the begging of the memory page.
    if (auto& arena {block->GetArena()}; arena.large) {
        // Directly free pages if the arena is a large arena.
        dbg::Assert(!arena.desc);
        FreePages(mem_pool, GetVrAddrPool(type), &arena, arena.count);
    } else {
        // Get the block descriptor.
        const auto desc {arena.desc};
        dbg::Assert(desc);
        // Add the block to the free block list of the descriptor.
        desc->GetFreeBlockList().PushBack(block->GetTag());

        // All blocks in the arena are free.
        if (++arena.count == desc->GetBlockCountPerArena()) {
            // Remove all blocks from the free block list.
            for (stl::size_t i {0}; i != arena.count; ++i) {
                auto& block {arena.GetBlock(i)};
                dbg::Assert(desc->GetFreeBlockList().Find(block.GetTag()));
                block.GetTag().Detach();
            }

            // Free the arena.
            FreePages(mem_pool, GetVrAddrPool(type), &arena);
        }
    }
}

void Free(void* const vr_base) noexcept {
    Free(GetDefaultPoolType(), vr_base);
}

void* Allocate(const PoolType type, const stl::size_t size) noexcept {
    dbg::Assert(size > 0);
    auto& mem_pool {GetPhyMemPagePool(type)};
    const stl::lock_guard lck_guard {mem_pool.GetLock()};
    if (mem_pool.GetFreeCount() * page_size < size) {
        return nullptr;
    }

    auto& addr_pool {GetVrAddrPool(type)};
    if (size > MemBlockDescTab::max_block_size) {
        // Directly allocate a number of pages if the required size is larger than the maximum block size.
        const auto page_count {CalcPageCount(size + sizeof(MemArena))};
        const auto arena {static_cast<MemArena*>(AllocPages(mem_pool, addr_pool, page_count))};
        AssertAlloc(arena);
        arena->desc = nullptr;
        // The arena is a large arena and the count refers to the number of pages instead of blocks.
        arena->large = true;
        arena->count = page_count;
        return reinterpret_cast<stl::byte*>(arena) + sizeof(MemArena);
    } else {
        auto& descs {GetMemBlockDescTab(type)};
        // Get the suitable block descriptor.
        const auto desc {descs.GetMinDesc(size)};
        dbg::Assert(desc);

        if (desc->GetFreeBlockList().IsEmpty()) {
            // Allocate a new arena if the free block list of the descriptor is empty.
            const auto arena {static_cast<MemArena*>(AllocPages(mem_pool, addr_pool))};
            AssertAlloc(arena);
            arena->desc = desc;
            // The arena is not a large arena and the count refers to the number of blocks.
            arena->large = false;
            arena->count = desc->GetBlockCountPerArena();

            // Add all blocks to the free block list.
            const intr::IntrGuard intr_guard;
            for (stl::size_t i {0}; i != arena->count; ++i) {
                auto& block {arena->GetBlock(i)};
                dbg::Assert(!desc->GetFreeBlockList().Find(block.GetTag()));
                desc->GetFreeBlockList().PushBack(block.GetTag());
            }
        }

        dbg::Assert(!desc->GetFreeBlockList().IsEmpty());
        // Remove a block from the free block list and return its address.
        auto& block {MemBlock::GetByTag(desc->GetFreeBlockList().Pop())};
        stl::memset(&block, 0, desc->GetBlockSize());
        auto& arena {block.GetArena()};
        dbg::Assert(arena.count > 0);
        --arena.count;
        return &block;
    }
}

void* Allocate(const stl::size_t size) noexcept {
    return Allocate(GetDefaultPoolType(), size);
}

void AssertAlloc(const void* const addr) noexcept {
    dbg::Assert(addr, "Failed to allocate memory.");
}

void AssertAlloc(const stl::uintptr_t addr) noexcept {
    AssertAlloc(reinterpret_cast<const void*>(addr));
}

}  // namespace mem