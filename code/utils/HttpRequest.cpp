/**
    HttpRequest.hpp : HttpRequest implementation
    Copyright 2021 Daniel Wilson
*/

#include <utils/HttpRequest.hpp>
#include <utils/Tokens.hpp>
#include <utils/IniFile.hpp>
#include <utils/File.hpp>
#include <utils/Thread.hpp>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>

#define MAXBUFFERLEN 65536

namespace utils
{
    HttpRequest::HttpRequest()
    : m_length ( 0 )
    , m_start  ( -1 )
    , m_end    ( -1 )
    , m_timeout( false )
    {}

    HttpRequest::~HttpRequest()
    {
        Reset();
    }

    void HttpRequest::Reset()
    {
        utils::Lock lock( this );
        m_method.clear();
        m_uri.clear();
        m_version.clear();
        m_body.clear();
        m_addr.clear();
        m_port    = 0;
        m_length  = 0;
        m_start   = -1;
        m_end     = -1;
        m_timeout = false;
        m_meta    = NULL;
        m_response.clear();
        m_lasterror.clear();
    }

    ::std::string &HttpRequest::Uri()
    {
        utils::Lock lock( this );
        return m_uri;
    }

    ::std::string &HttpRequest::Method()
    {
        utils::Lock lock( this );
        return m_method;
    }

    ::std::string &HttpRequest::Version()
    {
        utils::Lock lock( this );
        return m_version;
    }

    ::std::shared_ptr< KeyValuePair< ::std::string, ::std::string > > &HttpRequest::Meta()
    {
        utils::Lock lock( this );
        return m_meta;
    }

    ::std::string HttpRequest::Host()
    {
        utils::Lock lock( this );
        ::std::string host;
        ::std::shared_ptr< KeyValuePair< ::std::string, ::std::string > > start = m_meta;
        while( start )
        {
            if( start->Key() == "HOST" )
            {
                host = start->Value();
                break;
            }
            start = start->Next();
        }
        return host;
    }

    ::std::string &HttpRequest::Response()
    {
        utils::Lock lock( this );
        return m_response;
    }

    ::std::string &HttpRequest::RemoteAddress()
    {
        utils::Lock lock( this );
        return m_addr;
    }

    uint32_t &HttpRequest::RemotePort()
    {
        utils::Lock lock( this );
        return m_port;
    }

    ::std::string HttpRequest::LastError()
    {
        ::std::string result = m_lasterror;
        m_lasterror.clear();
        return result;
    }

    bool HttpRequest::Read( ::std::shared_ptr< Socket > &a_socket )
    {
        utils::Lock   lock( this );
        uint32_t      timeout = 10;
        uint32_t      count = 0;
        ::std::string token;

        if( !a_socket )
        {
            return false;
        }
        utils::Lock valueLock( a_socket.get() );

        ::std::shared_ptr< Buffer > recvb = ::std::make_shared< Buffer >( MAXBUFFERLEN );

        if( !recvb )
        {
            return false;
        }

        Reset();
        if( !a_socket->Valid() )
        {
            return false;
        }

        // Get the HTTP request
        while( a_socket->Valid() && ( timeout > 0 ) )
        {
            // Read a line of data from the client
            recvb->Clear(); // Reset the buffer each loop
            if( Tokens::GetLine( *a_socket, *recvb ) != TokenTypes::Line )
            {
                usleep( 1000 );
                --timeout;
                continue;
            }
            // Empty line after request
            if( 0 == recvb->Length() )
            {
                // Get more if there is data involved
                if( m_length > 0 )
                {
                    recvb->Clear();
                    timeout = 10;
                    while( ( timeout > 0 ) && a_socket->IsReadable() )
                    {
                        if( !a_socket->Read( recvb ) )
                        {
                            // Calling IsReadable() incurs a timeout
                            --timeout;
                            continue;
                        }
                        uint8_t data = 0;
                        while( recvb->Read( data ) && ( m_body.length() < m_length ) )
                        {
                            m_body += ( char )data;
                        }
                        if( m_body.length() >= m_length )
                        {
                            break;
                        }
                    }
                }
                // This is the end of the HTTP request
                break;
            }
            else if( 0 == m_method.length() )
            {
                // Get the request line
                if( Tokens::GetToken( *recvb, token, ' ' ) == TokenTypes::Delineated )
                {
                    m_method = token;
                    Tokens::MakeUpper( m_method );
                    if( Tokens::GetToken( *recvb, token, ' ' ) == TokenTypes::Delineated )
                    {
                        m_uri = token;
                        if( Tokens::GetToken( *recvb, token, ' ' ) == TokenTypes::Delineated )
                        {
                            m_version = token;
                            Tokens::MakeUpper( m_version );
                        }
                        else
                        {
                            m_method.clear();
                            m_uri.clear();
                        }
                    }
                    else
                    {
                        m_method.clear();
                    }
                }
            }
            else
            {
                // Get the META data
                if( ( count < 100 ) && ( Tokens::GetToken( *recvb, token, ':' ) == TokenTypes::Delineated ) )
                {
                    ++count;
                    auto temp = ::std::make_shared< KeyValuePair< ::std::string, ::std::string > >();
                    if( temp )
                    {
                        temp->Key() = token;
                        Tokens::TrimSpace( temp->Key() );
                        Tokens::MakeUpper( temp->Key() );
                        if( Tokens::GetLine( *recvb, token ) == TokenTypes::Line )
                        {
                            temp->Value() = token;
                            Tokens::TrimSpace( temp->Value() );
                            if( temp->Key() == "CONTENT-LENGTH" )
                            {
                                m_length = 0;
                                if( Tokens::IsNumber( temp->Value() ) )
                                {
                                    try
                                    {
                                        m_length = ::std::stoll( temp->Value() );
                                    }
                                    catch( const ::std::exception &e )
                                    {
                                        m_lasterror.clear();
                                        m_lasterror = temp->Value();
                                        m_lasterror += " (";
                                        m_lasterror += e.what();
                                        m_lasterror += ")";
                                        m_length = 0;
                                    }
                                }
                                // Truncate data to MAXBUFFERLEN
                                if( m_length > MAXBUFFERLEN )
                                {
                                    m_length = MAXBUFFERLEN;
                                }
                            }
                            if( temp->Key() == "RANGE" )
                            {
                                auto range = ::std::make_shared< Buffer >( temp->Value().length() );
                                if( range )
                                {
                                    range->Write( ( const uint8_t * ) temp->Value().c_str(), temp->Value().length() );
                                    m_start = -1;
                                    m_end = -1;
                                    int negative = 0;
                                    TokenType type = TokenType::NotFound;
                                    while( ( type = Tokens::GetToken( *range, token ) ) != TokenType::NotFound )
                                    {
                                        ::std::string token_upper( token );
                                        Tokens::MakeUpper( token_upper );
                                        if( token_upper == "BYTES" )
                                        {
                                            continue;
                                        }
                                        if( token == "=" )
                                        {
                                            continue;
                                        }
                                        if( token == "-" )
                                        {
                                            ++negative;
                                            continue;
                                        }
                                        if( ( -1 == m_start ) && ( type == TokenType::Number ) )
                                        {
                                            try
                                            {
                                                m_start = ::std::stoll( token );
                                                if( negative )
                                                {
                                                    m_start *= -1;
                                                    negative = 0;
                                                }
                                            }
                                            catch( const ::std::exception &e )
                                            {
                                                m_lasterror.clear();
                                                m_lasterror = temp->Value();
                                                m_lasterror += " (";
                                                m_lasterror += e.what();
                                                m_lasterror += ")";
                                                m_start = -1;
                                                break;
                                            }
                                            continue;
                                        }
                                        else if( ( -1 == m_end ) && ( type == TokenType::Number ) )
                                        {
                                            try
                                            {
                                                m_end = ::std::stoll( token );
                                                if( negative > 1 )
                                                {
                                                    m_end *= -1;
                                                    negative = 0;
                                                }
                                            }
                                            catch( const ::std::exception &e )
                                            {
                                                m_lasterror.clear();
                                                m_lasterror = temp->Value();
                                                m_lasterror += " (";
                                                m_lasterror += e.what();
                                                m_lasterror += ")";
                                                m_start = -1;
                                                m_end   = -1;
                                                break;
                                            }
                                            continue;
                                        }
                                        break;
                                    }
                                }
                            }
                            if( !( m_meta ) )
                            {
                                m_meta = temp;
                            }
                            else
                            {
                                auto end = m_meta;
                                while( end && end->Next() )
                                {
                                    end = end->Next();
                                }
                                if( end && !( end->Next() ) )
                                {
                                    end->Next() = temp;
                                }
                            }
                        }
                        else
                        {
                            break;
                        }
                    }
                }
                else
                {
                    break;
                }
            }
        }
        m_timeout = ( timeout <= 0 );
        return ( !m_timeout && ( m_method.length() > 0 ) && ( m_uri.length() > 0 ) && ( m_version.length() > 0 ) );
    }

    int32_t HttpRequest::Respond( ::std::shared_ptr< Socket > &a_socket, ::std::string &a_fileName, ::std::string &a_type, bool a_listDirs )
    {
        utils::Lock lock( this );
        char buffer[ MAXBUFFERLEN ];

        if( !a_socket || !( a_socket->Valid() ) )
        {
            return -1;
        }
        utils::Lock valueLock( a_socket.get() );

        auto sendb = ::std::make_shared< Buffer >( MAXBUFFERLEN );
        auto file  = ::std::make_shared< File >( a_fileName.c_str() );

        if( !sendb || !file )
        {
            return -1;
        }

        if( ( ( m_method == "HEAD" ) || ( m_method == "GET" ) ) && !file->Exists() && ( m_response.length() == 0 ) )
        {
            if( a_socket->Valid() && sendb )
            {
                sendb->Write( ( const uint8_t * )"HTTP/1.1 404 NOT FOUND\r\n" );
                sendb->Write( ( const uint8_t * )"Connection: Close\r\n" );
                sendb->Write( ( const uint8_t * )"Content-type: text/plain\r\n" );
                sendb->Write( ( const uint8_t * )"Content-length: 14\r\n\r\n" );
                sendb->Write( ( const uint8_t * )"NOT FOUND!\r\n\r\n" );
                while( ( sendb->Length() > 0 ) && a_socket->Valid() )
                {
                    a_socket->Write( sendb );
                }
                return 404;
            }
            return -1;
        }
        if( m_timeout || ( 0 == m_version.length() ) )
        {
            sendb->Write( ( const uint8_t * )"HTTP/1.1 408 TIMEOUT\r\n" );
            sendb->Write( ( const uint8_t * )"Connection: Close\r\n" );
            sendb->Write( ( const uint8_t * )"Content-type: text/plain\r\n" );
            sendb->Write( ( const uint8_t * )"Content-length: 12\r\n\r\n" );
            sendb->Write( ( const uint8_t * )"TIMEOUT!\r\n\r\n" );
            while( sendb->Length() && a_socket->Valid() )
            {
                a_socket->Write( sendb );
            }
            return 408;
        }
        if( ( file->IsFile() || ( m_response.length() > 0 ) ) &&
            ( ( ( m_method  == "HEAD" ) ||
                ( m_method  == "GET" ) ) &&
              ( ( m_version == "HTTP/1.1" ) ||
                ( m_version == "HTTP/1.0" ) ) ) )
        {
            if( a_type.length() > 0 )
            {
                // Partial content is only allowed for files, not internally generated content
                if( ( m_method == "GET" ) && ( m_start >= 0 ) )
                {
                    if( file->IsFile() && ( m_end < m_start ) )
                    {
                        m_end = file->Size() - 1;
                    }
                    else if( m_response.length() > 0 )
                    {
                        m_start = 0;
                        m_end   = m_response.length() - 1;
                    }
                    sendb->Write( ( const uint8_t * )"HTTP/1.1 206 PARTIAL CONTENT\r\n" );
                    sendb->Write( ( const uint8_t * )"Connection: Close\r\n" );
                    sendb->Write( ( const uint8_t * )"Content-type: " );
                    sendb->Write( ( const uint8_t * )a_type.c_str(), a_type.length() );
                    sendb->Write( ( const uint8_t * )"\r\n" );
                    sendb->Write( ( const uint8_t * )"Accept-ranges: bytes\r\n" );
                    sendb->Write( ( const uint8_t * )"Content-range: bytes " );
                    snprintf( buffer, sizeof( buffer ), "%lu", m_start );
                    sendb->Write( ( const uint8_t * )buffer, strlen( buffer ) );
                    sendb->Write( ( const uint8_t * )"-" );
                    snprintf( buffer, sizeof( buffer ), "%lu", m_end );
                    sendb->Write( ( const uint8_t * )buffer, strlen( buffer ) );
                    sendb->Write( ( const uint8_t * )"/" );
                    snprintf( buffer, sizeof( buffer ), "%lu", file->Size() );
                    sendb->Write( ( const uint8_t * )buffer, strlen( buffer ) );
                    sendb->Write( ( const uint8_t * )"\r\n" );
                    snprintf( buffer, sizeof( buffer ), "%lu", m_end - m_start + 1 );
                    sendb->Write( ( const uint8_t * )"Content-length: " );
                    sendb->Write( ( const uint8_t * )buffer, strlen( buffer ) );
                    sendb->Write( ( const uint8_t * )"\r\n\r\n", 4 );
                    while( sendb->Length() && a_socket->Valid() )
                    {
                        a_socket->Write( sendb );
                    }
                    if( file->IsFile() )
                    {
                        if( ( m_end >= m_start ) && file->Seek( m_start ) )
                        {
                            while( ( file->Position() < ( m_end + 1 ) ) && a_socket->Valid() )
                            {
                                uint32_t result = 0;
                                if( ( ( m_end + 1 ) - file->Position() ) > ( int64_t )sizeof( buffer ) )
                                {
                                    result = file->Read( ( uint8_t * )buffer, sizeof( buffer ) );
                                }
                                else
                                {
                                    result = file->Read( ( uint8_t * )buffer, ( m_end + 1 ) - file->Position() );
                                }
                                uint32_t total = 0;
                                while( ( total < result ) && a_socket->Valid() )
                                {
                                    total += sendb->Write( ( const uint8_t * )buffer + total, result - total );
                                    if( sendb->Length() > 0 )
                                    {
                                        a_socket->Write( sendb );
                                    }
                                }
                            }
                        }
                        while( sendb->Length() && a_socket->Valid() )
                        {
                            a_socket->Write( sendb );
                        }
                    }
                    else if( m_response.length() > 0 )
                    {
                        uint32_t sent = 0;
                        while( ( sent < m_response.length() ) && a_socket->Valid() )
                        {
                            sent += a_socket->Write( ( uint8_t * )( m_response.c_str() + sent ), static_cast< uint32_t >( m_response.length() - sent ) );
                        }
                    }
                    return 206;
                }
                else
                {
                    if( file->IsFile() )
                    {
                        snprintf( buffer, sizeof( buffer ), "%lu", file->Size() );
                    }
                    else if( m_response.length() > 0 )
                    {
                        snprintf( buffer, sizeof( buffer ), "%lu", m_response.length() );
                    }
                    sendb->Write( ( const uint8_t * )"HTTP/1.1 200 OK\r\n" );
                    sendb->Write( ( const uint8_t * )"Connection: Close\r\n" );
                    sendb->Write( ( const uint8_t * )"Content-type: " );
                    sendb->Write( ( const uint8_t * )a_type.c_str(), a_type.length() );
                    sendb->Write( ( const uint8_t * )"\r\n" );
                    sendb->Write( ( const uint8_t * )"Accept-ranges: bytes\r\n" );
                    sendb->Write( ( const uint8_t * )"Content-length: " );
                    sendb->Write( ( const uint8_t * )buffer, strlen( buffer ) );
                    sendb->Write( ( const uint8_t * )"\r\n\r\n" );
                    while( sendb->Length() && a_socket->Valid() )
                    {
                        a_socket->Write( sendb );
                    }
                    if( m_method == "GET" )
                    {
                        if( file->IsFile() )
                        {
                            while( ( ( file->Size() - file->Position() ) > 0 ) && a_socket->Valid() )
                            {
                                uint32_t result = 0;
                                if( ( file->Size() - file->Position() ) > sizeof( buffer ) )
                                {
                                    result = file->Read( ( uint8_t * )buffer, sizeof( buffer ) );
                                }
                                else
                                {
                                    result = file->Read( ( uint8_t * )buffer, file->Size() - file->Position() );
                                }
                                uint32_t total = 0;
                                while( ( total < result ) && a_socket->Valid() )
                                {
                                    if( result > 0 )
                                    {
                                        total += sendb->Write( ( const uint8_t * )buffer + total, result - total );
                                    }
                                    if( sendb->Length() > 0 )
                                    {
                                        a_socket->Write( sendb );
                                    }
                                }
                            }
                            while( sendb->Length() && a_socket->Valid() )
                            {
                                a_socket->Write( sendb );
                            }
                        }
                        else if( m_response.length() > 0 )
                        {
                            uint32_t sent = 0;
                            while( ( sent < m_response.length() ) && a_socket->Valid() )
                            {
                                sent += a_socket->Write( ( uint8_t * )( m_response.c_str() + sent ), static_cast< uint32_t >( m_response.length() - sent ) );
                            }
                        }
                    }
                    return 200;
                }
            }
            else
            {
                sendb->Write( ( const uint8_t * )"HTTP/1.1 404 NOT FOUND\r\n" );
                sendb->Write( ( const uint8_t * )"Connection: Close\r\n" );
                sendb->Write( ( const uint8_t * )"Content-type: text/html\r\n" );
                sendb->Write( ( const uint8_t * )"Content-length: 57\r\n\r\n" );
                sendb->Write( ( const uint8_t * )"<html><head><center>Not Found!</center></head></html>\r\n\r\n" );
                while( sendb->Length() && a_socket->Valid() )
                {
                    a_socket->Write( sendb );
                }
                return 404;
            }
        }
        else if( a_listDirs && file->IsDirectory() &&
               ( ( ( m_method  == "HEAD" ) ||
                   ( m_method  == "GET" ) ) &&
                 ( ( m_version == "HTTP/1.1" ) ||
                   ( m_version == "HTTP/1.0" ) ) ) )
        {
            sendb->Write( ( const uint8_t * )"HTTP/1.1 200 OK\r\n" );
            sendb->Write( ( const uint8_t * )"Connection: Close\r\n" );
            sendb->Write( ( const uint8_t * )"Content-type: text/html\r\n" );
            sendb->Write( ( const uint8_t * )"\r\n" );
            while( sendb->Length() && a_socket->Valid() )
            {
                a_socket->Write( sendb );
            }
            if( m_method == "GET" )
            {
                DIR *dir = opendir( file->Name().c_str() );
                if( nullptr == dir )
                {
                    return 200;
                }

                // Start the html content
                sendb->Write( ( const uint8_t * )"<head><title>Directory Listing</title></head>\n<body>\n" );

                // Enumerate the directory contents
                struct dirent *entry;
                while( ( entry = readdir( dir ) ) != nullptr )
                {
                    // Skip symlinks
                    if( entry->d_type == DT_LNK )
                    {
                        continue;
                    }
                    // Skip ., .., and hidden files
                    if( entry->d_name[0] == '.' )
                    {
                        continue;
                    }
                    sendb->Write( ( const uint8_t * )"<a style=\"font-family: monospace;\" href=\"" );
                    sendb->Write( ( const uint8_t * )entry->d_name );
                    sendb->Write( ( const uint8_t * )"\">");
                    sendb->Write( ( const uint8_t * )entry->d_name );
                    if( entry->d_type == DT_DIR )
                    {
                        sendb->Write( ( const uint8_t * )"/" );
                    }
                    sendb->Write( ( const uint8_t * )"</a><br>\n" );
                    while( sendb->Length() && a_socket->Valid() )
                    {
                        a_socket->Write( sendb );
                    }
                }
                closedir( dir );
                sendb->Write( ( const uint8_t * )"</body>\n" );
                while( sendb->Length() && a_socket->Valid() )
                {
                    a_socket->Write( sendb );
                }
            }
            return 200;
        }
        else
        {
            sendb->Write( ( const uint8_t * )"HTTP/1.1 405 METHOD NOT ALLOWED\r\n" );
            sendb->Write( ( const uint8_t * )"Allow: GET, HEAD\r\n" );
            sendb->Write( ( const uint8_t * )"Connection: Close\r\n" );
            sendb->Write( ( const uint8_t * )"Content-type: text/html\r\n" );
            sendb->Write( ( const uint8_t * )"Content-length: 59\r\n\r\n" );
            sendb->Write( ( const uint8_t * )"<html><head><center>Not Allowed!</center></head></html>\r\n\r\n" );
            while( sendb->Length() && a_socket->Valid() )
            {
                a_socket->Write( sendb );
            }
            return 405;
        }
    }

    void HttpRequest::Log( LogFile &a_logger )
    {
        utils::Lock lock( this );
        utils::Lock valueLock( &a_logger );
        char port[ 32 ];
        ::std::shared_ptr< KeyValuePair< ::std::string, ::std::string > > start = m_meta;

        snprintf( port, sizeof( port ), "%u", m_port );
        a_logger.Log( m_addr, true, false );
        a_logger.Log( ":", false, false );
        a_logger.Log( port, false, false );
        a_logger.Log( " - ", false, false );
        a_logger.Log( m_method, false, false );
        a_logger.Log( " ", false, false );
        a_logger.Log( m_uri, false, false );
        a_logger.Log( " ", false, false );
        a_logger.Log( m_version, false, true );

        while( start )
        {
            a_logger.Log( m_addr, true, false );
            a_logger.Log( ":", false, false );
            a_logger.Log( port, false, false );
            a_logger.Log( " - ", false, false );
            a_logger.Log( start->Key(), false, false );
            a_logger.Log( " = ", false, false );
            a_logger.Log( start->Value(), false, true );
            start = start->Next();
        }

        if( m_body.length() > 0 )
        {
            ::std::string hex;
            ::std::string printable;
            uint8_t max_width = 0;
            for( size_t i = 0; i < m_body.length(); ++i )
            {
                if( Tokens::IsPrintable( ( uint8_t )m_body[ i ] ) )
                {
                    printable += m_body[ i ];
                }
                else
                {
                    printable += '.';
                }
                char tmp[8];
                snprintf( tmp, sizeof( tmp ), "%02X ", 0xFF & m_body[ i ] );
                hex += tmp;
                if( ( ( ( i + 1 ) % 12 ) == 0 ) || ( i == ( m_body.length() - 1 ) ) )
                {
                    if( hex.length() > max_width )
                    {
                        max_width = hex.length();
                    }
                    a_logger.Log( m_addr, true, false );
                    a_logger.Log( ":", false, false );
                    a_logger.Log( port, false, false );
                    a_logger.Log( " - [ ", false, false );
                    a_logger.Log( hex, false, false );
                    for( uint8_t j = hex.length(); j < max_width; ++j )
                    {
                        a_logger.Log( " ", false, false );
                    }
                    a_logger.Log( "]  ", false, false );
                    a_logger.Log( printable, false, true );
                    hex.clear();
                    printable.clear();
                }
            }
        }

        if( m_lasterror.length() > 0 )
        {
            a_logger.Log( m_addr, true, false );
            a_logger.Log( ":", false, false );
            a_logger.Log( port, false, false );
            a_logger.Log( " - ", false, false );
            a_logger.Log( "Last error = ", false, false );
            a_logger.Log( m_lasterror.c_str(), false, true );
        }
    }

    uint8_t HttpHelpers::CharToHex( char a_value )
    {
        if( ( a_value >= '0' ) && ( a_value <= '9' ) )
        {
            return ( a_value - '0' );
        }
        if( ( a_value >= 'A' ) && ( a_value <= 'Z' ) )
        {
            a_value += ( 'a' - 'A' );
        }
        switch( a_value )
        {
            case 'a':
                return 10;
            case 'b':
                return 11;
            case 'c':
                return 12;
            case 'd':
                return 13;
            case 'e':
                return 14;
            case 'f':
                return 15;
            default:
                return 128;
        }
    }

    uint32_t HttpHelpers::UriDecode( ::std::string &a_uri, ::std::string &a_ext )
    {
        ::std::string newUri;
        uint32_t changes = 0;

        for( uint32_t i = 0; i < a_uri.length(); ++i )
        {
            switch( a_uri[ i ] )
            {
                case '/':
                case '\\':
                    a_ext.clear();
                    if( ( newUri.length() > 0 ) && ( '/' != newUri[ newUri.length() - 1 ] ) )
                    {
                        newUri += '/';
                    }
                    break;
                case '.':
                    a_ext = '.';
                    if( ( newUri.length() > 0 ) && ( '.' != newUri[ newUri.length() - 1 ] ) )
                    {
                        newUri += '.';
                    }
                    break;
                case '%':
                    if( i < ( uint32_t )( a_uri.length() - 2 ) )
                    {
                        char a = HttpHelpers::CharToHex( a_uri[ i + 1 ] );
                        char b = HttpHelpers::CharToHex( a_uri[ i + 2 ] );
                        if( ( a <= 0x0F ) && ( b <= 0x0F ) )
                        {
                            newUri += ( char )( ( a << 4 ) & 0xF0 ) +
                                      ( char )( ( b      ) & 0x0F );
                            ++changes;
                            i += 2;
                        }
                    }
                    break;
                default:
                    if( ( a_uri[ i ] >= ' ' ) && ( a_uri[ i ] < '~' ) )
                    {
                        newUri += a_uri[ i ];
                        if( a_ext.length() > 0 )
                        {
                            a_ext += a_uri[ i ];
                        }
                    }
                    else
                    {
                        a_ext.clear();
                    }
                    break;
            }
        }
        Tokens::MakeLower( a_ext );
        a_uri = newUri;

        return changes;
    }

    bool HttpHelpers::UriDecode( ::std::string &a_base, ::std::string &a_defaultDoc, ::std::string &a_uri, ::std::string &a_ext, ::std::string &a_defmime )
    {
        while( HttpHelpers::UriDecode( a_uri, a_ext ) != 0 );

        ::std::string newUri( a_base );
        if( ( newUri.length() > 0 ) && ( '/' != newUri[ newUri.length() - 1 ] ) )
        {
            newUri += '/';
        }
        newUri += a_uri;
        bool isDir = IsDirectory( newUri );
        if( isDir && ( newUri.length() > 0 ) && ( '/' != newUri[ newUri.length() - 1 ] ) )
        {
            newUri += '/';
        }

        // If this is a directory, check if the default document exists
        if( isDir || ( a_ext.length() == 0 ) )
        {
            ::std::string newDefUri( newUri );
            while( HttpHelpers::UriDecode( a_defaultDoc, a_ext ) != 0 );
            newDefUri += a_defaultDoc;

            // Use the default document if it exists
            if( IsFile( newDefUri ) )
            {
                newUri = newDefUri;
            }
            else
            {
                // Clear the extension in the case the default document isn't used
                a_ext.clear();
                // Treating it like a directory either way at this point
                isDir = true;
            }
        }

        if( a_ext.length() == 0 )
        {
            // Set the extention to HTML for directory listings
            a_ext = ( isDir )? ".html": a_defmime;
        }
        a_uri = newUri;

        if( a_uri.length() > 0 )
        {
            return true;
        }
        return false;
    }

    bool HttpHelpers::IsDirectory( ::std::string &a_path )
    {
        struct stat st;
        if( stat( a_path.c_str(), &st ) != 0 )
        {
            return false;
        }
        return S_ISDIR( st.st_mode );
    }

    bool HttpHelpers::IsFile( ::std::string &a_path )
    {
        struct stat st;
        if( stat( a_path.c_str(), &st ) != 0 )
        {
            return false;
        }
        return S_ISREG( st.st_mode );
    }
}