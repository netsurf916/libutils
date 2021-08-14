/**
    Writable.hpp : Writable interface definitions
    Copyright 2015-2019 Daniel Wilson
*/

#pragma once

#ifndef _WRITABLE_HPP_
#define _WRITABLE_HPP_

#include <utils/Lock.hpp>

namespace utils
{
    class Writable : virtual public Lockable
    {
        public:
            virtual bool     IsWritable() noexcept = 0;
            virtual bool     Write( const uint8_t &a_value ) noexcept = 0;
            virtual uint32_t Write( const uint8_t *a_value, uint32_t a_length ) noexcept = 0;
        protected:
            virtual ~Writable() {}
    };
}

#endif // _WRITABLE_HPP_
