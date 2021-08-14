/**
    String.cpp : String class implementation
    Copyright 2014-2019 Daniel Wilson
*/

#include <utils/String.hpp>
#include <utils/Buffer.hpp>
#include <utils/Tokens.hpp>
#include <cstring>
#include <memory>

namespace utils
{
    String::String()
    : m_data     ( nullptr )
    , m_length   ( 0 )
    , m_size     ( 0 )
    , m_blockSize( 128 )
    {
    }

    String::String( const char &a_value )
    : m_data     ( nullptr )
    , m_length   ( 0 )
    , m_size     ( 0 )
    , m_blockSize( 128 )
    {
        m_data = new char[ 2 ];
        if( m_data )
        {
            m_data[ 0 ] = a_value;
            m_data[ 1 ] = 0;
            m_size      = 2;
            m_length    = 1;
        }
    }

    String::String( const char *a_value, uint16_t a_length /*= 0*/ )
    : m_data     ( nullptr )
    , m_length   ( 0 )
    , m_size     ( 0 )
    , m_blockSize( 128 )
    {
        if( ( nullptr != a_value ) && ( 0 == a_length ) )
        {
            size_t length = strlen( a_value );
            if( length <= MaxLength() )
            {
                a_length = static_cast< uint16_t >( length );
            }
        }
        if( a_length > 0 )
        {
            m_size   = a_length + 1;
            m_size   = m_size - ( m_size % m_blockSize );
            while( m_size < ( m_length + 1 ) )
            {
                m_size += m_blockSize;
            }
            m_length = a_length;
            m_data   = new char[ m_size ];
        }
        if( ( nullptr != a_value ) && ( nullptr != m_data ) )
        {
            memmove( m_data, a_value, sizeof( char ) * m_length );
            m_data[ m_length ] = 0;
        }
        else
        {
            m_size   = 0;
            m_length = 0;
        }
    }

    String::String( const String &a_value )
    : m_data     ( nullptr )
    , m_length   ( 0 )
    , m_size     ( 0 )
    , m_blockSize( 128 )
    {
        ::utils::Lock lock( this );
        m_size   = a_value.m_size;
        m_length = a_value.m_length;
        if( ( m_length <= MaxLength() ) && ( m_size <= MaxLength() ) && ( nullptr != a_value.m_data ) )
        {
            m_data = new char[ m_size ];
            if( nullptr != m_data )
            {
                memmove( m_data, a_value.m_data, sizeof( char ) * m_size );
            }
            else
            {
                m_size   = 0;
                m_length = 0;
            }
        }
    }

    String::~String()
    {
        ::utils::Lock lock( this );
        Clear();
    }

    uint8_t String::Type() noexcept
    {
        return static_cast< uint8_t >( SerializableType::String );
    }

    bool String::Serialize( Writable &a_out ) noexcept
    {
        ::utils::Lock lock( this );
        ::utils::Lock valueLock( &a_out );
        bool ok = SerializeType( a_out );
        ok = ok && m_length.Serialize( a_out );
        if( m_length > 0 )
        {
            ok = ok && ( m_length == a_out.Write( ( uint8_t * )m_data , m_length ) );
        }
        return ok;
    }

    bool String::Deserialize( Readable &a_in ) noexcept
    {
        ::utils::Lock lock( this );
        ::utils::Lock valueLock( &a_in );
        bool ok = DeserializeType( a_in );
        ok = ok && m_length.Deserialize( a_in );
        ok = ok && ( m_length <= MaxLength() );
        if( ok && ( m_length > 0 ) )
        {
            m_size = ( ( m_length + 1 ) - ( ( m_length + 1 ) % m_blockSize ) );
            while( m_size < m_length )
            {
                m_size += m_blockSize;
            }
            m_data = new char[ m_size ];
            ok = ( nullptr != m_data );
            ok = ok && ( m_length == a_in.Read( ( uint8_t * )m_data, m_length ) );
            m_data[ m_length ] = 0;
        }
        return ok;
    }

    String::operator bool()
    {
        ::utils::Lock lock( this );
        return ( ( nullptr != m_data ) && ( m_length > 0 ) );
    }

    String::operator const char *()
    {
        ::utils::Lock lock( this );
        return m_data;
    }

    String::operator const uint8_t *()
    {
        ::utils::Lock lock( this );
        return ( const uint8_t * )m_data;
    }

    const char *String::Value()
    {
        ::utils::Lock lock( this );
        return m_data;
    }

    String &String::operator=( const char &a_value )
    {
        ::utils::Lock lock( this );
        if( m_data )
        {
            memset( m_data, 0, m_size );
            delete [] m_data;
            m_data = nullptr;
        }
        m_size   = m_blockSize;
        m_length = 1;
        m_data   = new char[ m_size ];
        if( nullptr != m_data )
        {
            m_data[ 0 ] = a_value;
            m_data[ 1 ] = 0;
        }
        else
        {
            m_size   = 0;
            m_length = 0;
        }
        return *this;
    }

    String &String::operator=( const char *a_value )
    {
        ::utils::Lock lock( this );
        m_size   = 0;
        m_length = 0;
        if( nullptr != m_data )
        {
            memset( m_data, 0, m_size );
            delete [] m_data;
            m_data = nullptr;
        }
        if( nullptr != a_value )
        {
            m_length = strlen( a_value );
        }
        if( ( m_length > 0 ) && ( m_length <= MaxLength() ) )
        {
            m_size   = m_length + 1;
            m_size   = m_size - ( m_size % m_blockSize );
            while( m_size < ( m_length + 1 ) )
            {
                m_size += m_blockSize;
            }
            m_data   = new char[ m_size ];
        }
        if( ( nullptr != a_value ) && ( nullptr != m_data ) )
        {
            memmove( m_data, a_value, sizeof( char ) * m_length );
            m_data[ m_length ] = 0;
        }
        else
        {
            m_size   = 0;
            m_length = 0;
        }
        return *this;
    }

    String &String::operator=( String &a_value )
    {
        ::utils::Lock lock( this );
        ::utils::Lock valueLock( &a_value );
        if( nullptr != m_data )
        {
            memset( m_data, 0, m_size );
            delete [] m_data;
            m_data = nullptr;
        }
        m_size   = a_value.m_size;
        m_length = a_value.m_length;
        if( ( m_length <= MaxLength() ) && ( m_size <= MaxLength() ) )
        {
            m_data   = new char[ m_size ];
            if( ( nullptr != m_data ) && ( nullptr != a_value.m_data ) )
            {
                memmove( m_data, a_value.m_data, m_size );
            }
            else
            {
                m_size   = 0;
                m_length = 0;
                if( nullptr != m_data )
                {
                    memset( m_data, 0, m_size );
                    delete [] m_data;
                    m_data = nullptr;
                }
            }
        }
        return *this;
    }

    String &String::operator+=( const char &a_value )
    {
        ::utils::Lock lock( this );
        if( ( m_size - m_length ) > 1 )
        {
            m_data[ m_length++ ] = a_value;
            m_data[ m_length ]   = 0;
        }
        else if( ( m_size + m_blockSize ) <= MaxLength() )
        {
            m_size += m_blockSize;
            char *temp = new char[ m_size ];
            if( nullptr != temp )
            {
                if( nullptr != m_data )
                {
                    memmove( temp, m_data, ( m_size - m_blockSize ) );
                    delete [] m_data;
                    m_data = nullptr;
                }
                temp[ m_length++ ] = a_value;
                temp[ m_length ]   = 0;
                m_data = temp;
            }
            else
            {
                m_size -= m_blockSize;
            }
        }
        return *this;
    }

    String &String::operator+=( const char *a_value )
    {
        ::utils::Lock lock( this );
        uint32_t i = 0;
        if( nullptr != a_value )
        {
            while( 0 != a_value[ i ] )
            {
                *this += a_value[ i++ ];
            }
        }
        return *this;
    }

    String &String::operator+=( String &a_value )
    {
        ::utils::Lock lock( this );
        ::utils::Lock valueLock( &a_value );
        if( a_value )
        {
            for( uint16_t i = 0; i < a_value.m_length; ++i )
            {
                *this += a_value.m_data[ i ];
            }
        }
        return *this;
    }

    bool String::operator==( const char &a_value )
    {
        ::utils::Lock lock( this );
        return ( m_data && ( m_length == 1 ) && ( m_data[ 0 ] == a_value ) );
    }

    bool String::operator==( const char *a_value )
    {
        ::utils::Lock lock( this );
        uint32_t length  = 0;
        if( nullptr == m_data )
        {
            return ( nullptr == a_value );
        }
        if( nullptr != a_value )
        {
            length = strlen( a_value );
        }
        if( length == m_length )
        {
            return ( 0 == strncmp( m_data, a_value, length ) );
        }
        return false;
    }

    bool String::operator==( String &a_value )
    {
        ::utils::Lock valueLock( &a_value );
        uint32_t length = 0;
        if( nullptr == m_data )
        {
            return ( nullptr == a_value.m_data );
        }
        if( nullptr != a_value.m_data )
        {
            length = a_value.m_length;
        }
        if( length == m_length )
        {
            return ( 0 == strncmp( m_data, a_value.m_data, length ) );
        }
        return false;
    }

    bool String::operator!=( const char &a_value )
    {
        return !( *this == a_value );
    }

    bool String::operator!=( const char *a_value )
    {
        return !( *this == a_value );
    }

    bool String::operator!=( String &a_value )
    {
        ::utils::Lock lock( this );
        ::utils::Lock valueLock( &a_value );
        return !( *this == a_value );
    }

    char &String::operator[]( uint16_t a_index )
    {
        ::utils::Lock lock( this );
        return m_data[ a_index % m_length ];
    }

    bool String::Contains( const char *a_value, uint16_t a_length /*= 0*/ )
    {
        ::utils::Lock lock( this );
        if( ( nullptr != a_value ) && ( 0 == a_length ) )
        {
            a_length = strlen( a_value );
        }
        if( ( nullptr == m_data  ) || ( 0 == m_length ) ||
            ( nullptr == a_value ) || ( 0 == a_length ) ||
            ( a_length > MaxLength() ) )
        {
            return false;
        }
        uint16_t length = m_length;
        char *value  = m_data;
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

    uint16_t String::Size()
    {
        ::utils::Lock lock( this );
        return m_size;
    }

    uint16_t String::Length()
    {
        ::utils::Lock lock( this );
        return m_length;
    }

    bool String::SetValue( const char *a_value, uint16_t a_length /*= 0*/ )
    {
        ::utils::Lock lock( this );
        m_size   = 0;
        m_length = 0;
        if( nullptr != m_data )
        {
            memset( m_data, 0, m_size );
            delete [] m_data;
            m_data = nullptr;
        }
        if( ( nullptr != a_value ) && ( 0 == a_length ) )
        {
            a_length = strlen( a_value );
        }
        if( ( a_length > 0 ) && ( a_length <= MaxLength() ) )
        {
            m_size   = a_length + 1;
            m_size   = m_size - ( m_size % m_blockSize );
            while( m_size < ( m_length + 1 ) )
            {
                m_size += m_blockSize;
            }
            m_length = a_length;
            m_data   = new char[ m_size ];
        }
        if( ( nullptr != a_value ) && ( nullptr != m_data ) )
        {
            memmove( m_data, a_value, sizeof( char ) * m_length );
            m_data[ m_length ] = 0;
        }
        else
        {
            m_size   = 0;
            m_length = 0;
            if( nullptr != m_data )
            {
                memset( m_data, 0, m_size );
                delete [] m_data;
                m_data = nullptr;
            }
        }
        return ( nullptr != m_data );
    }

    void String::Clear()
    {
        ::utils::Lock lock( this );
        if( m_data )
        {
            memset( m_data, 0, m_size );
            delete [] m_data;
            m_data = nullptr;
        }
        m_size   = 0;
        m_length = 0;
    }

    String &String::ToLower()
    {
        ::utils::Lock lock( this );
        for( uint32_t i = 0; i < m_length; i++ )
        {
            if( ( m_data[ i ] >= 'A' ) && ( m_data[ i ] <= 'Z' ) )
            {
                m_data[ i ] += ( 'a' - 'A' );
            }
        }
        return *this;
    }

    String &String::ToUpper()
    {
        ::utils::Lock lock( this );
        for( uint32_t i = 0; i < m_length; i++ )
        {
            if( ( m_data[ i ] >= 'a' ) && ( m_data[ i ] <= 'z' ) )
            {
                m_data[ i ] -= ( 'a' - 'A' );
            }
        }
        return *this;
    }

    int64_t String::ToInt()
    {
        ::utils::Lock lock( this );
        bool negative = false;
        String token;
        ::std::shared_ptr< Buffer > buffer = ::std::make_shared< Buffer >( m_length );
        if( buffer )
        {
            ( *buffer ).Write( ( uint8_t * )m_data, m_length );
            TokenType type = Tokens::GetToken( *buffer, token );
            if( type == TokenTypes::Symbol )
            {
                negative = ( token == '-' );
                type = Tokens::GetToken( *buffer, token );
            }
            if( type == TokenTypes::Number )
            {
                return token.ToUint() * ( ( negative )? -1: 1 );
            }
        }
        return 0;
    }

    uint64_t String::ToUint()
    {
        ::utils::Lock lock( this );
        uint64_t temp = 0;
        String token;
        ::std::shared_ptr< Buffer > buffer = ::std::make_shared< Buffer >( m_length );
        if( buffer )
        {
            ( *buffer ).Write( ( uint8_t * )m_data, m_length );
            TokenType type = Tokens::GetToken( *buffer, token );
            if( type == TokenTypes::Symbol )
            {
                if( token == '+' )
                {
                    type = Tokens::GetToken( *buffer, token );
                }
            }
            if( type == TokenTypes::Number )
            {
                for( uint32_t i = 0; i < token.Length(); i++ )
                {
                    temp *= 10;
                    temp += ( ( token[ i ] - '0' ) % 10 );
                }
            }
        }
        return temp;
    }

    String &String::Trim( const char &a_value /*= ' '*/ )
    {
        ::utils::Lock lock( this );
        while( ( m_length > 0 ) && ( m_data[ 0 ] == a_value ) )
        {
            // This needs to be <= to copy the nullptr terminator
            for( uint32_t i = 1; i <= m_length; i++ )
            {
                m_data[ i - 1 ] = m_data[ i ];
            }
            m_length--;
        }
        while( ( m_length > 0 ) && ( m_data[ m_length - 1 ] == a_value ) )
        {
            m_data[ m_length - 1 ] = 0;
            m_length--;
        }
        return *this;
    }

    uint16_t String::MaxLength()
    {
        return 0x2000; // 8k bytes
    }
}
