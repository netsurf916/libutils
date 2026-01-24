/**
    Writable.hpp : Writable interface definitions
    Description: Writable interface for serialization.
    Copyright 2015-2026 Daniel Wilson
    SPDX-License-Identifier: MIT
*/

#pragma once

#ifndef _WRITABLE_HPP_
#define _WRITABLE_HPP_

#include <utils/Lock.hpp>

namespace utils
{
    /**
     * @brief Interface for writable byte sinks.
     * @details Implementations provide synchronous write semantics.
     */
    class Writable : virtual public Lockable
    {
        public:
            /**
             * @brief Check whether the sink is writable.
             * @return True if write operations can proceed; false otherwise.
             */
            virtual bool     IsWritable() noexcept = 0;
            /**
             * @brief Write a single byte to the sink.
             * @param a_value Byte to write.
             * @return True on success; false on failure.
             */
            virtual bool     Write( const uint8_t &a_value ) noexcept = 0;
            /**
             * @brief Write a buffer of bytes to the sink.
             * @param a_value Source buffer; must be non-null when a_length > 0.
             * @param a_length Number of bytes to write.
             * @return Number of bytes written; 0 on failure.
             */
            virtual uint32_t Write( const uint8_t *a_value, uint32_t a_length ) noexcept = 0;
        protected:
            /**
             * @brief Protected virtual destructor for interface cleanup.
             */
            virtual ~Writable() {}
    };
}

#endif // _WRITABLE_HPP_
