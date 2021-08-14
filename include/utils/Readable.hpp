/**
    Readable.hpp : Readable interface definitions
    Copyright 2015-2019 Daniel Wilson
*/

#pragma once

#ifndef _READABLE_HPP_
#define _READABLE_HPP_

#include <utils/Lock.hpp>

namespace utils
{
    class Readable : virtual public Lockable
    {
        public:
            virtual bool     IsReadable() noexcept = 0;
            virtual bool     Read( uint8_t &a_value, bool a_block = true ) noexcept = 0;
            virtual uint32_t Read( uint8_t *a_value, uint32_t a_length, bool a_block = true ) noexcept = 0;
            virtual bool     Peek( uint8_t &a_value ) noexcept = 0;
            virtual uint32_t Peek( uint8_t *a_value, uint32_t a_length ) noexcept = 0;
        protected:
            virtual ~Readable() {}
    };
}

#endif // _READABLE_HPP_
