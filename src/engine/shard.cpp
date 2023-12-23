#include "shard.h"

namespace core
{
    Shard::Shard(float maxLoadFactor, size_t expectedTokenCount)
        : m_maxLoadFactor(maxLoadFactor)
        , m_record((float)expectedTokenCount / maxLoadFactor)
        , m_docTrace(expectedTokenCount/3)
    {
    }

    void Shard::insert(std::string&& token, const std::string& doc, size_t pos)
    {
        std::string_view ref = m_docTrace.addOrIncrement(doc);
        auto callback = [ref, pos = pos](const auto& it, bool iterValid) {
            if (iterValid)
            {
                it->second->addIfNotPresent(std::make_pair(ref, pos));
                return TokenRecordPtr{};
            }
            auto newRecord = std::make_shared<TokenRecord>();
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
            for (auto&&[path, _]: record->values())
            {
                m_docTrace.eraseOrDecrement(std::string{path});
            }
        }
    }

    bool Shard::exists(const std::string& token) const
    {
        return m_record.exists(token);
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

    Json Shard::serialize() const
    {
        return m_record.serialize();
    }
}