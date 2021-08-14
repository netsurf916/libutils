/**
    String.hpp : String definition
    Copyright 2014-2019 Daniel Wilson
*/

#pragma once

#ifndef _STRING_HPP_
#define _STRING_HPP_

#include <utils/Types.hpp>
#include <utils/Lock.hpp>
#include <utils/Serializable.hpp>
#include <utils/Primitive.hpp>

namespace utils
{
    class String : public Serializable
    {
        private:
            char                  *m_data;
            Primitive< uint16_t >  m_length;
            Primitive< uint16_t >  m_size;
            Primitive< uint8_t  >  m_blockSize;

        public:
            String();
            String( const char &a_value );
            String( const char *a_value, uint16_t a_length = 0 );
            String( const String &a_value );
            ~String();

            // Serializable functions
            uint8_t Type() noexcept final;
            bool    Serialize  ( Writable &a_out ) noexcept final;
            bool    Deserialize( Readable &a_in  ) noexcept final;

            operator bool();
            operator const char *();
            operator const uint8_t *();
            const char *Value();

            String &operator= ( const char   &a_value );
            String &operator= ( const char   *a_value );
            String &operator= (       String &a_value );
            String &operator+=( const char   &a_value );
            String &operator+=( const char   *a_value );
            String &operator+=(       String &a_value );
            bool    operator==( const char   &a_value );
            bool    operator==( const char   *a_value );
            bool    operator==(       String &a_value );
            bool    operator!=( const char   &a_value );
            bool    operator!=( const char   *a_value );
            bool    operator!=(       String &a_value );
            char   &operator[]( uint16_t a_index );

            bool    Contains( const char *a_value, uint16_t a_length = 0 );

            uint16_t Size();
            uint16_t Length();
            bool     SetValue( const char *a_value, uint16_t a_length = 0 );
            void     Clear();
            String  &ToLower();
            String  &ToUpper();
            int64_t  ToInt();
            uint64_t ToUint();
            String  &Trim( const char &a_value = ' ' );

            static uint16_t MaxLength();
    };
}

#endif // _STRING_HPP_
