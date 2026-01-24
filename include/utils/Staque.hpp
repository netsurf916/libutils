/**
    Description: Stack/queue hybrid container.
    Staque.cpp : Staque class implementation
    Copyright 2008-2026 Daniel Wilson
    SPDX-License-Identifier: MIT
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
    /**
     * @brief Node for Staque linked list.
     * @details Holds a shared pointer to a value and links to neighbors.
     */
    class StaqueElement : public Lockable
    {
        private:
            ::std::shared_ptr< type >                  m_value;
            ::std::shared_ptr< StaqueElement< type > > m_next;
            ::std::shared_ptr< StaqueElement< type > > m_previous;

        public:
            /**
             * @brief Construct an empty node with no value.
             */
            StaqueElement()
            : m_value()
            , m_next()
            , m_previous()
            {}

            /**
             * @brief Construct a node holding a copy of a value.
             * @param a_value Value to copy into the node.
             */
            StaqueElement( const type &a_value )
            : m_value( ::std::make_shared< type >( a_value ) )
            , m_next()
            , m_previous()
            {}

            /**
             * @brief Access the stored value.
             * @return Shared pointer to the value; may be null if unset.
             */
            ::std::shared_ptr< type > &Value()
            {
                return m_value;
            }

            /**
             * @brief Access the next node.
             * @return Shared pointer to the next node.
             */
            ::std::shared_ptr< StaqueElement< type > > &Next()
            {
                return m_next;
            }

            /**
             * @brief Access the previous node.
             * @return Shared pointer to the previous node.
             */
            ::std::shared_ptr< StaqueElement< type > > &Previous()
            {
                return m_previous;
            }
    };

    template< typename type >
    /**
     * @brief Stack/queue hybrid container.
     * @details Provides push/pop (stack) and enqueue/dequeue (queue) operations.
     * @note Not safe for concurrent access without external synchronization.
     */
    class Staque : public Lockable
    {
        private:
            ::std::shared_ptr< StaqueElement< type > > m_start;
            ::std::shared_ptr< StaqueElement< type > > m_end;
            uint32_t                                   m_length;

        public:
            /**
             * @brief Construct an empty container.
             */
            Staque()
            : m_length( 0 )
            {
            }

            /**
             * @brief Destroy the container and release nodes.
             */
            ~Staque()
            {
                ::utils::Lock lock( this );
            }

            /**
             * @brief Get the current number of elements.
             * @return Reference to the length count.
             */
            const uint32_t &Length()
            {
                ::utils::Lock lock( this );
                return m_length;
            }

            /**
             * @brief Push a value onto the front (stack behavior).
             * @param a_value Value to copy into the container.
             * @return True if the value was added; false on allocation failure.
             */
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

            /**
             * @brief Pop a value from the front (stack behavior).
             * @param a_value Output value to receive the removed element.
             * @return True if an element was removed; false if empty.
             */
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

            /**
             * @brief Enqueue a value to the back (queue behavior).
             * @param a_value Value to copy into the container.
             * @return True if the value was added; false on allocation failure.
             */
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

            /**
             * @brief Dequeue a value from the front (queue behavior).
             * @param a_value Output value to receive the removed element.
             * @return True if an element was removed; false if empty.
             */
            bool Dequeue( type &a_value )
            {
                return Pop( a_value );
            }

            /**
             * @brief Peek at the front element without removing it.
             * @param a_value Output value to receive the element.
             * @return True if an element is available; false if empty.
             */
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

            /**
             * @brief Clear all elements from the container.
             * @note Does not change the allocated nodes beyond releasing references.
             */
            void Clear()
            {
                m_start.reset();
                m_end.reset();
            }

            /**
             * @brief Retrieve a value at a specific index.
             * @param a_index Zero-based index from the front.
             * @param a_value Output value to receive the element.
             * @return True if the index is valid and value retrieved; false otherwise.
             */
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
