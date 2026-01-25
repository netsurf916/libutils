/**
    Readable.hpp : Readable interface definitions
    Description: Readable interface for serialization.
    Copyright 2015-2026 Daniel Wilson
    SPDX-License-Identifier: MIT
*/

#pragma once

#ifndef _READABLE_HPP_
#define _READABLE_HPP_

#include <utils/Lock.hpp>

namespace utils
{
    /**
     * @brief Interface for readable byte sources.
     * @details Implementations provide synchronous read/peek semantics.
     *          The a_block flag is advisory; implementations may ignore it.
     */
    class Readable : virtual public Lockable
    {
        public:
            /**
             * @brief Check whether the source is readable.
             * @return True if read operations can proceed; false otherwise.
             */
            virtual bool     IsReadable() noexcept = 0;

            /**
             * @brief Read a single byte from the source.
             * @param a_value Output byte to receive data.
             * @param a_block If true, implementation may block for data.
             * @return True if a byte was read; false on EOF or error.
             */
            virtual bool     Read( uint8_t &a_value, bool a_block = true ) noexcept = 0;

            /**
             * @brief Read up to a_length bytes from the source.
             * @param a_value Destination buffer; must be non-null when a_length > 0.
             * @param a_length Number of bytes requested.
             * @param a_block If true, implementation may block for data.
             * @return Number of bytes actually read; 0 on EOF or error.
             */
            virtual uint32_t Read( uint8_t *a_value, uint32_t a_length, bool a_block = true ) noexcept = 0;

            /**
             * @brief Peek the next byte without consuming it.
             * @param a_value Output byte to receive the data.
             * @return True if a byte is available; false otherwise.
             */
            virtual bool     Peek( uint8_t &a_value ) noexcept = 0;

            /**
             * @brief Peek up to a_length bytes without consuming them.
             * @param a_value Destination buffer; must be non-null when a_length > 0.
             * @param a_length Number of bytes requested.
             * @return Number of bytes copied; 0 if unavailable.
             */
            virtual uint32_t Peek( uint8_t *a_value, uint32_t a_length ) noexcept = 0;
        protected:
            /**
             * @brief Protected virtual destructor for interface cleanup.
             */
            virtual ~Readable() {}
    };
}

#endif // _READABLE_HPP_
