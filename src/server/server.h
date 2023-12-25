#pragma once

#include "../network/gen/gen_messages.h"
#include "../network/gen/gen_server_stub.h"
#include "../engine/engine.h"

namespace anechka
{
    class Anechka: public AbstractServer
    {
    public:
        explicit Anechka(const std::string& configPath);
        ~Anechka();
        Anechka(const Anechka& other) = delete;
        Anechka& operator=(const Anechka& other) = delete;
        net::ResponsePtr RequestTxtFileIndexing(const net::RequestPtr& requestPtr) override;
        net::ResponsePtr RequestRecursiveDirIndexing(const net::RequestPtr& requestPtr) override;
        net::ResponsePtr RequestTokenSearch(const net::RequestPtr& requestPtr) override;
        net::ResponsePtr RequestTokenSearchWithContext(const net::RequestPtr& requestPtr) override;
        net::ResponsePtr RequestTokenDeletion(const net::RequestPtr& requestPtr) override;
        net::ResponsePtr RequestQuerySearch(const net::RequestPtr& requestPtr) override;

    private:
        void config(const std::string& configPath);

    private:
        bool m_persistent{false};
        std::string m_dumpPath{"../dump/dump.bson"};
        core::SearchEnginePtr m_searchEngine;
    };
}