#include "doc_trace.h"

namespace core
{
    DocTrace::DocTrace(size_t reserve)
        : m_trace(reserve)
    {
    }

    std::string_view DocTrace::addOrIncrement(const std::string& path)
    {
        std::string_view hook;
        auto callback = [&hook](const auto& it, bool iterValid) {
            if (iterValid)
            {
                it->second++;
                hook = it->first;
            }
            return 0;
        };

        m_trace.mutate(path, callback);
        if (hook.empty())
        {
            m_trace.mutate(path, callback);
        }

        return hook;
    }

    void DocTrace::eraseOrDecrement(const std::string& path)
    {
        auto callback = [](const auto& it, bool iterValid) {
            if (iterValid)
            {
                it->second--;
                if (it->second == 0)
                {
                    it->first.erase();
                }
            }
            return 0;
        };

        m_trace.mutate(path, std::move(callback));
    }

    size_t DocTrace::getRefCount(const std::string& path) const
    {
        return m_trace.get(path);
    }

    size_t DocTrace::size() const
    {
        return m_trace.size();
    }
}