#pragma once

#include "../utility/common.h"
#include <memory>
#include <string>

namespace net
{
    class AbstractMessage
    {
    public:
        virtual ~AbstractMessage() = default;
        virtual Json toJson() const = 0;
        virtual void fromJson(const Json& json) = 0;
    };

    using RequestPtr = std::shared_ptr<AbstractMessage>;
    using ResponsePtr = std::shared_ptr<AbstractMessage>;
}