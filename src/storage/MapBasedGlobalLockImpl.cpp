#include "MapBasedGlobalLockImpl.h"

namespace Afina {
namespace Backend {

// See MapBasedGlobalLockImpl.h
bool MapBasedGlobalLockImpl::Put(const std::string &key, const std::string &value) {
    std::unique_lock<std::mutex> guard(_lock);
    if (_max_size < 1) {
        return false;
    }
    if (_backend.empty()) {
        _head = key;
    } else if (_backend.size() == _max_size) {
        std::string next = _backend[_head].second.second;
        auto it = _backend.find(next);
        if (it != _backend.end()) {
            it->second.second.first = std::string();
        }
        _backend.erase(_head);
        _head = next;
    }
    _backend[key].first = value;
    _backend[key].second.first = _tail;
    _tail = key;
    return true;
}

// See MapBasedGlobalLockImpl.h
bool MapBasedGlobalLockImpl::PutIfAbsent(const std::string &key, const std::string &value) {
    std::unique_lock<std::mutex> guard(_lock);
    auto it = _backend.find(key);
    if (it == _backend.end()) {
        if (_max_size < 1) {
            return false;
        }
        if (_backend.empty()) {
            _head = key;
        } else if (_backend.size() == _max_size) {
            std::string next = _backend[_head].second.second;
            it = _backend.find(next);
            if (it != _backend.end()) {
                it->second.second.first = std::string();
            }
            _backend.erase(_head);
            _head = next;
        }
        _backend[key].first = value;
        _backend[key].second.first = _tail;
        _tail = key;
        return true;
    }
    return false;
}

// See MapBasedGlobalLockImpl.h
bool MapBasedGlobalLockImpl::Set(const std::string &key, const std::string &value) {
    std::unique_lock<std::mutex> guard(_lock);
    auto it = _backend.find(key);
    if (it == _backend.end()) {
        return false;
    }
    it->second.first = value;
    if (_tail != key) {
        std::string next = it->second.second.second;
        it = _backend.find(it->second.second.first);
        if (it != _backend.end()) {
            it->second.second.second = next;
        }
        _backend[next].second.first = it->first;
        _backend[_tail].second.second = key;
        _backend[key].second.first = _tail;
        _tail = key;
    }
    return true;
}

// See MapBasedGlobalLockImpl.h
bool MapBasedGlobalLockImpl::Delete(const std::string &key) {
    std::unique_lock<std::mutex> guard(_lock);
    auto it = _backend.find(key);
    if (it == _backend.end()) {
        return false;
    }
    std::string next = it->second.second.second, prev = it->second.second.first;
    _backend.erase(it);
    it = _backend.find(prev);
    if (it != _backend.end()) {
        it->second.second.second = next;
    }
    it = _backend.find(next);
    if (it != _backend.end()) {
        it->second.second.first = prev;
    }
    if (_head == key) {
        _head = next;
    }
    if (_tail == key) {
        _tail = prev;
    }
    return true;
}

// See MapBasedGlobalLockImpl.h
bool MapBasedGlobalLockImpl::Get(const std::string &key, std::string &value) const {
    std::unique_lock<std::mutex> guard(*const_cast<std::mutex *>(&_lock));
    auto it = _backend.find(key);
    if (it == _backend.end()) {
        return false;
    }
    value = it->second.first;
    return true;
}

} // namespace MapBasedGlobalLockImpl
} // namespace Afina
