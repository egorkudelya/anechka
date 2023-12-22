#pragma once

#include "uset.h"
#include "json_spec.h"
#include <iostream>

#define release_assert(exp, message) {if (!(exp)) {std::cerr << (message) << std::endl; std::abort();}}

namespace utils
{
    template<typename DerivedType, typename BasePtr>
    std::shared_ptr<DerivedType> downcast(const BasePtr& basePtr)
    {
        return std::dynamic_pointer_cast<DerivedType>(basePtr);
    }

    inline void toLower(std::string& data)
    {
        std::transform(data.begin(), data.end(), data.begin(), [](unsigned char c) {
            return std::tolower(c);
        });
    }

    inline void removeControlChars(std::string& data)
    {
        data.erase(std::remove_if(data.begin(), data.end(),
                                  [](char c) {
                                      return std::iscntrl(c);
                                  }), data.end());
    }

    inline void removeLeading(std::string& str, char charToRemove)
    {
        str.erase(0, std::min(str.find_first_not_of(charToRemove), str.size() - 1));
    }

    template<class T>
    inline void hash_combine(size_t& seed, const T& v)
    {
        std::hash<T> hasher;
        seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    }
}