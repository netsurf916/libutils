/**
    HttpAccess.cpp : HttpAccess implementation
    Copyright 2026 Daniel Wilson
    Created with the help of ChatGPT.
    SPDX-License-Identifier: MIT
*/

#include <utils/HttpAccess.hpp>
#include <utils/Buffer.hpp>
#include <utils/File.hpp>
#include <utils/HttpRequest.hpp>
#include <utils/IniFile.hpp>
#include <utils/Lock.hpp>
#include <utils/Socket.hpp>
#include <utils/Tokens.hpp>
#include <crypt.h>
#include <string.h>
#include <vector>

#define MAXBUFFERLEN 4096

namespace utils
{
    HttpAccess::HttpAccess()
    : m_enabled( false )
    , m_loaded( false )
    {
    }

    bool HttpAccess::Configure( IniFile &a_ini )
    {
        utils::Lock lock( this );
        if( !a_ini.ReadValue( "settings", "access", m_file ) || ( m_file.length() == 0 ) )
        {
            m_entries.clear();
            m_file.clear();
            m_fileHandle.reset();
            m_enabled = false;
            m_loaded  = false;
            m_realm.clear();
            return false;
        }
        m_fileHandle = ::std::make_shared< File >( m_file.c_str(), FileMode::DefaultRead );
        m_enabled = true;
        m_loaded  = false;
        m_realm = "Restricted";
        a_ini.ReadValue( "settings", "realm", m_realm );
        return true;
    }

    bool HttpAccess::Enabled() const
    {
        return m_enabled;
    }

    bool HttpAccess::IsAuthorized( HttpRequest &a_request )
    {
        utils::Lock lock( this );
        if( !m_enabled )
        {
            return true;
        }
        if( !RefreshIfNeeded() )
        {
            return false;
        }

        ::std::string header;
        if( !a_request.HeaderValue( "authorization", header ) )
        {
            return false;
        }
        Tokens::TrimSpace( header );
        ::std::string scheme;
        ::std::string encoded;
        auto space = header.find( ' ' );
        if( space == ::std::string::npos )
        {
            return false;
        }
        scheme  = header.substr( 0, space );
        encoded = header.substr( space + 1 );
        Tokens::TrimSpace( scheme );
        Tokens::TrimSpace( encoded );
        Tokens::MakeUpper( scheme );
        if( scheme != "BASIC" )
        {
            return false;
        }

        ::std::string decoded;
        if( !DecodeBase64( encoded, decoded ) )
        {
            return false;
        }
        auto colon = decoded.find( ':' );
        if( colon == ::std::string::npos )
        {
            return false;
        }
        ::std::string user = decoded.substr( 0, colon );
        ::std::string pass = decoded.substr( colon + 1 );
        return CheckCredentials( user, pass );
    }

    int32_t HttpAccess::RespondUnauthorized( ::std::shared_ptr< Socket > &a_socket )
    {
        utils::Lock lock( this );
        if( !a_socket || !( a_socket->Valid() ) )
        {
            return -1;
        }
        utils::Lock valueLock( a_socket.get() );

        ::std::string realm = m_realm;
        if( realm.length() == 0 )
        {
            realm = "Restricted";
        }

        auto sendb = ::std::make_shared< Buffer >( MAXBUFFERLEN );
        if( !sendb )
        {
            return -1;
        }
        sendb->Write( ( const uint8_t * )"HTTP/1.1 401 UNAUTHORIZED\r\n" );
        sendb->Write( ( const uint8_t * )"WWW-Authenticate: Basic realm=\"" );
        sendb->Write( ( const uint8_t * )realm.c_str(), realm.length() );
        sendb->Write( ( const uint8_t * )"\"\r\n" );
        sendb->Write( ( const uint8_t * )"Connection: Close\r\n" );
        sendb->Write( ( const uint8_t * )"Content-Length: 0\r\n\r\n" );
        while( sendb->Length() && a_socket->Valid() )
        {
            a_socket->Write( sendb );
        }
        return 401;
    }

    bool HttpAccess::RefreshIfNeeded()
    {
        if( m_file.length() == 0 )
        {
            return false;
        }
        if( !m_fileHandle || !m_fileHandle->Exists() )
        {
            return false;
        }
        if( m_loaded && !m_fileHandle->IsModified() )
        {
            return true;
        }
        if( !LoadEntries() )
        {
            return false;
        }
        return true;
    }

    bool HttpAccess::LoadEntries()
    {
        if( !m_fileHandle || !m_fileHandle->Exists() )
        {
            return false;
        }
        m_entries.clear();
        auto buffer = ::std::make_shared< Buffer >( MAXBUFFERLEN );
        if( !buffer )
        {
            return false;
        }
        m_fileHandle->Seek( 0 );
        while( TokenTypes::Line == Tokens::GetLine( *m_fileHandle, *buffer ) )
        {
            ::std::string line;
            Tokens::GetLine( *buffer, line );
            buffer->Clear();
            Tokens::TrimSpace( line );
            if( line.length() == 0 )
            {
                continue;
            }
            if( ( line[ 0 ] == '#' ) || ( line[ 0 ] == ';' ) )
            {
                continue;
            }
            ParseEntry( line );
        }
        m_fileHandle->Close();
        m_loaded  = true;
        return true;
    }

    bool HttpAccess::ParseEntry( const ::std::string &a_line )
    {
        auto split = a_line.find( ':' );
        if( split == ::std::string::npos )
        {
            return false;
        }
        ::std::string user = a_line.substr( 0, split );
        ::std::string pass = a_line.substr( split + 1 );
        Tokens::TrimSpace( user );
        Tokens::TrimSpace( pass );
        if( user.length() == 0 )
        {
            return false;
        }
        Entry entry;
        entry.user = user;
        entry.pass = pass;
        m_entries.push_back( entry );
        return true;
    }

    bool HttpAccess::CheckCredentials( const ::std::string &a_user, const ::std::string &a_pass )
    {
        auto startsWithUpper = []( const ::std::string &a_value, const ::std::string &a_prefix )
        {
            if( a_value.length() < a_prefix.length() )
            {
                return false;
            }
            for( size_t i = 0; i < a_prefix.length(); ++i )
            {
                char valueChar = a_value[ i ];
                if( ( valueChar >= 'a' ) && ( valueChar <= 'z' ) )
                {
                    valueChar = static_cast< char >( valueChar - 'a' + 'A' );
                }
                if( valueChar != a_prefix[ i ] )
                {
                    return false;
                }
            }
            return true;
        };
        auto trimPadding = []( ::std::string a_value )
        {
            while( ( a_value.length() > 0 ) && ( a_value[ a_value.length() - 1 ] == '=' ) )
            {
                a_value.pop_back();
            }
            return a_value;
        };

        for( const auto &entry : m_entries )
        {
            if( entry.user != a_user )
            {
                continue;
            }
            const ::std::string shaPrefix = "{SHA}";
            if( startsWithUpper( entry.pass, shaPrefix ) )
            {
                uint8_t digest[ 20 ];
                Sha1( reinterpret_cast< const uint8_t * >( a_pass.c_str() ), a_pass.length(), digest );
                ::std::string encodedString = Base64Encode( digest, sizeof( digest ) );
                ::std::string stored = entry.pass.substr( shaPrefix.length() );
                if( stored == encodedString )
                {
                    return true;
                }
                if( trimPadding( stored ) == trimPadding( encodedString ) )
                {
                    return true;
                }
                continue;
            }
            const ::std::string plainPrefix = "{PLAIN}";
            if( startsWithUpper( entry.pass, plainPrefix ) )
            {
                if( entry.pass.substr( plainPrefix.length() ) == a_pass )
                {
                    return true;
                }
                continue;
            }
            if( entry.pass == a_pass )
            {
                return true;
            }
            if( entry.pass.length() > 0 )
            {
                const char *hashed = crypt( a_pass.c_str(), entry.pass.c_str() );
                if( hashed && ( strcmp( hashed, entry.pass.c_str() ) == 0 ) )
                {
                    return true;
                }
            }
        }
        return false;
    }

    bool HttpAccess::DecodeBase64( const ::std::string &a_input, ::std::string &a_output ) const
    {
        a_output.clear();
        int value = 0;
        int bits  = -8;

        for( size_t i = 0; i < a_input.length(); ++i )
        {
            char c = a_input[ i ];
            if( Tokens::IsSpace( static_cast< uint8_t >( c ) ) )
            {
                continue;
            }
            if( c == '=' )
            {
                break;
            }
            int decoded = Base64Value( c );
            if( decoded < 0 )
            {
                return false;
            }
            value = ( value << 6 ) + decoded;
            bits += 6;
            if( bits >= 0 )
            {
                a_output += static_cast< char >( ( value >> bits ) & 0xFF );
                bits -= 8;
            }
        }
        return true;
    }

    int HttpAccess::Base64Value( char a_char ) const
    {
        if( ( a_char >= 'A' ) && ( a_char <= 'Z' ) )
        {
            return a_char - 'A';
        }
        if( ( a_char >= 'a' ) && ( a_char <= 'z' ) )
        {
            return a_char - 'a' + 26;
        }
        if( ( a_char >= '0' ) && ( a_char <= '9' ) )
        {
            return a_char - '0' + 52;
        }
        if( a_char == '+' )
        {
            return 62;
        }
        if( a_char == '/' )
        {
            return 63;
        }
        return -1;
    }

    void HttpAccess::Sha1( const uint8_t *a_data, size_t a_len, uint8_t a_output[ 20 ] ) const
    {
        uint32_t h0 = 0x67452301;
        uint32_t h1 = 0xEFCDAB89;
        uint32_t h2 = 0x98BADCFE;
        uint32_t h3 = 0x10325476;
        uint32_t h4 = 0xC3D2E1F0;

        uint64_t bitLen = static_cast< uint64_t >( a_len ) * 8;
        size_t totalLen = a_len + 1;
        while( ( totalLen % 64 ) != 56 )
        {
            ++totalLen;
        }
        ::std::vector< uint8_t > buffer( totalLen + 8, 0 );
        for( size_t i = 0; i < a_len; ++i )
        {
            buffer[ i ] = a_data[ i ];
        }
        buffer[ a_len ] = 0x80;
        for( size_t i = 0; i < 8; ++i )
        {
            buffer[ totalLen + i ] = static_cast< uint8_t >( ( bitLen >> ( 56 - ( i * 8 ) ) ) & 0xFF );
        }

        for( size_t chunk = 0; chunk < buffer.size(); chunk += 64 )
        {
            uint32_t w[ 80 ];
            for( int i = 0; i < 16; ++i )
            {
                size_t index = chunk + ( i * 4 );
                w[ i ] = ( buffer[ index ] << 24 ) |
                         ( buffer[ index + 1 ] << 16 ) |
                         ( buffer[ index + 2 ] << 8 ) |
                         ( buffer[ index + 3 ] );
            }
            for( int i = 16; i < 80; ++i )
            {
                uint32_t value = w[ i - 3 ] ^ w[ i - 8 ] ^ w[ i - 14 ] ^ w[ i - 16 ];
                w[ i ] = ( value << 1 ) | ( value >> 31 );
            }

            uint32_t a = h0;
            uint32_t b = h1;
            uint32_t c = h2;
            uint32_t d = h3;
            uint32_t e = h4;

            for( int i = 0; i < 80; ++i )
            {
                uint32_t f = 0;
                uint32_t k = 0;
                if( i < 20 )
                {
                    f = ( b & c ) | ( ( ~b ) & d );
                    k = 0x5A827999;
                }
                else if( i < 40 )
                {
                    f = b ^ c ^ d;
                    k = 0x6ED9EBA1;
                }
                else if( i < 60 )
                {
                    f = ( b & c ) | ( b & d ) | ( c & d );
                    k = 0x8F1BBCDC;
                }
                else
                {
                    f = b ^ c ^ d;
                    k = 0xCA62C1D6;
                }

                uint32_t temp = ( ( a << 5 ) | ( a >> 27 ) ) + f + e + k + w[ i ];
                e = d;
                d = c;
                c = ( b << 30 ) | ( b >> 2 );
                b = a;
                a = temp;
            }

            h0 += a;
            h1 += b;
            h2 += c;
            h3 += d;
            h4 += e;
        }

        a_output[ 0 ] = ( h0 >> 24 ) & 0xFF;
        a_output[ 1 ] = ( h0 >> 16 ) & 0xFF;
        a_output[ 2 ] = ( h0 >> 8 ) & 0xFF;
        a_output[ 3 ] = h0 & 0xFF;
        a_output[ 4 ] = ( h1 >> 24 ) & 0xFF;
        a_output[ 5 ] = ( h1 >> 16 ) & 0xFF;
        a_output[ 6 ] = ( h1 >> 8 ) & 0xFF;
        a_output[ 7 ] = h1 & 0xFF;
        a_output[ 8 ] = ( h2 >> 24 ) & 0xFF;
        a_output[ 9 ] = ( h2 >> 16 ) & 0xFF;
        a_output[ 10 ] = ( h2 >> 8 ) & 0xFF;
        a_output[ 11 ] = h2 & 0xFF;
        a_output[ 12 ] = ( h3 >> 24 ) & 0xFF;
        a_output[ 13 ] = ( h3 >> 16 ) & 0xFF;
        a_output[ 14 ] = ( h3 >> 8 ) & 0xFF;
        a_output[ 15 ] = h3 & 0xFF;
        a_output[ 16 ] = ( h4 >> 24 ) & 0xFF;
        a_output[ 17 ] = ( h4 >> 16 ) & 0xFF;
        a_output[ 18 ] = ( h4 >> 8 ) & 0xFF;
        a_output[ 19 ] = h4 & 0xFF;
    }

    ::std::string HttpAccess::Base64Encode( const uint8_t *a_data, size_t a_len ) const
    {
        static const char table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
        ::std::string out;
        size_t i = 0;
        while( i < a_len )
        {
            size_t remaining = a_len - i;
            uint32_t octet_a = a_data[ i++ ];
            uint32_t octet_b = ( remaining > 1 ) ? a_data[ i++ ] : 0;
            uint32_t octet_c = ( remaining > 2 ) ? a_data[ i++ ] : 0;
            uint32_t triple = ( octet_a << 16 ) | ( octet_b << 8 ) | octet_c;
            out += table[ ( triple >> 18 ) & 0x3F ];
            out += table[ ( triple >> 12 ) & 0x3F ];
            out += ( remaining > 1 ) ? table[ ( triple >> 6 ) & 0x3F ] : '=';
            out += ( remaining > 2 ) ? table[ triple & 0x3F ] : '=';
        }
        return out;
    }
}
