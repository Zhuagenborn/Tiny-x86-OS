#include "kernel/io/file/path.h"
#include "kernel/debug/assert.h"

namespace io {

bool Path::IsRootDir(const stl::string_view path) noexcept {
    dbg::Assert(0 < path.size() && path.size() <= max_len);
    return path == root_dir_name || path == "/." || path == "/..";
}

bool Path::IsDir(const stl::string_view path) noexcept {
    dbg::Assert(path.size() <= max_len);
    return path.empty() || IsRootDir(path) || path.back() == separator;
}

bool Path::IsAbsolute(const stl::string_view path) noexcept {
    dbg::Assert(path.size() <= max_len);
    return !path.empty() ? path.front() == separator : false;
}

stl::string_view Path::GetFileName(const stl::string_view path) noexcept {
    if (!IsDir(path)) {
        const auto last_sep {path.rfind(separator)};
        return last_sep != stl::string_view::npos ? path.substr(last_sep + 1) : path;
    } else {
        return "";
    }
}

Path::Path(const char* const path) noexcept : Path {static_cast<stl::string_view>(path)} {}

Path::Path(const stl::string_view path) noexcept {
    Join(path);
}

stl::string_view Path::Parse(const stl::string_view path,
                             stl::array<char, max_name_len + 1>& name) noexcept {
    dbg::Assert(path.size() <= max_len);
    stl::memset(name.data(), '\0', name.size());
    if (path.empty()) {
        return nullptr;
    }

    stl::size_t src {0};
    while (path[src] == separator) {
        ++src;
    }

    stl::size_t dest {0};
    while (path[src] != separator && path[src] != '\0') {
        dbg::Assert(dest < max_len);
        name[dest++] = path[src++];
    }

    dbg::Assert(dest <= max_len);
    name[dest] = '\0';
    return path.substr(src);
}

stl::size_t Path::GetDepth(const stl::string_view path) noexcept {
    dbg::Assert(path.size() <= max_len);
    stl::size_t depth {0};
    Visit(
        path,
        [](const stl::string_view sub_path, const stl::string_view name, void* const arg) noexcept {
            dbg::Assert(arg);
            ++(*static_cast<stl::size_t*>(arg));
            return true;
        },
        &depth);
    return depth;
}

bool Path::Visit(const stl::string_view path, const Visitor visitor, void* const arg) noexcept {
    dbg::Assert(path.size() <= max_len);
    dbg::Assert(visitor);
    stl::array<char, max_name_len + 1> name;
    auto sub_path {Parse(path, name)};
    while (stl::strlen(name.data()) != 0) {
        if (!visitor(sub_path, name.data(), arg)) {
            return false;
        }

        if (!sub_path.empty()) {
            sub_path = Parse(sub_path, name);
        } else {
            stl::memset(name.data(), '\0', name.size());
        }
    }

    return true;
}

Path Path::Join(const stl::string_view parent, const stl::string_view child) noexcept {
    return Path {parent}.Join(child);
}

bool Path::IsRootDir() const noexcept {
    return IsRootDir(path_.data());
}

bool Path::IsDir() const noexcept {
    return IsDir(path_.data());
}

bool Path::IsAbsolute() const noexcept {
    return IsAbsolute(path_.data());
}

Path Path::Parse(stl::array<char, max_name_len + 1>& name) const noexcept {
    return Parse(path_.data(), name);
}

stl::size_t Path::GetDepth() const noexcept {
    return GetDepth(path_.data());
}

stl::string_view Path::GetFileName() const noexcept {
    return GetFileName(path_.data());
}

stl::string_view Path::GetPath() const noexcept {
    return path_.data();
}

bool Path::Visit(const Visitor visitor, void* const arg) const noexcept {
    return Visit(path_.data(), visitor, arg);
}

Path Path::Join(const stl::string_view child) const noexcept {
    return Path {path_.data()}.Join(child);
}

Path& Path::Join(const stl::string_view child) noexcept {
    dbg::Assert(child.size() <= max_len);

    if (GetSize() == 0 && IsAbsolute(child)) {
        path_.front() = separator;
    }

    Visit(
        child,
        [](const stl::string_view sub_path, const stl::string_view name, void* const arg) noexcept {
            const auto full {static_cast<char*>(arg)};
            dbg::Assert(full);
            const auto len {stl::strlen(full)};
            if (len > 0 && full[len - 1] != separator) {
                full[len] = separator;
            }

            dbg::Assert(stl::strlen(full) + stl::strlen(name.data()) <= max_len);
            stl::strcat(full, name.data());
            return true;
        },
        path_.data());

    if (IsDir(child)) {
        const auto size {GetSize()};
        dbg::Assert(size < max_len);
        path_[size] = separator;
        path_[size + 1] = '\0';
    }

    return *this;
}

stl::size_t Path::GetSize() const noexcept {
    return stl::strlen(path_.data());
}

Path& Path::Clear() noexcept {
    stl::memset(path_.data(), '\0', path_.size());
    return *this;
}

}  // namespace io