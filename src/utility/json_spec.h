#pragma once
#include "../json/json.h"

using Json = nlohmann::json;

namespace utils
{
    namespace detail
    {
        template<typename T>
        struct is_pointer {static const bool value = false;};

        template<typename T>
        struct is_pointer<T*> {static const bool value = true;};

        template<typename T, typename Enable = void>
        struct is_smart_pointer
        {
            enum
            {
                value = false
            };
        };

        template<typename T>
        struct is_smart_pointer<T, typename std::enable_if_t<
                   std::is_same_v<typename std::remove_cv_t<T>, std::shared_ptr<typename T::element_type>>>>
        {
            enum
            {
                value = true
            };
        };

        template<typename T>
        struct is_smart_pointer<T, typename std::enable_if_t<
                   std::is_same_v<typename std::remove_cv_t<T>, std::unique_ptr<typename T::element_type>>>>
        {
            enum
            {
                value = true
            };
        };

        template<typename T>
        struct is_smart_pointer<T, typename std::enable_if_t<
                   std::is_same_v<typename std::remove_cv_t<T>, std::weak_ptr<typename T::element_type>>>>
        {
            enum
            {
                value = true
            };
        };
    }

    template<typename T>
    Json serializeType(T&& obj)
    {
        /**
         * Serializable type must have either serialize() method returning Json
         * OR Json must be constructible from the type
         */
        using namespace detail;
        Json val;

        using FilteredType = std::decay_t<T>;
        if constexpr (is_pointer<FilteredType>::value || is_smart_pointer<FilteredType>::value)
        {
            using UnderlyingType = std::decay_t<decltype(*std::declval<FilteredType>())>;
            if constexpr (std::is_constructible_v<Json, UnderlyingType>)
            {
                val = *obj;
            }
            else if constexpr (std::is_member_function_pointer_v<decltype(&UnderlyingType::serialize)>)
            {
                val = obj->serialize();
            }
        }
        else if constexpr (std::is_constructible_v<Json, FilteredType>)
        {
            val = obj;
        }
        else if constexpr (std::is_member_function_pointer_v<decltype(&FilteredType::serialize)>)
        {
            val = obj.serialize();
        }

        return val;
    }
}