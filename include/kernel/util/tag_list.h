/**
 * @file tag_list.h
 * @brief The tag list.
 *
 * @par GitHub
 * https://github.com/Zhuagenborn
 */

#pragma once

#include "kernel/stl/cstdint.h"

/**
 * @brief
 * The doubly linked tag list.
 *
 * @details
 * It connects a number of tags. Each tag is a member of an object.
 *
 * @code
 *              Object         Object
 *  Head       ┌───────┐      ┌───────┐      Tail
 *  ┌───┐ ───► │ ┌───┐ │ ───► │ ┌───┐ │ ───► ┌───┐
 *  │Tag│      │ │Tag│ │      │ │Tag│ │      │Tag│
 *  └───┘ ◄─── │ └───┘ │ ◄─── │ └───┘ │ ◄─── └───┘
 *             └───────┘      └───────┘
 * @endcode
 */
class TagList {
public:
    struct Tag {
        Tag() noexcept = default;

        Tag(const Tag&) = delete;

        /**
         * @brief Get the object containing the tag.
         *
         * @tparam T The object type.
         * @tparam offset The tag offset from the beginning of the object.
         */
        template <typename T, stl::size_t offset = 0>
        T& GetElem() const noexcept {
            return const_cast<T&>(
                *reinterpret_cast<const T*>(reinterpret_cast<const stl::byte*>(this) - offset));
        }

        //! Detach the tag from the list.
        void Detach() noexcept;

        Tag* prev {nullptr};
        Tag* next {nullptr};
    };

    using Visitor = bool (*)(const Tag&, void*) noexcept;

    static void InsertBefore(Tag& before, Tag& tag) noexcept;

    TagList() noexcept;

    TagList(const TagList&) = delete;

    TagList& Init() noexcept;

    TagList& PushFront(Tag&) noexcept;

    TagList& PushBack(Tag&) noexcept;

    Tag& Pop() noexcept;

    bool Find(const Tag&) const noexcept;

    Tag* Find(Visitor, void* arg = nullptr) const noexcept;

    stl::size_t GetSize() const noexcept;

    bool IsEmpty() const noexcept;

private:
    Tag head_;
    Tag tail_;
};