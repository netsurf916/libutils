/**
    BitMask.cpp : BitMask class implementation
    Copyright 2015-2019 Daniel Wilson
*/

#include <utils/BitMask.hpp>

namespace utils
{
    BitMask::BitMask()
    : m_bitMask( 0 )
    {
    }

    BitMask::BitMask( uint32_t a_value )
    : m_bitMask( a_value )
    {
    }

    BitMask::~BitMask()
    {
        m_bitMask = 0;
    }

    BitMask::operator uint32_t()
    {
        ::utils::Lock lock( this );
        return m_bitMask;
    }

    BitMask &BitMask::operator =( uint32_t a_bitMask )
    {
        ::utils::Lock lock( this );
        m_bitMask = a_bitMask;
        return *this;
    }

    BitMask &BitMask::operator =( BitMask &a_bitMask )
    {
        ::utils::Lock lock( this );
        ::utils::Lock valueLock( &a_bitMask );
        m_bitMask = static_cast< uint32_t >( a_bitMask );
        return *this;
    }

    bool BitMask::operator []( uint8_t a_bit )
    {
        if( a_bit >= ( sizeof( uint32_t ) * 8 ) )
        {
            return false;
        }
        return ( ( m_bitMask & ( uint32_t )BIT( a_bit ) ) == ( uint32_t )BIT( a_bit ) );
    }

    bool BitMask::SetBit( uint8_t a_bit, bool a_set )
    {
        ::utils::Lock lock( this );
        if( a_bit >= ( sizeof( uint32_t ) * 8 ) )
        {
            return false;
        }
        if( a_set )
        {
            m_bitMask |= BIT( a_bit );
        }
        else
        {
            m_bitMask &= ~BIT( a_bit );
        }
        return true;
    }

    bool BitMask::GetBit( uint8_t a_bit, bool &a_set )
    {
        ::utils::Lock lock( this );
        if( a_bit >= ( sizeof( uint32_t ) * 8 ) )
        {
            return false;
        }
        a_set = ( ( m_bitMask & ( uint32_t )BIT( a_bit ) ) == ( uint32_t )BIT( a_bit ) );
        return true;
    }

    bool BitMask::IsSet( uint8_t a_bit )
    {
        if( a_bit >= ( sizeof( uint32_t ) * 8 ) )
        {
            return false;
        }
        return ( ( m_bitMask & ( uint32_t )BIT( a_bit ) ) == ( uint32_t )BIT( a_bit ) );
    }

    uint8_t BitMask::Type() noexcept
    {
        return static_cast< uint8_t >( SerializableType::BitMask );
    }

    bool BitMask::Serialize( Writable &a_out ) noexcept
    {
        ::utils::Lock lock( this );
        ::utils::Lock valueLock( &a_out );
        bool ok = SerializeType( a_out );
        ok = ok && m_bitMask.Serialize( a_out );
        return ok;
    }

    bool BitMask::Deserialize( Readable &a_in  ) noexcept
    {
        ::utils::Lock lock( this );
        ::utils::Lock valueLock( &a_in );
        bool ok = DeserializeType( a_in );
        ok = ok && m_bitMask.Deserialize( a_in );
        return ok;
    }
}