/**
    Buffer.cpp : Buffer class implementation
    Copyright 2015-2019 Daniel Wilson
*/

#include <utils/Buffer.hpp>
#include <sys/stat.h>
#include <cstring>
#include <stdio.h>

namespace utils
{
    Buffer::Buffer( uint32_t a_size /*= 65536*/ )
    : m_size( a_size )
    {
        m_buffer = new uint8_t[ m_size ];
        m_start  = m_buffer;
        m_end    = m_buffer;
        if( nullptr == m_buffer )
        {
            m_size = 0;
        }
        else
        {
            memset( m_buffer, 0, m_size );
        }
    }

    Buffer::Buffer( Buffer &a_value )
    : m_size( a_value.m_size )
    {
        ::utils::Lock valueLock( &a_value );
        m_buffer = new uint8_t[ m_size ];
        if( ( nullptr != m_buffer ) && ( nullptr != a_value.m_buffer ) )
        {
            memmove( m_buffer, a_value.m_buffer, m_size );
            m_start = m_buffer + ( a_value.m_start - a_value.m_buffer );
            m_end   = m_buffer + ( a_value.m_end   - a_value.m_buffer );
        }
        else
        {
            if( nullptr != m_buffer )
            {
                memset( m_buffer, 0, m_size );
                delete [] m_buffer;
                m_buffer = nullptr;
            }
            m_start = nullptr;
            m_end   = nullptr;
            m_size  = 0;
        }
    }

    Buffer::~Buffer()
    {
        ::utils::Lock lock( this );
        if( nullptr != m_buffer )
        {
            memset( m_buffer, 0, m_size );
            delete [] m_buffer;
            m_buffer = nullptr;
            m_start  = nullptr;
            m_end    = nullptr;
            m_size   =    0;
        }
    }

    bool Buffer::IsReadable() noexcept
    {
        ::utils::Lock lock( this );
        return ( nullptr != m_start );
    }

    bool Buffer::Read( uint8_t &a_value, bool a_block /*= false*/ ) noexcept
    {
        UNUSED( a_block );
        ::utils::Lock lock( this );
        if( ( nullptr == m_start ) || ( m_start >= m_end ) )
        {
            return false;
        }
        a_value = *m_start;
        ++m_start;
        if( m_start >= m_end )
        {
            m_start = m_buffer;
            m_end   = m_buffer;
        }
        return true;
    }

    uint32_t Buffer::Read( uint8_t *a_value, uint32_t a_length, bool a_block /*= false*/ ) noexcept
    {
        UNUSED( a_block );
        ::utils::Lock lock( this );
        if( ( nullptr == m_start ) || ( m_start >= m_end ) || ( 0 == a_length ) )
        {
            return 0;
        }
        if( ( m_end - m_start ) < a_length )
        {
            a_length = ( m_end - m_start );
        }
        memmove( a_value, m_start, a_length );
        m_start += a_length;
        if( m_start >= m_end )
        {
            m_start = m_buffer;
            m_end   = m_buffer;
        }
        return a_length;
    }

    bool Buffer::Peek( uint8_t &a_value ) noexcept
    {
        return Peek( a_value, 0 );
    }

    bool Buffer::Peek( uint8_t &a_value, uint32_t a_index ) noexcept
    {
        ::utils::Lock lock( this );
        if( ( nullptr == m_start ) || ( m_start >= m_end ) || ( ( m_start + a_index ) >= m_end ) )
        {
            return false;
        }
        a_value = *( m_start + a_index );
        return true;
    }

    uint32_t Buffer::Peek( uint8_t *a_value, uint32_t a_length ) noexcept
    {
        ::utils::Lock lock( this );
        if( ( nullptr == m_start ) || ( m_start >= m_end ) || ( 0 == a_length ) )
        {
            return 0;
        }
        if( ( m_end - m_start ) < a_length )
        {
            a_length = ( m_end - m_start );
        }
        memmove( a_value, m_start, a_length );
        return a_length;
    }

    bool Buffer::IsWritable() noexcept
    {
        ::utils::Lock lock( this );
        return ( nullptr != m_end );
    }

    bool Buffer::Write( const uint8_t &a_value ) noexcept
    {
        ::utils::Lock lock( this );
        if( ( nullptr == m_end ) )
        {
            return false;
        }
        Defragment();
        if( ( ( m_end - m_start ) / sizeof( uint8_t ) ) >= m_size )
        {
            return false;
        }
        *m_end = a_value;
        ++m_end;
        return true;
    }

    uint32_t Buffer::Write( const uint8_t *a_value, uint32_t a_length /*= 0*/ ) noexcept
    {
        ::utils::Lock lock( this );
        if( nullptr == m_end )
        {
            return false;
        }
        if( ( nullptr != a_value ) && ( 0 == a_length ) )
        {
            while( 0 != a_value[ a_length ] )
            {
                ++a_length;
            }
        }
        if( 0 == a_length )
        {
            return false;
        }
        Defragment();
        if( ( m_size - ( m_end - m_start ) ) < a_length )
        {
            a_length = ( m_size - ( m_end - m_start ) );
        }
        if( a_length > 0 )
        {
            memmove( m_end, a_value, a_length );
            m_end += a_length;
        }
        return a_length;
    }

    const uint8_t *Buffer::Value()
    {
        ::utils::Lock lock( this );
        if( m_start >= m_end )
        {
            return nullptr;
        }
        return m_start;
    }

    Buffer::operator const void *()
    {
        ::utils::Lock lock( this );
        if( m_start >= m_end )
        {
            return nullptr;
        }
        return static_cast< const void * >( m_start );
    }

    Buffer::operator const uint8_t *()
    {
        ::utils::Lock lock( this );
        if( m_start >= m_end )
        {
            return nullptr;
        }
        return m_start;
    }

    uint32_t Buffer::Size()
    {
        ::utils::Lock lock( this );
        return m_size;
    }

    uint32_t Buffer::Length()
    {
        ::utils::Lock lock( this );
        if( m_start >= m_end )
        {
            return 0;
        }
        return ( m_end - m_start );
    }

    uint32_t Buffer::Space()
    {
        ::utils::Lock lock( this );
        if( ( m_start > m_end ) ||
            ( ( m_end - m_start ) >= m_size ) )
        {
            return 0;
        }
        return ( m_size - ( m_end - m_start ) );
    }

    void Buffer::Clear()
    {
        ::utils::Lock lock( this );
        m_start = m_buffer;
        m_end   = m_buffer;
        memset( m_buffer, 0, m_size );
    }

    void Buffer::TrimLeft( uint32_t a_count /*= 0xFFFFFFFF*/ )
    {
        ::utils::Lock lock( this );
        if( a_count > ( m_end - m_start ) )
        {
            m_start = m_buffer;
            m_end   = m_buffer;
        }
        else
        {
            m_start += a_count;
            if( m_start >= m_end )
            {
                m_start = m_buffer;
                m_end   = m_buffer;
            }
        }
    }

    void Buffer::TrimRight( uint32_t a_count /*= 0xFFFFFFFF*/ )
    {
        ::utils::Lock lock( this );
        if( a_count > ( m_end - m_start ) )
        {
            m_start = m_buffer;
            m_end   = m_buffer;
        }
        else
        {
            m_end -= a_count;
            if( m_start >= m_end )
            {
                m_start = m_buffer;
                m_end   = m_buffer;
            }
        }
    }

    void Buffer::Defragment()
    {
        ::utils::Lock lock( this );
        if( ( m_start > m_buffer ) && ( m_start != m_end ) )
        {
            memmove( m_buffer, m_start, ( m_end - m_start ) );
            m_end   -= ( m_start - m_buffer );
            m_start  = m_buffer;
        }
    }

    bool Buffer::Contains( const char *a_value, uint32_t a_length /*= 0*/ )
    {
        return Contains( reinterpret_cast< const uint8_t * >( a_value ), a_length );
    }

    bool Buffer::Contains( const uint8_t *a_value, uint32_t a_length /*= 0*/ )
    {
        ::utils::Lock lock( this );
        if( ( nullptr != a_value ) && ( 0 == a_length ) )
        {
            while( 0 != a_value[ a_length ] )
            {
                ++a_length;
            }
        }
        if( ( nullptr == m_start ) || ( m_start == m_end ) ||
            ( nullptr == a_value ) || ( 0 == a_length ) )
        {
            return false;
        }
        uint32_t  length = ( m_end - m_start );
        uint8_t  *value  = m_start;
        while( length >= a_length )
        {
            if( 0 == memcmp( value, a_value, a_length) )
            {
                return true;
            }
            --length;
            ++value;
        }
        return false;
    }
}
