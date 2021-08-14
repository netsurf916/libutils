/**
    KeyValuePair.hpp : KeyValuePair definition
    Copyright 2016-2019 Daniel Wilson
*/

#pragma once

#ifndef _KEYVALUEPAIR_HPP_
#define _KEYVALUEPAIR_HPP_

#include <utils/Types.hpp>
#include <utils/Lock.hpp>
#include <utils/Serializable.hpp>
#include <utils/Tokens.hpp>
#include <memory>

namespace utils
{
    template< typename key_type, typename value_type >
    class KeyValuePair : public Serializable
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
                m_key.Clear();
                m_value.Clear();
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

            // Serializable functions
            uint8_t Type() noexcept final
            {
                return SerializableType::KeyValuePair;
            }

            bool Serialize( Writable &a_out ) noexcept final
            {
                ::utils::Lock lock( this );
                return SerializeType( a_out ) && m_key.Serialize( a_out ) && m_value.Serialize( a_out );
            }

            bool Deserialize( Readable &a_in ) noexcept final
            {
                ::utils::Lock lock( this );
                return DeserializeType( a_in ) && m_key.Deserialize( a_in ) && m_value.Deserialize( a_in );
            }

            void ToJson( String &a_json )
            {
                ::std::shared_ptr< KeyValuePair< key_type, value_type > > meta = this;
                String temp;
                a_json += "{";
                while( meta )
                {
                    a_json += "\"";
                    a_json += Tokens::EscapeJson( meta->Key(), temp ); temp.Clear();
                    a_json += "\"";
                    a_json += ":";
                    a_json += "\"";
                    a_json += Tokens::EscapeJson( meta->Value(), temp ); temp.Clear();
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
