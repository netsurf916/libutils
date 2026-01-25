/**
    BitMask.hpp : BitMask definition
    Description: BitMask utility for flag-style bit operations.
    Copyright 2015-2026 Daniel Wilson
    SPDX-License-Identifier: MIT
*/

#pragma once

#ifndef _BITMASK_HPP_
#define _BITMASK_HPP_

#include <utils/Lockable.hpp>

#ifndef BIT
#define BIT(n) ( 1 << n )
#endif

namespace utils
{
    /**
     * @brief Thread-aware bitmask utility for flag-style operations.
     * @details Stores a 32-bit mask and provides helpers to set, clear, and query
     *          individual bits. Some methods take an internal lock, but callers
     *          should still synchronize if the same instance is accessed from
     *          multiple threads concurrently.
     */
    class BitMask : public Lockable
    {
        public:
            /**
             * @brief Create an empty bitmask with all bits cleared.
             */
            BitMask();

            /**
             * @brief Create a bitmask initialized to a specific value.
             * @param a_value Initial 32-bit mask value.
             */
            BitMask( uint32_t a_value );

            /**
             * @brief Copy-construct a bitmask from another instance.
             * @param a_bitMask Source bitmask to copy from.
             */
            BitMask( const utils::BitMask &a_bitMask );

            /**
             * @brief Destroy the bitmask instance.
             */
            ~BitMask();

            /**
             * @brief Convert the bitmask to its raw 32-bit value.
             * @return Current mask value.
             */
            operator uint32_t();

            /**
             * @brief Assign a raw 32-bit mask value.
             * @param a_bitMask New mask value to store.
             * @return Reference to this instance.
             */
            BitMask &operator = ( uint32_t a_bitMask );

            /**
             * @brief Assign from another bitmask instance.
             * @param a_bitMask Source bitmask to copy from.
             * @return Reference to this instance.
             */
            BitMask &operator = ( BitMask &a_bitMask );

            /**
             * @brief Query whether a bit is set.
             * @param a_bit Bit index (0-31).
             * @return True if the bit is set; false if clear or out of range.
             * @note This operator does not lock; synchronize externally if needed.
             */
            bool     operator []( uint8_t a_bit );

            /**
             * @brief Set or clear a specific bit.
             * @param a_bit Bit index (0-31).
             * @param a_set True to set the bit; false to clear it.
             * @return True on success; false if the bit index is out of range.
             */
            bool     SetBit     ( uint8_t a_bit, bool  a_set );

            /**
             * @brief Retrieve a bit state into a provided output variable.
             * @param a_bit Bit index (0-31).
             * @param a_set Output that receives the bit state when successful.
             * @return True on success; false if the bit index is out of range.
             */
            bool     GetBit     ( uint8_t a_bit, bool &a_set );

            /**
             * @brief Check whether a specific bit is set.
             * @param a_bit Bit index (0-31).
             * @return True if the bit is set; false if clear or out of range.
             * @note This method does not lock; synchronize externally if needed.
             */
            bool     IsSet      ( uint8_t a_bit );

        private:
            uint32_t m_bitMask;
    };
}

#endif // _BITMASK_HPP_
