/**
    KeyValuePair.hpp : KeyValuePair definition
    Description: Key/value pair helper structure.
    Copyright 2016-2026 Daniel Wilson
    SPDX-License-Identifier: MIT
*/

#pragma once

#ifndef _KEYVALUEPAIR_HPP_
#define _KEYVALUEPAIR_HPP_

#include <utils/Types.hpp>
#include <utils/Lock.hpp>
#include <utils/Tokens.hpp>
#include <utils/Lockable.hpp>
#include <memory>
#include <string>

namespace utils
{
    template< typename key_type, typename value_type >
    /**
     * @brief Simple linked key/value pair node.
     * @details Stores a key, a value, and a next pointer to form a linked list.
     *          Accessors acquire a lock for basic thread-safety, but callers
     *          should still synchronize when iterating across nodes.
     */
    class KeyValuePair : public Lockable
    {
        private:
            key_type   m_key;
            value_type m_value;
            ::std::shared_ptr< KeyValuePair< key_type, value_type > > m_next;

        public:
            /**
             * @brief Construct an empty key/value pair with default values.
             */
            KeyValuePair()
            : m_key()
            , m_value()
            , m_next()
            {}

            /**
             * @brief Destroy the pair and clear key/value storage.
             */
            ~KeyValuePair()
            {
                ::utils::Lock lock( this );
                m_key.clear();
                m_value.clear();
            }

            /**
             * @brief Access the key.
             * @return Mutable reference to the key.
             */
            key_type &Key()
            {
                ::utils::Lock lock( this );
                return m_key;
            }

            /**
             * @brief Access the value.
             * @return Mutable reference to the value.
             */
            value_type &Value()
            {
                ::utils::Lock lock( this );
                return m_value;
            }

            /**
             * @brief Access the next node in the list.
             * @return Shared pointer to the next node.
             */
            ::std::shared_ptr< KeyValuePair< key_type, value_type > > &Next()
            {
                ::utils::Lock lock( this );
                return m_next;
            }

            /**
             * @brief Serialize the linked list into a JSON object string.
             * @param a_json Output string to append JSON into.
             * @note This appends to the provided string without clearing it.
             */
            void ToJson( ::std::string &a_json )
            {
                ::std::shared_ptr< KeyValuePair< key_type, value_type > > meta = this;
                ::std::string temp;
                a_json += "{";
                while( meta )
                {
                    a_json += "\"";
                    a_json += Tokens::EscapeJson( meta->Key(), temp ); temp.clear();
                    a_json += "\"";
                    a_json += ":";
                    a_json += "\"";
                    a_json += Tokens::EscapeJson( meta->Value(), temp ); temp.clear();
                    a_json += "\"";
                    meta = meta->Next();
                    if( meta )
                    {
                        a_json += ",";
                    }
                }
                a_json += "}";
            }
        };
}

#endif // _KEYVALUEPAIR_HPP_
