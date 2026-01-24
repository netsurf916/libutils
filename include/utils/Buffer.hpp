/**
    Buffer.hpp : Buffer class implementation
    Description: Dynamic byte buffer for serialization and IO.
    Copyright 2014-2026 Daniel Wilson
    SPDX-License-Identifier: MIT
*/

#pragma once

#ifndef _BUFFER_HPP_
#define _BUFFER_HPP_

#include <utils/Types.hpp>
#include <utils/Readable.hpp>
#include <utils/Writable.hpp>

namespace utils
{
    class Buffer : public Readable,
                   public Writable
    {
        private:
            uint8_t  *m_buffer;
            uint8_t  *m_start;
            uint8_t  *m_end;
            uint32_t  m_size;

        public:
            Buffer( uint32_t a_size = 65536 );
            Buffer( Buffer &a_value );
            ~Buffer();

        public:
            // Read functions
            bool     IsReadable() noexcept final;
            bool     Read( uint8_t &a_value, bool a_block = false ) noexcept final;
            uint32_t Read( uint8_t *a_value, uint32_t a_length, bool a_block = false ) noexcept final;
            bool     Peek( uint8_t &a_value ) noexcept final;
            bool     Peek( uint8_t &a_value, uint32_t a_index ) noexcept;
            uint32_t Peek( uint8_t *a_value, uint32_t a_length ) noexcept final;

            // Write functions
            bool     IsWritable() noexcept final;
            bool     Write( const uint8_t &a_value ) noexcept final;
            uint32_t Write( const uint8_t *a_value, uint32_t a_length = 0 ) noexcept final;

            const uint8_t *Value();

            operator const void  *();
            operator const uint8_t *();

            uint32_t Size();
            uint32_t Length();
            uint32_t Space();
            void     Clear();
            void     TrimLeft ( uint32_t a_count = 0xFFFFFFFF );
            void     TrimRight( uint32_t a_count = 0xFFFFFFFF );
            void     Defragment();

            bool Contains( const char  *a_value, uint32_t a_length = 0 );
            bool Contains( const uint8_t *a_value, uint32_t a_length = 0 );
    };
}

#endif // _BUFFER_HPP_
