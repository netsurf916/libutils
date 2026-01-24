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
    /**
     * @brief Dynamic byte buffer for serialization and IO.
     * @details Provides a grow-free, fixed-capacity byte buffer with read/write
     *          cursors. The buffer is not resized after construction; writes
     *          truncate when capacity is exhausted. Internal locking is used,
     *          but callers should still synchronize if sharing instances across
     *          threads.
     */
    class Buffer : public Readable,
                   public Writable
    {
        private:
            uint8_t  *m_buffer;
            uint8_t  *m_start;
            uint8_t  *m_end;
            uint32_t  m_size;

        public:
            /**
             * @brief Construct a buffer with a fixed capacity.
             * @param a_size Capacity in bytes. If allocation fails, size becomes 0.
             */
            Buffer( uint32_t a_size = 65536 );
            /**
             * @brief Copy-construct a buffer and its contents.
             * @param a_value Source buffer to copy.
             * @note The copy captures current contents and cursor positions.
             */
            Buffer( Buffer &a_value );
            /**
             * @brief Destroy the buffer and release memory.
             */
            ~Buffer();

        public:
            // Read functions
            /**
             * @brief Check if the buffer can be read from.
             * @return True if readable state is available; false otherwise.
             */
            bool     IsReadable() noexcept final;
            /**
             * @brief Read a single byte from the buffer.
             * @param a_value Output byte to receive the data.
             * @param a_block Ignored; included for interface compatibility.
             * @return True if a byte was read; false if the buffer is empty.
             */
            bool     Read( uint8_t &a_value, bool a_block = false ) noexcept final;
            /**
             * @brief Read up to a_length bytes from the buffer.
             * @param a_value Destination buffer; must be non-null when a_length > 0.
             * @param a_length Number of bytes requested.
             * @param a_block Ignored; included for interface compatibility.
             * @return Number of bytes actually read; 0 if empty or invalid length.
             */
            uint32_t Read( uint8_t *a_value, uint32_t a_length, bool a_block = false ) noexcept final;
            /**
             * @brief Peek at the next byte without consuming it.
             * @param a_value Output byte to receive the peeked value.
             * @return True if a byte is available; false if the buffer is empty.
             */
            bool     Peek( uint8_t &a_value ) noexcept final;
            /**
             * @brief Peek at a byte at an offset from the current read position.
             * @param a_value Output byte to receive the peeked value.
             * @param a_index Zero-based offset from the current read cursor.
             * @return True if the index is valid; false if out of range.
             */
            bool     Peek( uint8_t &a_value, uint32_t a_index ) noexcept;
            /**
             * @brief Peek up to a_length bytes without consuming them.
             * @param a_value Destination buffer; must be non-null when a_length > 0.
             * @param a_length Number of bytes requested.
             * @return Number of bytes copied; 0 if empty or invalid length.
             */
            uint32_t Peek( uint8_t *a_value, uint32_t a_length ) noexcept final;

            // Write functions
            /**
             * @brief Check if the buffer can be written to.
             * @return True if writable state is available; false otherwise.
             */
            bool     IsWritable() noexcept final;
            /**
             * @brief Write a single byte into the buffer.
             * @param a_value Byte to write.
             * @return True if the byte was written; false if buffer is full.
             */
            bool     Write( const uint8_t &a_value ) noexcept final;
            /**
             * @brief Write bytes into the buffer.
             * @param a_value Source buffer; must be non-null when a_length > 0.
             * @param a_length Number of bytes to write. If 0, treat a_value as
             *        a null-terminated sequence.
             * @return Number of bytes written; 0 on failure or no space.
             */
            uint32_t Write( const uint8_t *a_value, uint32_t a_length = 0 ) noexcept final;

            /**
             * @brief Access the readable buffer region.
             * @return Pointer to the first readable byte, or nullptr if empty.
             */
            const uint8_t *Value();

            /**
             * @brief Convert to a raw pointer for the readable region.
             * @return Pointer to readable data, or nullptr if empty.
             */
            operator const void  *();
            /**
             * @brief Convert to a byte pointer for the readable region.
             * @return Pointer to readable data, or nullptr if empty.
             */
            operator const uint8_t *();

            /**
             * @brief Get total buffer capacity.
             * @return Capacity in bytes.
             */
            uint32_t Size();
            /**
             * @brief Get current readable length.
             * @return Number of bytes currently available for reading.
             */
            uint32_t Length();
            /**
             * @brief Get available space for writing.
             * @return Number of bytes that can be written before full.
             */
            uint32_t Space();
            /**
             * @brief Clear the buffer and reset cursors.
             */
            void     Clear();
            /**
             * @brief Trim bytes from the beginning of the buffer.
             * @param a_count Number of bytes to remove; defaults to all.
             * @note Trimming more than length clears the buffer.
             */
            void     TrimLeft ( uint32_t a_count = 0xFFFFFFFF );
            /**
             * @brief Trim bytes from the end of the buffer.
             * @param a_count Number of bytes to remove; defaults to all.
             * @note Trimming more than length clears the buffer.
             */
            void     TrimRight( uint32_t a_count = 0xFFFFFFFF );
            /**
             * @brief Compact the buffer so readable data starts at the base.
             * @note This does not change the readable data, only its position.
             */
            void     Defragment();

            /**
             * @brief Check whether the buffer contains a byte sequence.
             * @param a_value Null-terminated sequence to search for when
             *        a_length is 0; must be non-null.
             * @param a_length Sequence length; 0 triggers null-terminated scan.
             * @return True if the sequence is found; false otherwise.
             */
            bool Contains( const char  *a_value, uint32_t a_length = 0 );
            /**
             * @brief Check whether the buffer contains a byte sequence.
             * @param a_value Sequence to search for; must be non-null.
             * @param a_length Sequence length; 0 triggers null-terminated scan.
             * @return True if the sequence is found; false otherwise.
             */
            bool Contains( const uint8_t *a_value, uint32_t a_length = 0 );
    };
}

#endif // _BUFFER_HPP_
