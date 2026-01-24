/**
    Lock.cpp : Lock implementation
    Copyright 2014-2026 Daniel Wilson
    SPDX-License-Identifier: MIT
*/

#include <utils/Lock.hpp>

namespace utils
{
    Lock::Lock( Lockable *a_object )
    : m_object( a_object )
    {
        if( nullptr != m_object )
        {
            m_object->Lock();
        }
    }

    Lock::~Lock()
    {
        if( nullptr != m_object )
        {
            m_object->Unlock();
        }
    }
}
