/**
    Primitive.hpp : Primitive definition
    Copyright 2015-2019 Daniel Wilson
*/

#pragma once

#ifndef _PRIMITIVE_HPP_
#define _PRIMITIVE_HPP_

#include <utils/Serializable.hpp>

namespace utils
{
    template< typename type >
    class Primitive : public Serializable
    {
        private:
            type m_value;

        public:
            Primitive()
            : m_value( 0 )
            {
            }

            Primitive( const type &a_value )
            : m_value( a_value )
            {
            }

            ~Primitive()
            {
                m_value = 0;
            }

            operator type()
            {
                ::utils::Lock lock( this );
                return m_value;
            }

            type operator =( const type &a_value )
            {
                ::utils::Lock lock( this );
                return ( m_value = a_value );
            }

            type operator =( const Primitive< type > &a_value )
            {
                ::utils::Lock lock( this );
                return ( m_value = a_value.m_value );
            }

            type operator &=( const type &a_value )
            {
                ::utils::Lock lock( this );
                return ( m_value &= a_value );
            }

            type operator &=( const Primitive< type > &a_value )
            {
                ::utils::Lock lock( this );
                return ( m_value &= a_value.m_value );
            }

            type operator |=( const type &a_value )
            {
                ::utils::Lock lock( this );
                return ( m_value |= a_value );
            }

            type operator |=( const Primitive< type > &a_value )
            {
                ::utils::Lock lock( this );
                return ( m_value |= a_value.m_value );
            }

            type operator ++()
            {
                ::utils::Lock lock( this );
                return ( ++m_value );
            }

            type operator ++( int )
            {
                ::utils::Lock lock( this );
                return ( m_value++ );
            }

            type operator +=( const type &a_value )
            {
                ::utils::Lock lock( this );
                return ( m_value += a_value );
            }

            type operator +=( const Primitive< type > &a_value )
            {
                ::utils::Lock lock( this );
                return ( m_value += a_value.m_value );
            }

            type operator --()
            {
                ::utils::Lock lock( this );
                return ( --m_value );
            }

            type operator --( int )
            {
                ::utils::Lock lock( this );
                return ( m_value-- );
            }

            type operator -=( const type &a_value )
            {
                ::utils::Lock lock( this );
                return ( m_value -= a_value );
            }

            type operator -=( const Primitive< type > &a_value )
            {
                ::utils::Lock lock( this );
                return ( m_value -= a_value.m_value );
            }

            type operator %=( const type &a_value )
            {
                ::utils::Lock lock( this );
                return ( m_value %= a_value );
            }

            type operator %=( const Primitive< type > &a_value )
            {
                ::utils::Lock lock( this );
                return ( m_value %= a_value.m_value );
            }

            // Serializable functions
            uint8_t Type() noexcept final
            {
                return static_cast< uint8_t >( SerializableType::Primitive );
            }

            bool Serialize( Writable &a_out ) noexcept final
            {
                ::utils::Lock lock( this );
                ::utils::Lock valueLock( &a_out );
                bool ok = SerializeType( a_out );
                uint8_t valueSize = sizeof( m_value );
                ok = ok && ( sizeof( valueSize ) == a_out.Write( valueSize ) );
                type value = m_value;
                if( sizeof( value ) > 1 )
                {
                    value = ToNetwork( m_value );
                }
                ok = ok && ( sizeof( value ) == a_out.Write( ( const uint8_t * )&value, sizeof( value ) ) );
                return ok;
            }

            bool Deserialize( Readable &a_in ) noexcept final
            {
                ::utils::Lock lock( this );
                ::utils::Lock valueLock( &a_in );
                bool ok = DeserializeType( a_in );
                uint8_t valueSize = 0;
                ok = ok && ( sizeof( valueSize ) == a_in.Read( valueSize ) );
                ok = ok && ( sizeof( m_value ) == valueSize );
                ok = ok && ( sizeof( m_value ) == a_in.Read( ( uint8_t * )&m_value, sizeof( m_value ) ) );
                if( ok && ( sizeof( m_value ) > 1 ) )
                {
                    m_value = FromNetwork( m_value );
                }
                return ok;
            }
    };
}

#endif // _PRIMITIVE_HPP_
