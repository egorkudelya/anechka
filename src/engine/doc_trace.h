#pragma once

#include "umap.h"

namespace core
{
    struct DocStat
    {
        size_t tokenCount;
    };

    void to_json(Json& json, const DocStat& docStat);
    void from_json(const Json& json, DocStat& docStat);

    class DocTrace
    {
        /**
        * DocTrace is responsible for keeping track of currently indexed documents and their statistics
        * (e.g the number of tokens in a particular document). Documents are reference-counted and those
        * not in use are deleted using eraseOrDecrement.
        */
    public:
        explicit DocTrace(size_t reserve);
        std::string_view addOrIncrement(const std::string& path, DocStat&& docStat);
        void eraseOrDecrement(const std::string& path);
        size_t getRefCount(const std::string& path) const;
        size_t getTokenCount(const std::string& path) const;
        float getAvgTokenCount() const;
        size_t size() const;
        Json serialize() const;

    private:
        std::atomic<float> m_avgTokenCount{0.0f};
        SUMap<std::string, size_t> m_trace;
        SUMap<std::string_view, DocStat> m_stats;
    };
}
