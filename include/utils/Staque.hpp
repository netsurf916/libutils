/**
    Staque.cpp : Staque class implementation
    Copyright 2008-2019 Daniel Wilson
*/

#pragma once

#ifndef _STAQUE_HPP_
#define _STAQUE_HPP_

#include <utils/Types.hpp>
#include <utils/Lock.hpp>
#include <memory>

namespace utils
{
    template< typename type >
    class StaqueElement : public Lockable
    {
        private:
            ::std::shared_ptr< type >                  m_value;
            ::std::shared_ptr< StaqueElement< type > > m_next;
            ::std::shared_ptr< StaqueElement< type > > m_previous;

        public:
            StaqueElement()
            : m_value()
            , m_next()
            , m_previous()
            {}

            StaqueElement( const type &a_value )
            : m_value( ::std::make_shared< type >( a_value ) )
            , m_next()
            , m_previous()
            {}

            ::std::shared_ptr< type > &Value()
            {
                return m_value;
            }

            ::std::shared_ptr< StaqueElement< type > > &Next()
            {
                return m_next;
            }

            ::std::shared_ptr< StaqueElement< type > > &Previous()
            {
                return m_previous;
            }
    };

    template< typename type >
    class Staque : public Lockable
    {
        private:
            ::std::shared_ptr< StaqueElement< type > > m_start;
            ::std::shared_ptr< StaqueElement< type > > m_end;
            uint32_t                                   m_length;

        public:
            Staque()
            : m_length( 0 )
            {
            }

            ~Staque()
            {
                ::utils::Lock lock( this );
            }

            const uint32_t &Length()
            {
                ::utils::Lock lock( this );
                return m_length;
            }

            bool Push( type &a_value )
            {
                ::utils::Lock lock( this );
                ::std::shared_ptr< StaqueElement< type > > temp =
                    ::std::make_shared< StaqueElement< type > >( a_value );
                bool ok = temp && temp->Value();
                if( ok && !m_start )
                {
                    m_start  = temp;
                    m_end    = temp;
                    m_length = 1;
                }
                else if( ok )
                {
                    m_start->Previous()         = temp;
                    m_start->Previous()->Next() = m_start;
                    m_start                     = m_start->Previous();
                    ++m_length;
                }
                return ok;
            }

            bool Pop( type &a_value )
            {
                ::utils::Lock lock( this );
                bool ok = m_start && m_start->Value();
                if( ok )
                {
                    a_value = *( m_start->Value() );
                    m_start = m_start->Next();
                    if( m_start )
                    {
                        m_start->Previous().reset();
                    }
                    else
                    {
                        m_end = m_start;
                    }
                    --m_length;
                }
                return ok;
            }

            bool Enqueue( type &a_value )
            {
                ::utils::Lock lock( this );
                ::std::shared_ptr< StaqueElement< type > > temp =
                    ::std::make_shared< StaqueElement< type > >( a_value );
                bool ok = temp && temp->Value();
                if( ok && !m_end )
                {
                    m_start  = temp;
                    m_end    = temp;
                    m_length = 1;
                }
                else if( ok )
                {
                    m_end->Next()             = temp;
                    m_end->Next()->Previous() = m_end;
                    m_end                     = m_end->Next();
                    ++m_length;
                }
                return ok;
            }

            bool Dequeue( type &a_value )
            {
                return Pop( a_value );
            }

            bool Peek( type &a_value )
            {
                ::utils::Lock lock( this );
                bool ok = m_start && m_start->Value();
                if( ok )
                {
                    a_value = *( m_start->Value() );
                }
                return ok;
            }

            void Clear()
            {
                m_start.reset();
                m_end.reset();
            }

            bool GetAt( uint32_t a_index, type &a_value )
            {
                ::utils::Lock lock( this );
                bool ok = ( a_index < m_length );
                ::std::shared_ptr< StaqueElement< type > > start = m_start;
                while( ok && start && ( a_index > 0 ) )
                {
                    start = start->Next();
                    --a_index;
                }
                ok = ok && ( 0 == a_index ) && start && start->Value();
                if( ok )
                {
                    a_value = *( start->Value() );
                }
                return ok;
            }
    };
}

#endif // _STAQUE_HPP_
