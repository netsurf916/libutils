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
    /**
     * @brief Base class providing a recursive mutex for derived types.
     * @details Derived classes can use Lock to guard critical sections.
     *          Lock/Unlock are protected to enforce RAII usage.
     */
    class Lockable
    {
        private:
            mutable ::std::recursive_mutex m_mutex;

        protected:
            /**
             * @brief Virtual destructor for proper cleanup in derived classes.
             */
            virtual ~Lockable() {};

            /**
             * @brief Acquire the recursive mutex.
             * @note Intended for use by utils::Lock.
             */
            void Lock() noexcept;

            /**
             * @brief Release the recursive mutex.
             * @note Intended for use by utils::Lock.
             */
            void Unlock() noexcept;

        friend class Lock;
    };
}

#endif // _LOCK_HPP_
