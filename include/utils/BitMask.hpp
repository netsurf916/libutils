/**
    BitMask.hpp : BitMask definition
    Copyright 2015-2021 Daniel Wilson
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
    class BitMask : public Lockable
    {
        public:
            BitMask();
            BitMask( uint32_t a_value );
            BitMask( const utils::BitMask &a_bitMask );
            ~BitMask();

            operator uint32_t();
            BitMask &operator = ( uint32_t a_bitMask );
            BitMask &operator = ( BitMask &a_bitMask );
            bool     operator []( uint8_t a_bit );
            bool     SetBit     ( uint8_t a_bit, bool  a_set );
            bool     GetBit     ( uint8_t a_bit, bool &a_set );
            bool     IsSet      ( uint8_t a_bit );

        private:
            uint32_t m_bitMask;
    };
}

#endif // _BITMASK_HPP_
