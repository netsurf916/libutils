/**
    HttpRequest.hpp : HttpRequest implementation
    Copyright 2021-2026 Daniel Wilson
    SPDX-License-Identifier: MIT
*/

#include <utils/HttpRequest.hpp>
#include <utils/File.hpp>
#include <utils/Tokens.hpp>
#include <dirent.h>
#include <string.h>
#include <unistd.h>
#include <vector>
#include <algorithm>

#define MAXBUFFERLEN 65536

namespace utils
{
    HttpRequest::HttpRequest()
    : m_length ( 0 )
    , m_start  ( -1 )
    , m_end    ( -1 )
    , m_sset   ( false )
    , m_eset   ( false )
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

    bool HttpRequest::HeaderValue( const ::std::string &a_key, ::std::string &a_value )
    {
        utils::Lock lock( this );
        a_value.clear();
        if( a_key.length() == 0 )
        {
            return false;
        }
        ::std::string key( a_key );
        Tokens::MakeUpper( key );
        ::std::shared_ptr< KeyValuePair< ::std::string, ::std::string > > start = m_meta;
        while( start )
        {
            if( start->Key() == key )
            {
                a_value = start->Value();
                return true;
            }
            start = start->Next();
        }
        return false;
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
                                    m_end   = -1;
                                    m_sset  = false;
                                    m_eset  = false;
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
                                        if( !m_sset && ( type == TokenType::Number ) )
                                        {
                                            try
                                            {
                                                m_start = ::std::stoll( token );
                                                if( negative )
                                                {
                                                    m_start *= -1;
                                                    negative = 0;
                                                }
                                                m_sset = true;
                                            }
                                            catch( const ::std::exception &e )
                                            {
                                                m_lasterror.clear();
                                                m_lasterror = temp->Value();
                                                m_lasterror += " (";
                                                m_lasterror += e.what();
                                                m_lasterror += ")";
                                                m_start = -1;
                                                m_sset  = false;
                                                break;
                                            }
                                            continue;
                                        }
                                        else if( !m_eset && ( type == TokenType::Number ) )
                                        {
                                            try
                                            {
                                                m_end = ::std::stoll( token );
                                                if( negative > 1 )
                                                {
                                                    m_end *= -1;
                                                    negative = 0;
                                                }
                                                m_eset = true;
                                            }
                                            catch( const ::std::exception &e )
                                            {
                                                m_lasterror.clear();
                                                m_lasterror = temp->Value();
                                                m_lasterror += " (";
                                                m_lasterror += e.what();
                                                m_lasterror += ")";
                                                m_end   = -1;
                                                m_eset  = false;
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

        bool serverOptions = ( m_method == "OPTIONS" ) && ( m_uri == "*" );
        if( ( ( m_method == "HEAD" ) || ( m_method == "GET" ) || ( m_method == "OPTIONS" ) ) && !serverOptions &&
            !file->Exists() && ( m_response.length() == 0 ) )
        {
            if( a_socket->Valid() && sendb )
            {
                sendb->Write( ( const uint8_t * )"HTTP/1.1 404 NOT FOUND\r\n" );
                sendb->Write( ( const uint8_t * )"Connection: Close\r\n" );
                sendb->Write( ( const uint8_t * )"Content-Length: 0\r\n\r\n" );
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
            sendb->Write( ( const uint8_t * )"Content-Length: 0\r\n\r\n" );
            while( sendb->Length() && a_socket->Valid() )
            {
                a_socket->Write( sendb );
            }
            return 408;
        }
        if( ( m_method == "OPTIONS" ) &&
            ( ( m_version == "HTTP/1.1" ) ||
              ( m_version == "HTTP/1.0" ) ) )
        {
            sendb->Write( ( const uint8_t * )"HTTP/1.1 200 OK\r\n" );
            sendb->Write( ( const uint8_t * )"Allow: GET, HEAD, OPTIONS\r\n" );
            sendb->Write( ( const uint8_t * )"Connection: Close\r\n" );
            sendb->Write( ( const uint8_t * )"Content-Length: 0\r\n\r\n" );
            while( sendb->Length() && a_socket->Valid() )
            {
                a_socket->Write( sendb );
            }
            return 200;
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
                if( ( m_method == "GET" ) && m_sset && file->IsFile() )
                {
                    if( m_start < 0 )
                    {
                        int64_t length = -m_start;
                        if( length < 0 ) // Necessary check in case of min value
                        {
                            length = 0;
                        }
                        int64_t size = static_cast< int64_t >( file->Size() );
                        if( size > 0 )
                        {
                            m_start = size - length;
                            if( m_start < 0 )
                            {
                                m_start = 0;
                            }
                            m_end = size - 1;
                        }
                    }
                    if( !m_eset || ( m_end < 0 ) )
                    {
                        m_end = file->Size() - 1;
                    }
                    if( ( file->Size() == 0 ) || ( m_end < m_start ) )
                    {
                        sendb->Write( ( const uint8_t * )"HTTP/1.1 416 RANGE NOT SATISFIABLE\r\n" );
                        sendb->Write( ( const uint8_t * )"Connection: Close\r\n" );
                        sendb->Write( ( const uint8_t * )"Content-Range: bytes */" );
                        snprintf( buffer, sizeof( buffer ), "%lu", file->Size() );
                        sendb->Write( ( const uint8_t * )buffer, strlen( buffer ) );
                        sendb->Write( ( const uint8_t * )"\r\n");
                        sendb->Write( ( const uint8_t * )"Content-Length: 0\r\n\r\n" );
                        while( sendb->Length() && a_socket->Valid() )
                        {
                            a_socket->Write( sendb );
                        }
                        return 416;
                    }
                    sendb->Write( ( const uint8_t * )"HTTP/1.1 206 PARTIAL CONTENT\r\n" );
                    sendb->Write( ( const uint8_t * )"Connection: Close\r\n" );
                    sendb->Write( ( const uint8_t * )"Content-Type: " );
                    sendb->Write( ( const uint8_t * )a_type.c_str(), a_type.length() );
                    sendb->Write( ( const uint8_t * )"\r\n" );
                    sendb->Write( ( const uint8_t * )"Accept-Ranges: bytes\r\n" );
                    sendb->Write( ( const uint8_t * )"Content-Range: bytes " );
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
                    sendb->Write( ( const uint8_t * )"Content-Length: " );
                    sendb->Write( ( const uint8_t * )buffer, strlen( buffer ) );
                    sendb->Write( ( const uint8_t * )"\r\n\r\n", 4 );
                    while( sendb->Length() && a_socket->Valid() )
                    {
                        a_socket->Write( sendb );
                    }
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
                    sendb->Write( ( const uint8_t * )"Content-Type: " );
                    sendb->Write( ( const uint8_t * )a_type.c_str(), a_type.length() );
                    sendb->Write( ( const uint8_t * )"\r\n" );
                    sendb->Write( ( const uint8_t * )"Accept-Ranges: bytes\r\n" );
                    sendb->Write( ( const uint8_t * )"Content-Length: " );
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
                                result = file->Read( ( uint8_t * )buffer, sizeof( buffer ) );
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
                sendb->Write( ( const uint8_t * )"Content-Length: 0\r\n\r\n" );
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
            sendb->Write( ( const uint8_t * )"Content-Type: text/html\r\n\r\n" );
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

                // Enumerate the directory contents
                struct dirent *entry;
                std::vector< std::string > listing;
                while( ( entry = readdir( dir ) ) != nullptr )
                {
                    std::string name = entry->d_name;
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
                    if( entry->d_type == DT_DIR )
                    {
                        name += '/';
                    }
                    listing.push_back( name );
                }
                closedir( dir );
                entry = nullptr;

                // Sort the list
                std::sort( listing.begin(), listing.end() );

                // Start the html content
                sendb->Write( ( const uint8_t * )"<!DOCTYPE html>\n<head>\n<meta charset=\"utf-8\">\n<title>");
                sendb->Write( ( const uint8_t * )HttpHelpers::HtmlEscape( HttpHelpers::UriDecode( m_uri ) ).c_str() );
                // For no style, use:
                //sendb->Write( ( const uint8_t * )"</title><meta charset=\"utf-8\"></head>\n<body>\n" );
                sendb->Write( ( const uint8_t * )
                    "</title></head>\n"
                    "<body style=\""
                    "margin:0;"
                    "padding:16px;"
                    "background:#000;"
                    "color:#ddd;"
                    "font:14px/1.45 system-ui,-apple-system,Segoe UI,Roboto,Arial,sans-serif;"
                    "\">\n"
                    "<div style=\""
                    "max-width:900px;"
                    "margin:auto;"
                    "background:#222;"
                    "border-radius:12px;"
                    "padding:12px 16px;"
                    "box-shadow:0 0 0 1px #000;"
                    "\">\n"
                    "<div style=\"font-weight:600;margin-bottom:12px;\">Index of " );
                sendb->Write((const uint8_t*)
                HttpHelpers::HtmlEscape(HttpHelpers::UriDecode(m_uri)).c_str());
                sendb->Write((const uint8_t*)"</div>\n");

                // Use the sorted directory listing to write the listing for the client
                for( auto& name : listing )
                {
                    if( name.empty() )
                    {
                        continue;
                    }
                    // For no style, use:
                    //sendb->Write( ( const uint8_t * )"<a style=\"font-family: monospace;\" href=\"" );
                    sendb->Write( ( const uint8_t * )
                        "<div style=\""
                        "display:flex;"
                        "align-items:center;"
                        "gap:8px;"
                        "padding:6px 4px;"
                        "border-radius:6px;"
                        "\">\n"
                        "<a style=\""
                        "color:#e6edf3;"
                        "text-decoration:none;"
                        "font-family:ui-monospace,SFMono-Regular,Menlo,Consolas,monospace;"
                        "\" href=\"" );
                    if( name.back() == '/' )
                    {
                        // URL encoding the trailing slash is ugly and breaks things
                        sendb->Write( ( const uint8_t * )HttpHelpers::UriEncode( name.substr(0, name.size() - 1 ) ).c_str() );
                        // The slash is needed for relative links, however
                        sendb->Write( ( const uint8_t * )"/" );
                    }
                    else
                    {
                        sendb->Write( ( const uint8_t * )HttpHelpers::UriEncode( name ).c_str() );
                    }
                    sendb->Write( ( const uint8_t * )"\">");
                    // For no style, remove this line:
                    sendb->Write( ( const uint8_t * )"<span style=\"color:#ddd;\">• </span>" );
                    sendb->Write( ( const uint8_t * )HttpHelpers::HtmlEscape( name ).c_str() );
                    // For no style, use:
                    //sendb->Write( ( const uint8_t * )"</a><br>\n" );
                    sendb->Write( ( const uint8_t * )"</a></div>\n" );
                    while( sendb->Length() && a_socket->Valid() )
                    {
                        a_socket->Write( sendb );
                    }
                }
                // For no style, use:
                //sendb->Write( ( const uint8_t * )"</body>\n" );
                sendb->Write( ( const uint8_t * )"</div></body>\n" );
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
            sendb->Write( ( const uint8_t * )"Allow: GET, HEAD, OPTIONS\r\n" );
            sendb->Write( ( const uint8_t * )"Connection: Close\r\n" );
            sendb->Write( ( const uint8_t * )"Content-Length: 0\r\n\r\n" );
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
}
