/**
    Thread.hpp : Thread class definition
    Description: Thread wrapper utilities.
    Copyright 2015-2026 Daniel Wilson
    SPDX-License-Identifier: MIT
*/

#include <utils/Lock.hpp>
#include <pthread.h>
#include <signal.h>
#include <memory>

#pragma once

#ifndef _THREAD_HPP_
#define _THREAD_HPP_

namespace utils
{
    template< typename type >
    class Thread : public Lockable
    {
        private:
            pthread_t                  m_thread;
            pthread_attr_t             m_attributes;
            ::std::shared_ptr< type >  m_context;
            bool                       m_ok;
            bool                       m_running;
            void                      *( *m_function ) ( void * );

        public:
            Thread( void *( *a_function ) ( void * ) )
            {
                m_context  = ::std::make_shared< type >();
                m_function = a_function;
                m_running  = false;
                m_ok = ( nullptr != m_function );
                m_ok = ( nullptr != m_context );
                m_ok = m_ok && ( 0 == pthread_attr_init( &m_attributes ) );
                m_ok = m_ok && ( 0 == pthread_attr_setdetachstate( &m_attributes, PTHREAD_CREATE_JOINABLE ) );
            }

            ~Thread()
            {
                utils::Lock lock( this );
                Join();
                pthread_attr_destroy( &m_attributes );
            }

            ::std::shared_ptr< type > &GetContext()
            {
                return m_context;
            }

            bool Start()
            {
                utils::Lock lock( this );
                m_ok = m_ok && ( 0 == pthread_create( &m_thread, &m_attributes, m_function, ( void * ) &( *m_context ) ) );
                m_running = m_ok;
                return m_ok;
            }

            bool Join()
            {
                utils::Lock lock( this );
                m_ok = m_ok && m_running && ( 0 == pthread_join( m_thread, nullptr ) );
                m_running = !m_ok;
                return m_ok;
            }

            bool Kill()
            {
                utils::Lock lock( this );
                m_ok = m_ok && m_running && ( 0 == pthread_kill( m_thread, SIGKILL ) );
                m_running = !m_ok;
                return m_ok;
            }

            bool Cancel()
            {
                utils::Lock lock( this );
                m_ok = m_ok && m_running && ( 0 == pthread_cancel( m_thread ) );
                m_running = !m_ok;
                return m_ok;
            }
    };
}

#endif // _THREAD_HPP_
