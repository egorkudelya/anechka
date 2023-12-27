#include "shard.h"

namespace core
{
    // TODO
    static const float tokenDTraceShare = 0.00275f;

    Shard::Shard(float maxLoadFactor, size_t estTokenCount, size_t estDocCount)
        : m_maxLoadFactor(maxLoadFactor)
        , m_record((float)estTokenCount / maxLoadFactor)
        , m_docTrace(estDocCount)
    {
    }

    void Shard::insert(std::string&& token, const std::string& doc, DocStat&& docStat, size_t pos)
    {
        std::string_view ref = m_docTrace.addOrIncrement(doc, std::move(docStat));

        auto callback = [dcSize = m_docTrace.size(), ref, pos = pos]
                        (const auto& it, bool iterValid, bool& shouldErase)
        {
            shouldErase = false;
            if (iterValid)
            {
                it->second->addIfNotPresent(std::make_pair(ref, pos));
                return TokenRecordPtr{};
            }
            size_t expectedLoad = dcSize * tokenDTraceShare;
            auto newRecord = std::make_shared<TokenRecord>(expectedLoad);
            newRecord->addIfNotPresent(std::make_pair(ref, pos));
            return newRecord;
        };

        m_record.mutate(token, std::move(callback));
    }

    ConstTokenRecordPtr Shard::search(const std::string& token, bool& exists) const
    {
        auto defaultValue = std::make_shared<TokenRecord>();
        TokenRecordPtr res = m_record.get(token, defaultValue);
        if (res != defaultValue)
        {
            exists = true;
        }
        else
        {
            exists = false;
        }
        return res;
    }

    void Shard::erase(const std::string& token)
    {
        bool exists;
        auto record = search(token, exists);
        if (!exists)
        {
            return;
        }

        if (m_record.erase(token))
        {
            for (auto&&[path, _]: record->iterate())
            {
                m_docTrace.eraseOrDecrement(std::string{path});
            }
        }
    }

    bool Shard::exists(const std::string& token) const
    {
        return m_record.contains(token);
    }

    float Shard::loadFactor() const
    {
        return m_record.loadFactor();
    }

    bool Shard::isExpandable() const
    {
        return m_record.loadFactor() < m_maxLoadFactor;
    }

    size_t Shard::tokenCount() const
    {
        return m_record.size();
    }

    size_t Shard::docCount() const
    {
        return m_docTrace.size();
    }

    size_t Shard::tokenCountPerDoc(const std::string& path) const
    {
        return m_docTrace.getTokenCount(path);
    }

    Json Shard::serialize() const
    {
        const auto& dump = m_record.serialize();
        const auto& docTrace = m_docTrace.serialize();
        return Json{{"dump", dump}, {"docTrace", docTrace}};
    }
}