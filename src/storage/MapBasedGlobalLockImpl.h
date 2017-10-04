#ifndef AFINA_STORAGE_MAP_BASED_GLOBAL_LOCK_IMPL_H
#define AFINA_STORAGE_MAP_BASED_GLOBAL_LOCK_IMPL_H

#include <map>
#include <mutex>
#include <string>
#include <utility>

#include <afina/Storage.h>

namespace Afina {
namespace Backend {

/**
 * # Map based implementation with global lock
 *
 *
 */
class MapBasedGlobalLockImpl : public Afina::Storage {
public:
    MapBasedGlobalLockImpl(size_t max_size = 1024) : _max_size(max_size) {}
<<<<<<< HEAD
    ~MapBasedGlobalLockImpl(void) {}
=======
    ~MapBasedGlobalLockImpl() {}
>>>>>>> d1889550123c497a6de29856d5b3958b58228cd6

    // Implements Afina::Storage interface
    bool Put(const std::string &key, const std::string &value) override;

    // Implements Afina::Storage interface
    bool PutIfAbsent(const std::string &key, const std::string &value) override;

    // Implements Afina::Storage interface
    bool Set(const std::string &key, const std::string &value) override;

    // Implements Afina::Storage interface
    bool Delete(const std::string &key) override;

    // Implements Afina::Storage interface
    bool Get(const std::string &key, std::string &value) const override;

private:
    std::mutex _lock;

    size_t _max_size;

<<<<<<< HEAD
    std::map<std::string, std::pair<std::string, std::pair<std::string, std::string>>> _backend;
    std::string _head, _tail;
=======
    std::map<std::string, std::string> _backend;
>>>>>>> d1889550123c497a6de29856d5b3958b58228cd6
};

} // namespace Backend
} // namespace Afina

#endif // AFINA_STORAGE_MAP_BASED_GLOBAL_LOCK_IMPL_H
