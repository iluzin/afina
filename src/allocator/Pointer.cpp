#include <afina/allocator/Pointer.h>

namespace Afina {
namespace Allocator {

void *Pointer::get(void) const {
    void *base_end = static_cast<char *>(_allocator->_base) + _allocator->_base_len;
    Simple::node_t *head = static_cast<Simple::node_t *>(base_end) - 1;
    for (Simple::node_t *node = head; node; node = node->next) {
        if (_key == node->key) {
            return node->data;
        }
    }
    return nullptr;
}

} // namespace Allocator
} // namespace Afina
