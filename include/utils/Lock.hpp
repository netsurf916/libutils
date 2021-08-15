/**
    Lock.hpp : Lock class definition
    Copyright 2014-2021 Daniel Wilson
*/

#pragma once

#ifndef _LOCK_HPP_
#define _LOCK_HPP_

#include <utils/Types.hpp>
#include <utils/Lockable.hpp>

namespace utils
{
    class Lock
    {
        private:
            Lockable *m_object;

        public:
            Lock( Lockable *a_object );
            ~Lock();
    };
}

#endif // _LOCK_HPP_
