/**
    Lock.hpp : Lock class definition
    Description: Mutex lock wrapper for thread safety.
    Copyright 2014-2026 Daniel Wilson
    SPDX-License-Identifier: MIT
*/

#pragma once

#ifndef _LOCK_HPP_
#define _LOCK_HPP_

#include <utils/Types.hpp>
#include <utils/Lockable.hpp>

namespace utils
{
    /**
     * @brief RAII guard for Lockable objects.
     * @details Locks the provided Lockable in the constructor and releases it
     *          in the destructor.
     * @note The provided Lockable pointer must remain valid for the guard's
     *       lifetime.
     */
    class Lock
    {
        private:
            Lockable *m_object;

        public:
            /**
             * @brief Acquire a lock on the given Lockable object.
             * @param a_object Lockable instance to guard; must be non-null.
             */
            Lock( Lockable *a_object );
            /**
             * @brief Release the lock held on the Lockable object.
             */
            ~Lock();
    };
}

#endif // _LOCK_HPP_
