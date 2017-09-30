#include <afina/allocator/Simple.h>

#include <afina/allocator/Error.h>
#include <afina/allocator/Pointer.h>

namespace Afina {
namespace Allocator {

Simple::Simple(void *base, size_t size) : _base(base), _base_len(size) {
    void *base_end = static_cast<char *>(_base) + _base_len;
    if (_base_len < sizeof (node_t)) {
        throw AllocError(AllocErrorType::NoMemory, std::string());
    }
    node_t *head = static_cast<node_t *>(base_end) - 1;
    head->next = nullptr;
    head->key = 0;
    head->data = head;
    head->size = sizeof *head;
}

/**
 * TODO: semantics
 * @param N size_t
 */
Pointer Simple::alloc(size_t N) {
    char *ptr = static_cast<char *>(_base);
    void *base_end = ptr + _base_len;
    node_t *head = static_cast<node_t *>(base_end) - 1, *prev = head;
    for (node_t *node = prev->next; node && static_cast<char *>(node->data) - ptr < N; node = node->next) {
        ptr = static_cast<char *>(node->data) + node->size;
        prev = node;
    }
    node_t *current = static_cast<node_t *>(head->data) - 1;
    if (reinterpret_cast<char *>(current) - ptr < N) {
        throw AllocError(AllocErrorType::NoMemory, std::string());
    }
    for (node_t *node = prev; node; node = node->next) {
        if (reinterpret_cast<char *>(current) - static_cast<char *>(node->data) < node->size) {
            throw AllocError(AllocErrorType::NoMemory, std::string());
        }
    }
    head->data = current;
    head->size += sizeof *current;
    Pointer p;
    p._allocator = this;
    for (node_t *node = head; node; node = node->next) {
        if (node->key == p._key) {
            ++p._key;
            node = head;
        }
    }
    current->next = prev->next;
    current->key = p._key;
    current->data = ptr;
    current->size = N;
    prev->next = current;
    return p;
}

/**
 * TODO: semantics
 * @param p Pointer
 * @param N size_t
 */
void Simple::realloc(Pointer &p, size_t N) {
    if (p._allocator == nullptr) {
        p = alloc(N);
    } else if (p._allocator == this) {
        void *base_end = static_cast<char *>(_base) + _base_len;
        node_t *head = static_cast<node_t *>(base_end) - 1, *current = head;
        while (current && current->key != p._key) {
            current = current->next;
        }
        if (current) {
            char *ptr = static_cast<char *>(current->data);
            if (static_cast<char *>(current->next ? current->next->data : head->data) - ptr < N) {
                node_t node = *current;
                free(p);
                try {
                    p = alloc(N);
                } catch (const std::exception &e) {
                    p = alloc(node.size);
                }
                current = static_cast<node_t *>(head->data);
                current->key = p._key = node.key;
                memmove(current->data, node.data, node.size);
                if (current->size < N) {
                    throw AllocError(AllocErrorType::NoMemory, std::string());
                }
            } else {
                current->size = N;
            }
        } else {
            throw AllocError(AllocErrorType::InvalidFree, std::string());
        }
    } else {
        throw AllocError(AllocErrorType::InvalidFree, std::string());
    }
}

/**
 * TODO: semantics
 * @param p Pointer
 */
void Simple::free(Pointer &p) {
    if (p._allocator != nullptr) {
        if (p._allocator != this) {
            throw AllocError(AllocErrorType::InvalidFree, std::string());
        }
        void *base_end = static_cast<char *>(_base) + _base_len;
        node_t *head = static_cast<node_t *>(base_end) - 1, *current = head->next, *prev = head;
        while (current && current->key != p._key) {
            prev = current;
            current = current->next;
        }
        if (current) {
            prev->next = current->next;
            if (current != static_cast<node_t *>(head->data)) {
                for (prev = head; prev->next != static_cast<node_t *>(head->data); prev = prev->next);
                *current = *prev->next;
                prev->next = current;
            }
            head->data = static_cast<node_t *>(head->data) + 1;
            head->size -= sizeof *current;
        } else {
            throw AllocError(AllocErrorType::InvalidFree, std::string());
        }
    }
}

/**
 * TODO: semantics
 */
void Simple::defrag(void) {
    char *ptr = static_cast<char *>(_base);
    void *base_end = ptr + _base_len;
    node_t *head = static_cast<node_t *>(base_end) - 1;
    for (node_t *node = head->next; node; node = node->next) {
        memmove(ptr, node->data, node->size);
        node->data = ptr;
        ptr += node->size;
    }
}

/**
 * TODO: semantics
 */
std::string Simple::dump(void) const { return ""; }

} // namespace Allocator
} // namespace Afina
