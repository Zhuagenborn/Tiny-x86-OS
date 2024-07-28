#include "kernel/io/file/file.h"
#include "kernel/debug/assert.h"
#include "kernel/io/disk/disk.h"
#include "kernel/io/disk/file/file.h"
#include "kernel/thread/thd.h"

namespace io {

void FileDesc::Close() noexcept {
    if (IsValid() && desc_ >= std_stream_count) {
        // Get the global file descriptor from the process file table.
        const auto global {tsk::ProcFileDescTab::GetGlobal(desc_)};
        auto& file_tab {fs::GetFileTab()};
        dbg::Assert(global < file_tab.GetSize());
        // Close the file in the global file table.
        file_tab[global].Close();
        tsk::ProcFileDescTab::Reset(desc_);
    }
}

File::File(const Path& path, const bit::Flags<OpenMode> flags) noexcept :
    desc_ {Open(path, flags)} {}

File::File(const FileDesc desc) noexcept : desc_ {desc} {}

File::File(File&& o) noexcept : desc_ {o.desc_} {
    o.desc_.Reset();
}

File::~File() noexcept {
    Close();
}

void File::Close() noexcept {
    if (IsOpen()) {
        desc_.Close();
        desc_.Reset();
    }
}

bool File::IsOpen() const noexcept {
    return desc_.IsValid();
}

stl::size_t File::Write(const void* const data, const stl::size_t size) noexcept {
    dbg::Assert(IsOpen());
    return GetDefaultPart().WriteFile(desc_, data, size);
}

stl::size_t File::Read(void* const buf, const stl::size_t size) noexcept {
    dbg::Assert(IsOpen());
    return GetDefaultPart().ReadFile(desc_, buf, size);
}

stl::size_t File::Seek(const stl::int32_t offset, const SeekOrigin origin) noexcept {
    dbg::Assert(IsOpen());
    return GetDefaultPart().SeekFile(desc_, offset, origin);
}

bool File::Delete(const Path& path) noexcept {
    return GetDefaultPart().DeleteFile(path);
}

FileDesc File::Open(const Path& path, const bit::Flags<OpenMode> flags) noexcept {
    return GetDefaultPart().OpenFile(path, flags);
}

namespace sc {

stl::size_t File::Open(const OpenArgs& args) noexcept {
    return io::File::Open(Path {args.path}, args.flags);
}

bool File::Delete(const char* const path) noexcept {
    return io::File::Delete(path);
}

stl::size_t File::Write(const WriteArgs& args) noexcept {
    return io::File {args.desc}.Write(args.data, args.size);
}

stl::size_t File::Read(const ReadArgs& args) noexcept {
    return io::File {args.desc}.Read(args.buf, args.size);
}

stl::size_t File::Seek(const SeekArgs& args) noexcept {
    return io::File {args.desc}.Seek(args.offset, args.origin);
}

void File::Close(const stl::size_t desc) noexcept {
    io::File {desc}.Close();
}

}  // namespace sc

}  // namespace io