/**
    BitMask.hpp : BitMask definition
    Copyright 2015-2019 Daniel Wilson
*/

#pragma once

#ifndef _BITMASK_HPP_
#define _BITMASK_HPP_

#include <utils/Serializable.hpp>
#include <utils/Primitive.hpp>

#ifndef BIT
#define BIT(n) ( 1 << n )
#endif

namespace utils
{
    class BitMask : public Serializable
    {
        public:
            BitMask();
            BitMask( uint32_t a_value );
            ~BitMask();

            operator uint32_t();
            BitMask &operator = ( uint32_t a_bitMask );
            BitMask &operator = ( BitMask &a_bitMask );
            bool     operator []( uint8_t a_bit );
            bool     SetBit     ( uint8_t a_bit, bool  a_set );
            bool     GetBit     ( uint8_t a_bit, bool &a_set );
            bool     IsSet      ( uint8_t a_bit );

            // Serializable functions
            uint8_t  Type() noexcept final;
            bool     Serialize  ( Writable &a_out ) noexcept final;
            bool     Deserialize( Readable &a_in  ) noexcept final;

        private:
            Primitive< uint32_t > m_bitMask;
    };
}

#endif // _BITMASK_HPP_
