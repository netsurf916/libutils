/**
    Lockable.hpp : Lockable interface definitions
    Description: Base class providing a lockable interface.
    Copyright 2014-2026 Daniel Wilson
    SPDX-License-Identifier: MIT
*/

#pragma once

#ifndef _LOCKABLE_HPP_
#define _LOCKABLE_HPP_

#include <utils/Types.hpp>
#include <mutex>

namespace utils
{
    class Lockable
    {
        private:
            mutable ::std::recursive_mutex m_mutex;

        protected:
            virtual ~Lockable() {};

            void Lock() noexcept;
            void Unlock() noexcept;

        friend class Lock;
    };
}

#endif // _LOCK_HPP_
