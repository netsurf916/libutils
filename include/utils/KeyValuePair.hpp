/**
    KeyValuePair.hpp : KeyValuePair definition
    Copyright 2016-2021 Daniel Wilson
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
    class KeyValuePair : public Lockable
    {
        private:
            key_type   m_key;
            value_type m_value;
            ::std::shared_ptr< KeyValuePair< key_type, value_type > > m_next;

        public:
            KeyValuePair()
            : m_key()
            , m_value()
            , m_next()
            {}

            ~KeyValuePair()
            {
                ::utils::Lock lock( this );
                m_key.clear();
                m_value.clear();
            }

            key_type &Key()
            {
                ::utils::Lock lock( this );
                return m_key;
            }

            value_type &Value()
            {
                ::utils::Lock lock( this );
                return m_value;
            }

            ::std::shared_ptr< KeyValuePair< key_type, value_type > > &Next()
            {
                ::utils::Lock lock( this );
                return m_next;
            }

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
