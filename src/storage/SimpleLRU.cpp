#include "SimpleLRU.h"

namespace Afina {
namespace Backend {


// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Put(const std::string &key, const std::string &value) {
    return (Set(key, value) || PutIfAbsent(key, value));
    }

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::PutIfAbsent(const std::string &key, const std::string &value) {
    if (!CheckKey(key)) {
        std::size_t elem_size = key.size() + value.size();
        if (elem_size > _max_size) { //no garantee
            return false;
            }
        if (elem_size + _cur_size > _max_size) {
            DeleteRequired(elem_size);
            }
        _cur_size += key.size() + value.size();
        auto *node = new lru_node(key, value);
        AppendTail(node);
        _lru_index.insert(std::make_pair(std::reference_wrapper<const std::string>(node->key), std::reference_wrapper<lru_node>(*node)));
        return true;
        }

    return false;
    }

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Set(const std::string &key, const std::string &value) {
    auto it = _lru_index.find(key);
    if (it == _lru_index.end()) { // no key
        return false;
    }

    auto *node = &it->second.get();
    std::size_t old_size = (*node).key.size() + (*node).value.size();
    std::size_t size = key.size() + value.size();

    if (old_size > size) {
        _cur_size += size - old_size;
    } else {
        if (size - old_size > _max_size - _cur_size) {
            return false;
        }

        _cur_size += size - old_size;
    }

    (*node).value = value;
    ToTail(node);
    return true;
}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Delete(const std::string &key) {
    auto it = _lru_index.find(key);

    if (it == _lru_index.end()) {
        return false; // no key
    }
    auto *node = &it->second.get();
    //_lru_index.erase(key);
    _lru_index.erase(it);
    _cur_size -= (*node).key.size() + (*node).value.size();

    if (_lru_head.get() == node) {
        _lru_head = std::move(_lru_head->next);
        _lru_head->prev = nullptr;
    } else if(_lru_tail == node) {
        _lru_tail = _lru_tail->prev;
        _lru_tail->next.reset();
    } else {
        auto tmp = std::move((*node).prev->next);
        (*node).prev->next = std::move(tmp->next);
        (*node).next->prev = tmp->prev;
        tmp.reset();
    }

    return true;
}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Get(const std::string &key, std::string &value) { // std::string?
    auto it = _lru_index.find(key);

    if (it == _lru_index.end()) {
        return false;
    }

    auto *node = &it->second.get();
    value = node->value;
    ToTail(node);
    return true;
}

//SimpleLRU::
void SimpleLRU::DeleteRequired(const std::size_t &size) {
    while (size > _max_size - _cur_size) {
        if (_lru_head.get() == nullptr) { // already empty list
            return;
        }
        _lru_index.erase(_lru_head->key);
        _cur_size -= _lru_head->key.size() + _lru_head->value.size();

        if (_lru_head->next == nullptr) { // 1 elem case
            _lru_head.reset();
            _lru_tail = nullptr;
        } else {
            _lru_head = std::move(_lru_head->next);
            _lru_head->prev = nullptr;
        }
    }
}

void SimpleLRU::AppendTail(lru_node *node) {
    if(_lru_tail == nullptr) { // empty list
        _lru_head = std::unique_ptr<lru_node>(node);
        _lru_tail = node;
    } else {
        node->prev = _lru_tail;
        _lru_tail->next.reset(node); // move?
        //_lru_tail->next = std::move(node);
        _lru_tail = node;
    }
}

bool SimpleLRU::CheckKey(const std::string &key) const {
    auto it = _lru_index.find(key);
    return (it != _lru_index.end());
}

void SimpleLRU::ToTail(lru_node *node) {
    if (node == _lru_tail) { // already in tail
        return;
    }

    if (node == _lru_head.get()) {
        auto tmp = std::move(_lru_head->next);
        _lru_head->prev = _lru_tail;
        _lru_tail->next = std::move(_lru_head);
        _lru_tail = _lru_tail->next.get(); // head is tail now
        _lru_head = std::move(tmp); // 2-nd is head now
        _lru_head->prev = nullptr;

    } else {
        auto prev = node->prev;
        auto next = std::move(node->next);
        std::unique_ptr<lru_node> tmp = std::move(prev->next); // save ptr
        //std::unique_ptr<lru_node> tmp = nullptr;
        //tmp.reset(node);
        next->prev = prev; // connect neighbors
        prev->next = std::move(next);
        (tmp)->prev = _lru_tail; // node to tail
        //_lru_tail->next.reset(tmp);
        _lru_tail->next = std::move(tmp);
        _lru_tail = _lru_tail->next.get();

    }
}
} // namespace Backend
} // namespace Afina








