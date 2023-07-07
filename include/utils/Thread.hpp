/**
    Thread.hpp : Thread class definition
    Copyright 2015-2021 Daniel Wilson
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
            bool                       m_started;
            void                      *( *m_function ) ( void * );

        public:
            Thread( void *( *a_function ) ( void * ) )
            {
                m_context    = ::std::make_shared< type >();
                m_function   = a_function;
                m_started    = false;
                m_ok = ( m_function != nullptr );
                m_ok = m_ok && m_context;
                m_ok = m_ok && ( 0 == pthread_attr_init( &m_attributes ) );
                m_ok = m_ok && ( 0 == pthread_attr_setdetachstate( &m_attributes, PTHREAD_CREATE_JOINABLE ) );
            }

            ~Thread()
            {
                if( IsRunning() )
                {
                    Kill();
                }
                Join();
                pthread_attr_destroy( &m_attributes );
            }

            ::std::shared_ptr< type > &GetContext()
            {
                return m_context;
            }

            bool IsOk()
            {
                utils::Lock lock( this );
                return m_ok;
            }

            bool Start()
            {
                utils::Lock lock( this );
                m_ok = m_ok && ( 0 == pthread_create( &m_thread, &m_attributes, m_function, ( void * ) &( *m_context ) ) );
                m_started = m_ok;
                return m_ok;
            }

            bool Join()
            {
                utils::Lock lock( this );
                if( m_started )
                {
                    m_ok = m_ok && ( 0 == pthread_join( m_thread, nullptr ) );
                    m_started = false;
                }
                return m_ok;
            }

            bool IsRunning()
            {
                utils::Lock lock( this );
                return ( 0 == pthread_kill( m_thread, 0 ) );
            }

            bool Kill()
            {
                utils::Lock lock( this );
                m_ok = m_ok && ( 0 == pthread_kill( m_thread, SIGKILL ) );
                return m_ok;
            }

            bool Cancel()
            {
                utils::Lock lock( this );
                m_ok = m_ok && ( 0 == pthread_cancel( m_thread ) );
                return m_ok;
            }
    };
}

#endif // _THREAD_HPP_
