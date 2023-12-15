#pragma once

#include "uset.h"
#include "json_spec.h"
#include <iostream>

#define release_assert(exp, message) {if (!(exp)) {std::cerr << (message) << std::endl; std::abort();}}

namespace utils
{
    template<typename DerivedPtr, typename BasePtr>
    std::shared_ptr<DerivedPtr> downcast(const BasePtr& basePtr)
    {
        return std::dynamic_pointer_cast<DerivedPtr>(basePtr);
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

    template <typename, bool end = false>
    void printType(const std::vector<std::string>& vec)
    {
        /**
         * Template spec specifically for client, see RequestTokenSearchWithContext
         */
        for (const auto& t: vec)
        {
            std::cout << t;
            if (t != *(vec.end() - 1))
            {
                std::cout << ", ";
            }
        }
        if (!end)
        {
            std::cout << ", ";
        }
    }

    template <typename T, bool end = false>
    void printType(T val)
    {
        std::cout << val;
        if (!end)
        {
            std::cout << ", ";
        }
    }

    template<class T, size_t... I>
    void printUnroll(const T& tup, std::index_sequence<I...> sec)
    {
        (...,
         (
             printType<decltype(std::get<I>(tup)), sec.size() - 1 == I>(std::get<I>(tup))
                 )
        );
    }

    template<class... T>
    void print(const std::tuple<T...>& tup)
    {
        std::cout << '(';
        printUnroll(tup, std::make_index_sequence<sizeof...(T)>());
        std::cout << ")\n";
    }
}