#include "kernel/io/disk/disk.h"
#include "kernel/io/disk/ide.h"
#include "kernel/io/io.h"
#include "kernel/io/video/console.h"
#include "kernel/io/video/print.h"
#include "kernel/memory/pool.h"
#include "kernel/util/format.h"

namespace io {

namespace {

//! Partition types.
enum class PartType : stl::uint8_t {
    //! Empty partitions.
    Empty = 0,
    //! Extended partitions or logical disks.
    ExtPart = 5
};

//! The maximum number of sectors that can be manipulated per disk access.
inline constexpr stl::size_t max_sector_count_per_access {256};

/**
 * @brief Clear the current disk interrupt.
 *
 * @details
 * After an interrupt is processed, we need to notify the disk,
 * otherwise it will not generate a new interrupt.
 */
void ClearCurrIntr(const IdeChnl& chnl) noexcept {
    ReadByteFromPort(chnl.GetStatusPort());
}

/**
 * @brief Adjusts the value of the number of sectors according to register restrictions.
 *
 * @details
 * The maximum number of sectors that can be manipulated per disk access is 256.
 * The I/O register is 2 bytes long, having values from 0 to 255.
 * So if we want to read or write 256 sectors once, we should set the register to 0.
 */
stl::uint8_t AdjustSectorCount(const stl::size_t count) noexcept {
    dbg::Assert(0 < count && count <= max_sector_count_per_access);
    return count >= max_sector_count_per_access ? 0 : count;
}

//! The disk interrupt handler.
void DiskIntrHandler(const stl::size_t intr_num) noexcept {
    const auto intr {static_cast<intr::Intr>(intr_num)};
    dbg::Assert(intr == intr::Intr::PrimaryIdeChnl || intr == intr::Intr::SecondaryIdeChnl);
    const auto chnl_idx {intr == intr::Intr::PrimaryIdeChnl ? 0 : 1};
    auto& chnl {GetIdeChnls()[chnl_idx]};
    dbg::Assert(chnl.GetIntrNum() == intr_num);
    if (chnl.IsWaitingForIntr()) {
        chnl.NeedToWaitForIntr(false);
        // When manipulating a disk, we locked it.
        // So when an interrupt is triggered, it can only be caused by the last operation.
        chnl.Unblock();
        ClearCurrIntr(chnl);
    }
}

#pragma pack(push, 1)

//! The partition table entry.
struct PartTabEntry {
    //! Whether the partition is the system boot partition.
    bool is_bootable;
    stl::uint8_t start_head;
    stl::uint8_t start_sector;
    stl::uint8_t start_cylinder;
    PartType type;
    stl::uint8_t end_head;
    stl::uint8_t end_sector;
    stl::uint8_t end_cylinder;

    /**
     * @brief The relative start LBA.
     *
     * @details
     * This value is a relative offset based on its parent object.
     * - For a logical partition, it is a part of a logical disk. So its base address is the start LBA of the logical disk.
     * - For a logical disk, it is a part of an extended partition. So its base address is the start LBA of the extended partition.
     * - For a primary partition or an extended partition, the base address is @p 0.
     */
    stl::uint32_t start_lba;

    stl::uint32_t sector_count;
};

static_assert(sizeof(PartTabEntry) == 16);

/**
 * @brief The partition table.
 *
 * @details
 * A partition table is 64 bytes with four partition entries.
 * It is saved in the boot record at offset @p 446 bytes.
 * Up to one of the four partitions can be used as an extended partition and the others as the primary partition.
 */
using PartTab = stl::array<PartTabEntry, Disk::prim_part_count>;

static_assert(sizeof(PartTab) == 64);

/**
 * @brief The boot record.
 *
 * @details
 * A boot record is 512 bytes and ends with @p 0x55, @p 0xAA.
 * It contains a partition table at offset @p 446 bytes.
 */
struct BootRecord {
    static constexpr stl::uint16_t end_sig {0xAA55};
    static constexpr stl::size_t data_size {
        Disk::sector_size - Disk::prim_part_count * sizeof(PartTabEntry) - sizeof(stl::uint16_t)};

    stl::array<stl::byte, data_size> data;
    PartTab parts;
    stl::uint16_t sig;
};

static_assert(sizeof(BootRecord) == Disk::sector_size);

#pragma pack(pop)

//! The 8-bit I/O register.
class Register {
public:
    constexpr Register(const stl::uint8_t val = 0) noexcept : val_ {val} {}

    constexpr operator stl::uint8_t() const noexcept {
        return val_;
    }

protected:
    stl::uint8_t val_;
};

static_assert(sizeof(Register) == sizeof(stl::uint8_t));

//! The device register.
class DeviceReg : public Register {
public:
    constexpr DeviceReg(const stl::uint8_t val = 0) noexcept : Register {val} {
        SetMbs();
    }

    constexpr DeviceReg(const bool master, const stl::size_t lba,
                        const bool lba_mod = true) noexcept :
        DeviceReg {Format(master, lba, lba_mod)} {}

    constexpr bool IsLbaMode() const noexcept {
        return bit::IsBitSet(val_, mod_pos);
    }

    constexpr DeviceReg& SetLbaMode(const bool lba) noexcept {
        if (lba) {
            bit::SetBit(val_, mod_pos);
        } else {
            bit::ResetBit(val_, mod_pos);
        }

        return *this;
    }

    constexpr bool IsMaster() const noexcept {
        return !bit::IsBitSet(val_, dev_pos);
    }

    constexpr DeviceReg& SetMaster(const bool master) noexcept {
        if (master) {
            bit::ResetBit(val_, dev_pos);
        } else {
            bit::SetBit(val_, dev_pos);
        }

        return *this;
    }

    constexpr DeviceReg& SetLba(const stl::size_t lba) noexcept {
        const auto mid {bit::GetBits(lba, 24, 27)};
        bit::SetBits(val_, mid, lba_24_27_pos, lba_24_27_len);
        return *this;
    }

private:
    static constexpr stl::size_t lba_24_27_pos {0};
    static constexpr stl::size_t lba_24_27_len {4};
    static constexpr stl::size_t dev_pos {lba_24_27_pos + lba_24_27_len};
    static constexpr stl::size_t mod_pos {6};

    static constexpr stl::uint8_t Format(const bool master, const stl::size_t lba,
                                         const bool lba_mod) noexcept {
        return DeviceReg {}.SetMaster(master).SetLbaMode(lba_mod).SetLba(lba);
    }

    constexpr DeviceReg& SetMbs() noexcept {
        bit::SetBit(val_, 5);
        bit::SetBit(val_, 7);
        return *this;
    }
};

//! The status register.
class StatusReg : public Register {
public:
    using Register::Register;

    constexpr bool HasError() const noexcept {
        return bit::IsBitSet(val_, err_pos);
    }

    constexpr bool IsDataPrepared() const noexcept {
        return bit::IsBitSet(val_, req_pos);
    }

    constexpr bool IsDeviceReady() const noexcept {
        return bit::IsBitSet(val_, ready_pos);
    }

    constexpr bool IsDeviceBusy() const noexcept {
        return bit::IsBitSet(val_, busy_pos);
    }

private:
    static constexpr stl::size_t err_pos {0};
    static constexpr stl::size_t req_pos {3};
    static constexpr stl::size_t ready_pos {6};
    static constexpr stl::size_t busy_pos {7};
};

//! The LBA byte register.
template <stl::size_t start_bit>
class LbaReg : public Register {
public:
    constexpr LbaReg& SetLba(const stl::size_t lba) noexcept {
        val_ = bit::GetByte(lba, start_bit);
        return *this;
    }

protected:
    using Register::Register;

    ~LbaReg() noexcept = default;
};

//! The LBA bits `0`-`7` register.
class LbaLowReg : public LbaReg<0> {
public:
    using LbaReg::LbaReg;
};

//! The LBA bits `8`-`15` register.
class LbaMidReg : public LbaReg<sizeof(stl::uint8_t) * bit::byte_len> {
public:
    using LbaReg::LbaReg;
};

//! The LBA bits `16`-`23` register.
class LbaHighReg : public LbaReg<sizeof(stl::uint16_t) * bit::byte_len> {
public:
    using LbaReg::LbaReg;
};

//! Swap bytes in a number of byte pairs.
void SwapBytePairs(const void* const src, void* const dest, const stl::size_t count) noexcept {
    dbg::Assert(src && dest && src != dest);
    for (stl::size_t i {0}; i != count; ++i) {
        const auto base {i * 2};
        static_cast<stl::byte*>(dest)[base] = static_cast<const stl::byte*>(src)[base + 1];
        static_cast<stl::byte*>(dest)[base + 1] = static_cast<const stl::byte*>(src)[base];
    }
}

void PrintDiskInfo(Disk& disk) noexcept {
    const auto info {disk.GetInfo()};
    io::Printf("\t\t\tSerial Number: {}\n", info.GetSerial());
    io::Printf("\t\t\tModel: {}\n", info.GetModel());
    io::Printf("\t\t\tSectors: {}\n", info.GetSectorCount());
    io::Printf("\t\t\tCapacity: {} MB\n", info.GetSectorCount() * Disk::sector_size / MB(1));

    const auto& prim_parts {disk.GetPrimaryParts()};
    for (stl::size_t i {0}; i != prim_parts.size(); ++i) {
        const auto& part {prim_parts[i]};
        if (part.IsValid()) {
            io::Printf("\t\t\tPrimary Part {}\n", part.GetName());
            io::Printf("\t\t\t\tStart Sector: {}\n", part.GetStartLba());
            io::Printf("\t\t\t\tSectors: {}\n", part.GetSectorCount());
        }
    }

    const auto& logic_parts {disk.GetLogicParts()};
    for (stl::size_t i {0}; i != logic_parts.size(); ++i) {
        const auto& part {logic_parts[i]};
        if (part.IsValid()) {
            io::Printf("\t\t\tLogic Part {}\n", part.GetName());
            io::Printf("\t\t\t\tStart Sector: {}\n", part.GetStartLba());
            io::Printf("\t\t\t\tSectors: {}\n", part.GetSectorCount());
        }
    }
}

/**
 * @brief A wrapper of a global @p bool variable representing whether disks have been initialized.
 *
 * @details
 * Global variables cannot be initialized properly.
 * We use @p static variables in methods instead since they can be initialized by methods.
 */
bool& IsDiskInitedImpl() noexcept {
    static bool inited {false};
    return inited;
}

}  // namespace

TagList& GetDiskParts() noexcept {
    static TagList parts;
    return parts;
}

stl::size_t GetDiskCount() noexcept {
    constexpr stl::uintptr_t count_addr {0x475};
    const auto count {*reinterpret_cast<const stl::uint8_t*>(count_addr)};
    dbg::Assert(count > 0);
    return count;
}

const Disk::PrimaryParts& Disk::GetPrimaryParts() const noexcept {
    return prim_parts_;
}

Disk& Disk::SetName(const stl::string_view name) noexcept {
    if (name.empty()) {
        stl::memset(name_.data(), '\0', name_.size());
    } else {
        stl::strcpy_s(name_.data(), name_.size(), name.data());
    }

    return *this;
}

const Disk::LogicParts& Disk::GetLogicParts() const noexcept {
    return logic_parts_;
}

const Disk::FilePart& Disk::GetPrimaryPart(const stl::size_t idx) const noexcept {
    dbg::Assert(idx < prim_part_count);
    return prim_parts_[idx];
}

Disk::PrimaryParts& Disk::GetPrimaryParts() noexcept {
    return const_cast<PrimaryParts&>(const_cast<const Disk&>(*this).GetPrimaryParts());
}

Disk::LogicParts& Disk::GetLogicParts() noexcept {
    return const_cast<LogicParts&>(const_cast<const Disk&>(*this).GetLogicParts());
}

Disk::FilePart& Disk::GetPrimaryPart(const stl::size_t idx) noexcept {
    return const_cast<FilePart&>(const_cast<const Disk&>(*this).GetPrimaryPart(idx));
}

Disk::FilePart& Disk::GetLogicPart(const stl::size_t idx) noexcept {
    return const_cast<FilePart&>(const_cast<const Disk&>(*this).GetLogicPart(idx));
}

const Disk::FilePart& Disk::GetLogicPart(const stl::size_t idx) const noexcept {
    dbg::Assert(idx < max_logic_part_count);
    return logic_parts_[idx];
}

const Disk& Disk::ReadSectors(const stl::size_t start_lba, void* const buf,
                              const stl::size_t count) const noexcept {
    dbg::Assert(buf && count > 0);
    dbg::Assert(start_lba + count <= max_lba);
    auto& chnl {GetIdeChnl()};
    const stl::lock_guard guard {chnl.GetLock()};

    Select();
    stl::size_t read_count {0};
    while (read_count < count) {
        const auto curr_count {stl::min(count - read_count, max_sector_count_per_access)};
        SetSectors(start_lba + read_count, curr_count);
        SendCmd(Cmd::Read);
        // Block the IDE channel when the disk is reading.
        // A disk can be blocked only after it receives a command and starts working.
        // When it has finished processing the command, it will wake itself up in the interrupt handler `DiskIntrHandler`.
        chnl.Block();
        // Check whether the disk is readable.
        if (!BusyWait()) {
            io::Console::Printf("Failed to read the disk '{}', LBA {}.\n", name_.data(),
                                start_lba + read_count);
            dbg::Assert(false);
        }

        ReadWords(reinterpret_cast<stl::byte*>(buf) + read_count * sector_size,
                  curr_count * sector_size / sizeof(stl::uint16_t));
        read_count += curr_count;
    }

    return *this;
}

const Disk& Disk::ReadWords(void* const buf, const stl::size_t count) const noexcept {
    dbg::Assert(buf && count > 0);
    ReadWordsFromPort(GetIdeChnl().GetDataPort(), buf, count);
    return *this;
}

Disk& Disk::Attach(IdeChnl* const ide_chnl, const stl::size_t idx) noexcept {
    ide_chnl_ = ide_chnl;
    dbg::Assert(idx < IdeChnl::max_disk_count);
    idx_ = idx;
    return *this;
}

bool Disk::BusyWait() const noexcept {
    constexpr stl::size_t max_wait_time {SecondsToMilliseconds(30)};
    constexpr stl::size_t sleep_time {10};
    const auto status_port {GetIdeChnl().GetStatusPort()};
    stl::size_t wait_time {0};
    while (wait_time < max_wait_time) {
        if (StatusReg {ReadByteFromPort(status_port)}.IsDeviceBusy()) {
            // Continue sleeping if the disk is still busy.
            tsk::Thread::GetCurrent().Sleep(sleep_time);
            wait_time += sleep_time;
        } else {
            return StatusReg {ReadByteFromPort(status_port)}.IsDataPrepared();
        }
    }

    return false;
}

Disk& Disk::WriteSectors(const stl::size_t start_lba, const void* const data,
                         const stl::size_t count) noexcept {
    dbg::Assert(data && count > 0);
    dbg::Assert(start_lba + count <= max_lba);

    auto& chnl {GetIdeChnl()};
    const stl::lock_guard guard {chnl.GetLock()};

    Select();
    stl::size_t written_count {0};
    while (written_count < count) {
        const auto curr_count {stl::min(count - written_count, max_sector_count_per_access)};
        SetSectors(start_lba + written_count, curr_count);
        SendCmd(Cmd::Write);
        // Check whether the disk is writable.
        if (!BusyWait()) {
            io::Console::Printf("Failed to write data to the disk '{}', LBA {}.\n", name_.data(),
                                start_lba + written_count);
            dbg::Assert(false);
        }

        WriteWords(reinterpret_cast<const stl::byte*>(data) + written_count * sector_size,
                   curr_count * sector_size / sizeof(stl::uint16_t));
        // Block the IDE channel when the disk is writing.
        chnl.Block();
        written_count += curr_count;
    }

    return *this;
}

Disk& Disk::WriteWords(const void* const data, const stl::size_t count) noexcept {
    dbg::Assert(data && count > 0);
    WriteWordsToPort(GetIdeChnl().GetDataPort(), data, count);
    return *this;
}

const Disk& Disk::Select() const noexcept {
    const bool is_master {idx_ == 0};
    WriteByteToPort(GetIdeChnl().GetDevicePort(), DeviceReg {}.SetMaster(is_master));
    return *this;
}

const Disk& Disk::SetSectors(const stl::size_t start_lba, const stl::size_t count) const noexcept {
    dbg::Assert(0 < count && count <= max_sector_count_per_access);
    dbg::Assert(start_lba + count <= max_lba);
    auto& chnl {GetIdeChnl()};
    WriteByteToPort(chnl.GetSectorCountPort(), AdjustSectorCount(count));
    WriteByteToPort(chnl.GetLbaLowPort(), LbaLowReg {}.SetLba(start_lba));
    WriteByteToPort(chnl.GetLbaMidPort(), LbaMidReg {}.SetLba(start_lba));
    WriteByteToPort(chnl.GetLbaHighPort(), LbaHighReg {}.SetLba(start_lba));
    const bool is_master {idx_ == 0};
    WriteByteToPort(chnl.GetDevicePort(), DeviceReg {is_master, start_lba});
    return *this;
}

Disk::Info::Info(const stl::byte* const buf) noexcept {
    dbg::Assert(buf);
    constexpr stl::size_t serial_pos {10 * sizeof(stl::uint16_t)};
    constexpr stl::size_t model_pos {27 * sizeof(stl::uint16_t)};
    constexpr stl::size_t sector_count_pos {60 * sizeof(stl::uint16_t)};
    // The information data is in words, where the position of every two neighboring characters is reversed.
    // So we need to swap every two of them.
    SwapBytePairs(buf + serial_pos, serial_.data(), serial_len / 2);
    SwapBytePairs(buf + model_pos, model_.data(), model_len / 2);
    sector_count_ = *reinterpret_cast<const stl::size_t*>(buf + sector_count_pos);
}

stl::string_view Disk::Info::GetSerial() const noexcept {
    return serial_.data();
}

stl::string_view Disk::Info::GetModel() const noexcept {
    return model_.data();
}

stl::size_t Disk::Info::GetSectorCount() const noexcept {
    return sector_count_;
}

Disk::Info Disk::GetInfo() const noexcept {
    Select();
    SendCmd(Cmd::Identify);
    GetIdeChnl().Block();
    if (!BusyWait()) {
        io::Printf("Failed to identify the disk '{}'.\n", name_.data());
        dbg::Assert(false);
    }

    stl::byte buf[sector_size] {};
    ReadWords(buf, sizeof(buf) / sizeof(stl::uint16_t));
    return Info {buf};
}

const Disk& Disk::SendCmd(const Cmd cmd) const noexcept {
    auto& chnl {GetIdeChnl()};
    chnl.NeedToWaitForIntr();
    WriteByteToPort(chnl.GetCmdPort(), static_cast<stl::byte>(cmd));
    return *this;
}

IdeChnl& Disk::GetIdeChnl() noexcept {
    return const_cast<IdeChnl&>(const_cast<const Disk&>(*this).GetIdeChnl());
}

const IdeChnl& Disk::GetIdeChnl() const noexcept {
    dbg::Assert(ide_chnl_);
    return *ide_chnl_;
}

stl::string_view Disk::GetName() const noexcept {
    return name_.data();
}

Disk& Disk::ScanParts() noexcept {
    return ScanParts(0, true);
}

Disk& Disk::ScanParts(const stl::size_t lba, const bool new_disk) noexcept {
    // The start LBA of the extended partition.
    static stl::size_t ext_lba_base {0};
    // The indexes of the current primary and logical partitions.
    static stl::size_t prim_idx {0}, logic_idx {0};
    if (new_disk) {
        // Reset the LBA of the extended partition and partition indexes when starting to scan a new disk.
        ext_lba_base = 0;
        prim_idx = 0;
        logic_idx = 0;
    }

    dbg::Assert(prim_idx < prim_part_count);
    if (logic_idx >= max_logic_part_count) {
        return *this;
    }

    // Whether we are scanning an extended partition.
    const auto IsInExtPart {[]() {
        return ext_lba_base != 0;
    }};

    const auto boot_record {mem::Allocate<BootRecord>(sizeof(BootRecord))};
    mem::AssertAlloc(boot_record);
    ReadSectors(lba, boot_record, sizeof(BootRecord) / sector_size);
    dbg::Assert(boot_record->sig == BootRecord::end_sig);

    for (auto entry {boot_record->parts.cbegin()}; entry != boot_record->parts.cend(); ++entry) {
        if (entry->type == PartType::ExtPart) {
            // Extended partitions or logical disks.
            if (IsInExtPart()) {
                // Logical disks use the LBA of the extended partition as their base address.
                ScanParts(ext_lba_base + entry->start_lba);
            } else {
                // When we scanning the master boot record, `ext_lba_base` is `0`.
                // We should save the LBA of the extended partition.
                // Logical disks inside it should have this as their base address.
                ext_lba_base = entry->start_lba;
                ScanParts(entry->start_lba);
            }
        } else if (entry->type != PartType::Empty) {
            // Primary partitions or logical partitions.
            if (IsInExtPart()) {
                // Logical partitions.
                auto& logic {logic_parts_[logic_idx]};
                // `lba` should be used as the base address since logical partitions can be located in a logical disk.
                logic.start_lba_ = lba + entry->start_lba;
                logic.sector_count_ = entry->sector_count;
                logic.disk_ = this;
                stl::array<char, 8> part_name;
                FormatStringBuffer(part_name.data(), "{}{}", name_.data(),
                                   logic_idx + 1 + prim_part_count);
                logic.SetName(part_name.data());
                GetDiskParts().PushBack(logic.tag_);
                ++logic_idx;
            } else {
                // Primary partitions.
                auto& prim {prim_parts_[prim_idx]};
                prim.start_lba_ = lba + entry->start_lba;
                prim.sector_count_ = entry->sector_count;
                prim.disk_ = this;
                stl::array<char, 8> part_name;
                FormatStringBuffer(part_name.data(), "{}{}", name_.data(), prim_idx + 1);
                prim.SetName(part_name.data());
                GetDiskParts().PushBack(prim.tag_);
                ++prim_idx;
            }
        }
    }

    mem::Free(boot_record);
    return *this;
}

void InitDisk() noexcept {
    dbg::Assert(!IsDiskInited());
    dbg::Assert(mem::IsMemInited());
    dbg::Assert(intr::IsIntrEnabled());

    io::PrintlnStr("Initializing disks.");
    stl::size_t inited_disk_count {0};
    // Initialize each disk on all IDE channels.
    for (stl::size_t chnl_idx {0}; chnl_idx != GetIdeChnlCount(); ++chnl_idx) {
        auto& chnl {GetIdeChnls()[chnl_idx]};
        stl::array<char, 8> chnl_name;
        FormatStringBuffer(chnl_name.data(), "ide{}", chnl_idx);
        chnl.SetName(chnl_name.data());
        io::Printf("\tInitializing the IDE channel '{}'.\n", chnl.GetName());

        // Set different interrupt numbers for two IDE channels.
        switch (chnl_idx) {
            case 0: {
                chnl.SetType(IdeChnl::Type::Primary);
                chnl.SetIntrNum(static_cast<stl::size_t>(intr::Intr::PrimaryIdeChnl));
                break;
            }
            case 1: {
                chnl.SetType(IdeChnl::Type::Secondary);
                chnl.SetIntrNum(static_cast<stl::size_t>(intr::Intr::SecondaryIdeChnl));
                break;
            }
            default: {
                dbg::Assert(false, "The system only supports two IDE channels.");
                break;
            }
        }

        intr::GetIntrHandlerTab().Register(chnl.GetIntrNum(), &DiskIntrHandler);
        for (stl::size_t disk_idx {0};
             disk_idx != IdeChnl::max_disk_count && inited_disk_count != GetDiskCount();
             ++disk_idx, ++inited_disk_count) {
            auto& disk {chnl.GetDisk(disk_idx)};
            stl::array<char, 8> disk_name;
            FormatStringBuffer(
                disk_name.data(), "sd{}",
                static_cast<char>('a' + chnl_idx * IdeChnl::max_disk_count + disk_idx));
            disk.SetName(disk_name.data());
            io::Printf("\t\tInitializing the disk '{}'.\n", disk.GetName());
            disk.Attach(&chnl, disk_idx);
            if (disk_idx != boot_disk_idx) {
                disk.ScanParts();
            }

            PrintDiskInfo(disk);
        }
    }

    IsDiskInitedImpl() = true;
    io::PrintStr("Disks have been initialized.\n");
}

bool IsDiskInited() noexcept {
    return IsDiskInitedImpl();
}

}  // namespace io