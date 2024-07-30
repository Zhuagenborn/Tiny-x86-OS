#include "kernel/io/disk/ide.h"
#include "kernel/io/io.h"

namespace io {

namespace {

//! Base I/O ports and register offsets.
namespace port {
inline constexpr stl::uint16_t primary_base {0x1F0};
inline constexpr stl::uint16_t secondary_base {0x170};
inline constexpr stl::uint16_t data_offset {0};
inline constexpr stl::uint16_t error_offset {1};
inline constexpr stl::uint16_t sector_count_offset {2};
inline constexpr stl::uint16_t lba_low_offset {3};
inline constexpr stl::uint16_t lba_mid_offset {4};
inline constexpr stl::uint16_t lba_high_offset {5};
inline constexpr stl::uint16_t device_offset {6};
inline constexpr stl::uint16_t status_offset {7};
inline constexpr stl::uint16_t cmd_offset {status_offset};
inline constexpr stl::uint16_t alt_status_offset {0x206};
inline constexpr stl::uint16_t ctrl_offset {alt_status_offset};
}  // namespace port

}  // namespace

IdeChnls& GetIdeChnls() noexcept {
    static IdeChnls chnls;
    return chnls;
}

stl::size_t GetIdeChnlCount() noexcept {
    const auto count {RoundUpDivide(GetDiskCount(), IdeChnl::max_disk_count)};
    dbg::Assert(0 < count && count <= max_ide_chnl_count);
    return count;
}

IdeChnl::Type IdeChnl::GetType() const noexcept {
    return type_;
}

stl::size_t IdeChnl::GetIntrNum() const noexcept {
    return intr_num_;
}

const Disk& IdeChnl::GetDisk(const stl::size_t idx) const noexcept {
    dbg::Assert(idx < max_disk_count);
    return disks_[idx];
}

const Disk& IdeChnl::GetMasterDisk() const noexcept {
    return GetDisk(0);
}

const Disk& IdeChnl::GetSlaveDisk() const noexcept {
    return GetDisk(1);
}

const IdeChnl::Disks& IdeChnl::GetDisks() const noexcept {
    return disks_;
}

Disk& IdeChnl::GetDisk(const stl::size_t idx) noexcept {
    return const_cast<Disk&>(const_cast<const IdeChnl&>(*this).GetDisk(idx));
}

Disk& IdeChnl::GetMasterDisk() noexcept {
    return const_cast<Disk&>(const_cast<const IdeChnl&>(*this).GetMasterDisk());
}

Disk& IdeChnl::GetSlaveDisk() noexcept {
    return const_cast<Disk&>(const_cast<const IdeChnl&>(*this).GetSlaveDisk());
}

IdeChnl::Disks& IdeChnl::GetDisks() noexcept {
    return const_cast<Disks&>(const_cast<const IdeChnl&>(*this).GetDisks());
}

IdeChnl& IdeChnl::SetType(const Type type) noexcept {
    type_ = type;
    switch (type) {
        case IdeChnl::Type::Primary: {
            base_port_ = port::primary_base;
            break;
        }
        case IdeChnl::Type::Secondary: {
            base_port_ = port::secondary_base;
            break;
        }
        default: {
            base_port_ = 0;
            dbg::Assert(false, "The system only supports two IDE chnls.");
            break;
        }
    }

    return *this;
}

void IdeChnl::Block() const noexcept {
    disk_done_.acquire();
}

void IdeChnl::Unblock() const noexcept {
    disk_done_.release();
}

stl::uint16_t IdeChnl::GetSectorCountPort() const noexcept {
    return GetBasePort() + port::sector_count_offset;
}

stl::uint16_t IdeChnl::GetLbaLowPort() const noexcept {
    return GetBasePort() + port::lba_low_offset;
}

stl::uint16_t IdeChnl::GetLbaMidPort() const noexcept {
    return GetBasePort() + port::lba_mid_offset;
}

stl::uint16_t IdeChnl::GetLbaHighPort() const noexcept {
    return GetBasePort() + port::lba_high_offset;
}

stl::uint16_t IdeChnl::GetDevicePort() const noexcept {
    return GetBasePort() + port::device_offset;
}

stl::uint16_t IdeChnl::GetStatusPort() const noexcept {
    return GetBasePort() + port::status_offset;
}

stl::uint16_t IdeChnl::GetAltStatusPort() const noexcept {
    return GetBasePort() + port::alt_status_offset;
}

stl::uint16_t IdeChnl::GetCmdPort() const noexcept {
    return GetBasePort() + port::cmd_offset;
}

stl::uint16_t IdeChnl::GetDataPort() const noexcept {
    return GetBasePort() + port::data_offset;
}

stl::uint16_t IdeChnl::GetErrorPort() const noexcept {
    return GetBasePort() + port::error_offset;
}

IdeChnl& IdeChnl::SetName(const stl::string_view name) noexcept {
    if (name.empty()) {
        stl::memset(name_.data(), '\0', name_.size());
    } else {
        stl::strcpy_s(name_.data(), name_.size(), name.data());
    }

    return *this;
}

IdeChnl& IdeChnl::SetIntrNum(const stl::size_t intr_num) noexcept {
    intr_num_ = intr_num;
    return *this;
}

stl::uint16_t IdeChnl::GetCtrlPort() const noexcept {
    return GetBasePort() + port::ctrl_offset;
}

IdeChnl& IdeChnl::NeedToWaitForIntr(const bool wait) noexcept {
    return const_cast<IdeChnl&>(const_cast<const IdeChnl&>(*this).NeedToWaitForIntr(wait));
}

const IdeChnl& IdeChnl::NeedToWaitForIntr(const bool wait) const noexcept {
    waiting_intr_ = wait;
    return *this;
}

stl::mutex& IdeChnl::GetLock() const noexcept {
    return mtx_;
}

bool IdeChnl::IsWaitingForIntr() const noexcept {
    return waiting_intr_;
}

stl::uint16_t IdeChnl::GetBasePort() const noexcept {
    dbg::Assert(type_ != Type::Invalid);
    return base_port_;
}

stl::string_view IdeChnl::GetName() const noexcept {
    return name_.data();
}

}  // namespace io