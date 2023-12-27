#include "doc_trace.h"

namespace core
{
    void to_json(Json& json, const DocStat& docStat)
    {
        json = Json {{"tokenCount", docStat.tokenCount}};
    }

    void from_json(const Json& json, DocStat& docStat)
    {
        json.at("tokenCount").get_to(docStat.tokenCount);
    }

    DocTrace::DocTrace(size_t reserve)
        : m_trace(reserve)
        , m_stats(reserve)
    {
    }

    std::string_view DocTrace::addOrIncrement(const std::string& path, DocStat&& docStat)
    {
        std::string_view hook;
        auto callback = [&hook](const auto& it, bool iterValid, bool& shouldErase) {
            shouldErase = false;
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
            m_stats.insert(hook, docStat);
        }

        return hook;
    }

    void DocTrace::eraseOrDecrement(const std::string& path)
    {
        m_trace.mutate(path, [](const auto& it, bool iterValid, bool& shouldErase) {
            shouldErase = false;
            if (iterValid)
            {
                it->second--;
                if (it->second == 0)
                {
                    shouldErase = true;
                }
            }
            return 0;
        });
        if (!m_trace.contains(path))
        {
            m_stats.erase(path);
        }
    }

    size_t DocTrace::getRefCount(const std::string& path) const
    {
        return m_trace.get(path);
    }

    size_t DocTrace::getTokenCount(const std::string& path) const
    {
        return m_stats.get(path).tokenCount;
    }

    size_t DocTrace::size() const
    {
        return m_trace.size();
    }

    Json DocTrace::serialize() const
    {
        return m_stats.serialize();
    }
}