#pragma once

#include "xxh64.h"
#include <string_view>

namespace core
{
    struct Xxh64Hasher
    {
        inline size_t operator()(std::string_view src) const
        {
            return xxh64::hash(src.data(), src.size(), 0);
        }
    };
}