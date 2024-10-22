/**
 * @file ide.h
 * @brief The IDE channel.
 *
 * @par GitHub
 * https://github.com/Zhuagenborn
 */

#pragma once

#include "kernel/io/disk/disk.h"
#include "kernel/stl/mutex.h"
#include "kernel/stl/semaphore.h"

namespace io {

//! The IDE channel.
class IdeChnl {
public:
    //! A machine usually has two channels: primary and secondary channels.
    enum class Type { Invalid, Primary, Secondary };

    //! Each channel has up to two disks.
    static constexpr stl::size_t max_disk_count {2};

    using Disks = stl::array<Disk, max_disk_count>;

    IdeChnl() noexcept = default;

    IdeChnl(const IdeChnl&) = delete;

    IdeChnl& SetType(Type) noexcept;

    Type GetType() const noexcept;

    IdeChnl& SetIntrNum(stl::size_t) noexcept;

    IdeChnl& SetName(stl::string_view) noexcept;

    const IdeChnl& NeedToWaitForIntr(bool wait = true) const noexcept;

    IdeChnl& NeedToWaitForIntr(bool wait = true) noexcept;

    bool IsWaitingForIntr() const noexcept;

    stl::size_t GetIntrNum() const noexcept;

    stl::uint16_t GetSectorCountPort() const noexcept;

    stl::uint16_t GetLbaLowPort() const noexcept;

    stl::uint16_t GetLbaMidPort() const noexcept;

    stl::uint16_t GetLbaHighPort() const noexcept;

    stl::uint16_t GetDevicePort() const noexcept;

    stl::uint16_t GetStatusPort() const noexcept;

    stl::uint16_t GetAltStatusPort() const noexcept;

    stl::uint16_t GetCmdPort() const noexcept;

    stl::uint16_t GetDataPort() const noexcept;

    stl::uint16_t GetErrorPort() const noexcept;

    stl::uint16_t GetCtrlPort() const noexcept;

    Disk& GetMasterDisk() noexcept;

    Disk& GetSlaveDisk() noexcept;

    Disk& GetDisk(stl::size_t) noexcept;

    Disks& GetDisks() noexcept;

    const Disk& GetMasterDisk() const noexcept;

    const Disk& GetSlaveDisk() const noexcept;

    const Disk& GetDisk(stl::size_t) const noexcept;

    const Disks& GetDisks() const noexcept;

    stl::mutex& GetLock() const noexcept;

    /**
     * @brief Block the thread.
     *
     * @details
     * Call this method when we need to wait for the disk to finish an operation.
     */
    void Block() const noexcept;

    /**
     * @brief Unblock the thread.
     *
     * @details
     * Call this method when the disk has finished its operation.
     */
    void Unblock() const noexcept;

    stl::string_view GetName() const noexcept;

private:
    static constexpr stl::size_t name_len {8};

    stl::uint16_t GetBasePort() const noexcept;

    mutable stl::mutex mtx_;

    stl::array<char, name_len + 1> name_;

    //! The channel type.
    Type type_ {Type::Invalid};

    //! The base I/O port.
    stl::uint16_t base_port_ {0};

    //! The interrupt number.
    stl::size_t intr_num_ {0};

    //! The disks under the channel.
    Disks disks_;

    /**
     * @brief
     * Whether the channel is waiting for an interrupt.
     *
     * @details
     * If an operation is submitted to the disk, we need to wait for an interrupt.
     */
    mutable bool waiting_intr_ {false};

    //! Whether the disk has finished its operation.
    mutable stl::binary_semaphore disk_done_ {0};
};

//! A machine usually has two IDE channels.
inline constexpr stl::size_t max_ide_chnl_count {2};

using IdeChnls = stl::array<IdeChnl, max_ide_chnl_count>;

//! Get IDE channels.
IdeChnls& GetIdeChnls() noexcept;

//! Get the number of IDE channels.
stl::size_t GetIdeChnlCount() noexcept;

}  // namespace io