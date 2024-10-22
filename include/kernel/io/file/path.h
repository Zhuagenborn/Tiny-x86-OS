/**
 * @file path.h
 * @brief File path operations.
 *
 * @par GitHub
 * https://github.com/Zhuagenborn
 */

#pragma once

#include "kernel/stl/array.h"
#include "kernel/stl/string_view.h"

namespace io {

class Path {
public:
    //! The maximum length of a path.
    static constexpr stl::size_t max_len {512};
    //! The maximum length of a file or directory name.
    static constexpr stl::size_t max_name_len {16};

    //! The root directory.
    static constexpr stl::string_view root_dir_name {"/"};
    //! The current directory.
    static constexpr stl::string_view curr_dir_name {"."};
    //! The parent directory.
    static constexpr stl::string_view parent_dir_name {".."};

    static constexpr char separator {'/'};

    //! A callback function used to visit names in a path.
    using Visitor = bool (*)(stl::string_view sub_path, stl::string_view name, void* arg) noexcept;

    static bool IsRootDir(stl::string_view) noexcept;

    static bool IsDir(stl::string_view) noexcept;

    static bool IsAbsolute(stl::string_view) noexcept;

    /**
     * @brief Get the depth of a path.
     *
     * @details
     * @code {.cpp}
     * Path::GetDepth("/") == 0;
     * Path::GetDepth("/a") == 1;
     * Path::GetDepth("/a/b") == 2;
     * @endcode
     */
    static stl::size_t GetDepth(stl::string_view) noexcept;

    /**
     * @brief Get the file name of a path.
     *
     * @details
     * @code {.cpp}
     * Path::GetFileName("/") == "";
     * Path::GetFileName("/a") == "a";
     * Path::GetFileName("/a/") == "";
     * @endcode
     */
    static stl::string_view GetFileName(stl::string_view) noexcept;

    static Path Join(stl::string_view parent, stl::string_view child) noexcept;

    /**
     * @brief Parse the first name in a path.
     *
     * @param[in] path A path to be parsed.
     * @param[out] name The first name in the path.
     * @return The remaining path.
     */
    static stl::string_view Parse(stl::string_view path,
                                  stl::array<char, max_name_len + 1>& name) noexcept;

    /**
     * @brief Visit all names in a path.
     *
     * @param path A path to be visited.
     * @param visitor
     * A callback function accepting each name and the remaining path.
     * If it returns @p false, the visiting stops and returns @p false.
     * @param arg An optional argument passed to the callback function.
     * @return Whether all names have been visited.
     *
     * @details
     * For example, the path @p "/a/b/c" will be visited as:
     * 1. The name is @p "a". The subpath is @p "/b/c".
     * 2. The name is @p "b". The subpath is @p "/c".
     * 3. The name is @p "c". The subpath is @p "".
     */
    static bool Visit(stl::string_view path, Visitor visitor, void* arg = nullptr) noexcept;

    Path(stl::string_view = nullptr) noexcept;

    Path(const char*) noexcept;

    bool IsRootDir() const noexcept;

    bool IsDir() const noexcept;

    bool IsAbsolute() const noexcept;

    stl::size_t GetDepth() const noexcept;

    Path Parse(stl::array<char, max_name_len + 1>& name) const noexcept;

    Path Join(stl::string_view child) const noexcept;

    Path& Join(stl::string_view child) noexcept;

    bool Visit(Visitor, void* arg = nullptr) const noexcept;

    Path& Clear() noexcept;

    stl::string_view GetFileName() const noexcept;

    stl::string_view GetPath() const noexcept;

    stl::size_t GetSize() const noexcept;

    operator stl::string_view() const noexcept {
        return path_.data();
    }

private:
    stl::array<char, max_len + 1> path_;
};

}  // namespace io