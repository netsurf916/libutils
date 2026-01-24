/**
    Lockable.cpp : Lockable definitions
    Copyright 2014-2026 Daniel Wilson
    SPDX-License-Identifier: MIT
*/

#include <utils/Lockable.hpp>
#include <exception>

namespace utils
{
    void Lockable::Lock() noexcept
    {
        try
        {
            m_mutex.lock();
        }
        catch( const ::std::exception &e )
        {
            UNUSED( e );
        }
    }

    void Lockable::Unlock() noexcept
    {
        m_mutex.unlock();
    }
}
