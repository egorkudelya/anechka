#include "shard.h"

namespace core
{
    Shard::Shard(float maxLoadFactor, size_t expectedTokenCount)
        : m_maxLoadFactor(maxLoadFactor)
        , m_record((float)expectedTokenCount / maxLoadFactor)
    {
    }

    void Shard::insert(std::string&& token, const std::string& doc, size_t pos)
    {
        auto addNewRecord = [doc = std::string{doc}, pos = pos] {
            auto newRecord = std::make_shared<TokenRecord>();
            newRecord->addIfNotPresent(std::make_pair(std::move(doc), pos));
            return newRecord;
        };

        auto uppendToExisting = [doc = std::string{doc}, pos = pos](auto&& record) {
            record.second->addIfNotPresent(std::make_pair(std::move(doc), pos));
        };

        m_record._insert(token, std::move(addNewRecord), std::move(uppendToExisting));
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
        m_record.erase(token);
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

    size_t Shard::size() const
    {
        return m_record.size();
    }

    Json Shard::serialize() const
    {
        return m_record.serialize();
    }
}