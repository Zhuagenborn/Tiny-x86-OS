#include "kernel/util/tag_list.h"
#include "kernel/debug/assert.h"
#include "kernel/interrupt/intr.h"

TagList::TagList() noexcept {
    Init();
}

TagList& TagList::Init() noexcept {
    head_.next = &tail_;
    tail_.prev = &head_;
    return *this;
}

void TagList::InsertBefore(Tag& before, Tag& tag) noexcept {
    const intr::IntrGuard guard;
    before.prev->next = &tag;
    tag.prev = before.prev;
    tag.next = &before;
    before.prev = &tag;
}

TagList& TagList::PushFront(Tag& tag) noexcept {
    InsertBefore(*head_.next, tag);
    return *this;
}

TagList& TagList::PushBack(Tag& tag) noexcept {
    InsertBefore(tail_, tag);
    return *this;
}

void TagList::Tag::Detach() noexcept {
    dbg::Assert(prev && next);
    const intr::IntrGuard guard;
    prev->next = next;
    next->prev = prev;
}

TagList::Tag& TagList::Pop() noexcept {
    dbg::Assert(!IsEmpty());
    const auto top {head_.next};
    dbg::Assert(top);
    top->Detach();
    return *top;
}

bool TagList::Find(const Tag& tag) const noexcept {
    auto curr {head_.next};
    while (curr != &tail_) {
        if (curr == &tag) {
            return true;
        } else {
            curr = curr->next;
        }
    }

    return false;
}

TagList::Tag* TagList::Find(const Visitor visitor, void* const arg) const noexcept {
    dbg::Assert(visitor);
    auto curr {head_.next};
    while (curr != &tail_) {
        if (visitor(*curr, arg)) {
            return curr;
        } else {
            curr = curr->next;
        }
    }

    return nullptr;
}

stl::size_t TagList::GetSize() const noexcept {
    stl::size_t len {0};
    auto curr {head_.next};
    while (curr != &tail_) {
        curr = curr->next;
        ++len;
    }

    return len;
}

bool TagList::IsEmpty() const noexcept {
    return head_.next == &tail_;
}