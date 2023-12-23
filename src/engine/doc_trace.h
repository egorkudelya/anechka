#pragma once

#include "umap.h"

namespace core
{
    class DocTrace
    {
    public:
        explicit DocTrace(size_t reserve);
        std::string_view addOrIncrement(const std::string& path);
        void eraseOrDecrement(const std::string& path);
        size_t getRefCount(const std::string& path) const;
        size_t size() const;

    private:
        SUMap<std::string, size_t> m_trace;
    };
}