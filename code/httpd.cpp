/**
    httpd.cpp : Simple web server implementation
    Copyright 2021 Daniel Wilson
*/

#include <utils/Socket.hpp>
#include <utils/Tokens.hpp>
#include <utils/LogFile.hpp>
#include <utils/IniFile.hpp>
#include <utils/File.hpp>
#include <utils/Thread.hpp>
#include <utils/KeyValuePair.hpp>
#include <sys/stat.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>

#define NUMTHREADS 128
#define DEFMIME    "none" // Make sure this is defined in the ini file

using namespace utils;

struct ThreadCTX : public Lockable
{
    ::std::shared_ptr< Socket >  socket;
    ::std::shared_ptr< LogFile > logger;
    ::std::shared_ptr< IniFile > settings;
};

void *ProcessClient( void *a_client );
bool  UriDecode( ::std::string &a_base, ::std::string &a_defaultDoc, ::std::string &a_uri, ::std::string &a_ext );

int main( int argc, char *argv[] )
{
    UNUSED( argc );
    UNUSED( argv );

    uid_t runningAs = getuid();

    // Try for root
    if( 0 != runningAs )
    {
        if( 0 == setuid( 0 ) )
        {
            printf( " [!] Assuming admin privileges.\n" );
        }
        else
        {
            printf( " [!] Insufficient privileges!\n" );
        }
    }

    ::std::shared_ptr< LogFile > logger   = ::std::make_shared< LogFile >( "httpd.log" );
    ::std::shared_ptr< IniFile > settings = ::std::make_shared< IniFile >( "httpd.ini" );

    if( !( logger ) || !( settings ) )
    {
        return 0;
    }

    ::std::string port;
    ::std::string address;
    settings->ReadValue( "settings", "port",    port );
    settings->ReadValue( "settings", "address", address );

    if( ( 0 == port.length() ) || ( 0 == address.length() ) )
    {
        printf( " [!] Failed to read configuration\n" );
        return 0;
    }

    ::std::shared_ptr< Socket > listener = ::std::make_shared< Socket >( address.c_str(), ::std::stoi( port ), SocketFlags::TcpServer );
    if( !listener || !listener->Valid() )
    {
        printf( " [!] Error listening on: %s:%lu\n", address.c_str(), ::std::stol( port ) );
        return 0;
    }

    printf( " [+] Listening for incoming connnections on: %s:%lu\n",
        address.c_str(), ::std::stol( port ) );
    logger->Log( "Listening for incoming connections on: ", true, false );
    logger->Log( address, false, false );
    logger->Log( ":", false, false );
    logger->Log( port, false, true );

    if( ( 0 != runningAs ) && ( 0 == setuid( runningAs ) ) )
    {
        printf( " [!] Giving up admin privileges.\n" );
    }

    ::std::shared_ptr< Thread< ThreadCTX > > clients[ NUMTHREADS ];

    while( listener->Valid() )
    {
        int32_t clientfd = -1;
        if( listener->Accept( clientfd ) )
        {
            bool found = false;
            printf( " [*] Client connected\n" );
            while( !found && listener->Valid() )
            {
                for( uint32_t c = 0; !found && ( c < NUMTHREADS ); ++c )
                {
                    if( clients[ c ] && !( clients[ c ]->IsRunning() ) )
                    {
                        clients[ c ].reset();
                    }
                    if( !( clients[ c ] ) )
                    {
                        clients[ c ] = ::std::make_shared< Thread< ThreadCTX > >( ProcessClient );
                        if( clients[ c ] && clients[ c ]->GetContext() )
                        {
                            clients[ c ]->GetContext()->socket   = ::std::make_shared< Socket >( clientfd );
                            clients[ c ]->GetContext()->logger   = logger;
                            clients[ c ]->GetContext()->settings = settings;
                            if( clients[ c ]->GetContext()->socket &&
                                clients[ c ]->GetContext()->socket->Valid() )
                            {
                                clients[ c ]->Start();
                                found = true;
                            }
                            else
                            {
                                clients[ c ] = NULL;
                            }
                        }
                    }
                }
                if( !found )
                {
                    usleep( 12500 );
                }
            }
            printf( " [+] Client thread started\n" );
        }
        else
        {
            usleep( 25000 );
        }
        for( uint32_t c = 0; c < NUMTHREADS; ++c )
        {
            if( clients[ c ] && !( clients[ c ]->IsRunning() ) )
            {
                clients[ c ].reset();
            }
        }
    }

    pthread_exit( NULL );
}

class HttpRequest : public Lockable
{
    private:
        ::std::string   m_method;
        ::std::string   m_uri;
        ::std::string   m_version;
        uint64_t m_length;
        int64_t  m_start;
        int64_t  m_end;
        ::std::string   m_body;
        ::std::string   m_host;
        uint32_t m_port;
        bool     m_timeout;
        ::std::shared_ptr< KeyValuePair< ::std::string, ::std::string > > m_meta;

    public:
        void Reset()
        {
            utils::Lock lock( this );
            m_method.clear();
            m_uri.clear();
            m_version.clear();
            m_body.clear();
            m_host.clear();
            m_port    = 0;
            m_length  = 0;
            m_start   = -1;
            m_end     = -1;
            m_timeout = false;
            m_meta    = NULL;
        }

        HttpRequest()
        : m_length ( 0 )
        , m_start  ( -1 )
        , m_end    ( -1 )
        , m_timeout( false )
        {}

        ~HttpRequest()
        {
            Reset();
        }

        ::std::string &Uri()
        {
            utils::Lock lock( this );
            return m_uri;
        }

        ::std::string &Method()
        {
            utils::Lock lock( this );
            return m_method;
        }

        ::std::shared_ptr< KeyValuePair< ::std::string, ::std::string > > &Meta()
        {
            utils::Lock lock( this );
            return m_meta;
        }

        ::std::string Host()
        {
            utils::Lock  lock( this );
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

        bool Read( Socket &a_socket )
        {
            utils::Lock   lock( this );
            utils::Lock   valueLock( &a_socket );
            uint32_t      timeout = 100;
            uint32_t      count = 0;
            ::std::string token;

            ::std::shared_ptr< Buffer > recvb = ::std::make_shared< Buffer >( 2048 );

            if( !recvb )
            {
                return false;
            }

            Reset();
            if( a_socket.Valid() )
            {
                a_socket.GetRemoteAddress( m_host, m_port );
            }
            else
            {
                return false;
            }

            // Get the HTTP request
            while( a_socket.Valid() && ( timeout > 0 ) )
            {
                if( !a_socket.ReadLine( recvb ) )
                {
                    --timeout;
                    usleep( 10000 );
                    continue;
                }
                if( 0 == recvb->Length() )
                {
                    // Get more if there is data involved
                    if( m_length > 0 )
                    {
                        recvb->Clear();
                        timeout = 100;
                        while( ( timeout > 0 ) && a_socket.Valid() )
                        {
                            if( !a_socket.Read( recvb ) )
                            {
                                --timeout;
                                usleep( 10000 );
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
                                    m_length = ::std::stoi( temp->Value() );
                                    // Truncate data to 4k bytes
                                    if( m_length > 4096 )
                                    {
                                        m_length = 4096;
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
                                        while( Tokens::GetToken( *range, token ) != TokenTypes::NotFound )
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
                                            if( -1 == m_start )
                                            {
                                                m_start = ::std::stoi( token );
                                                if( negative )
                                                {
                                                    m_start *= -1;
                                                    negative = 0;
                                                }
                                                continue;
                                            }
                                            else if( -1 == m_end )
                                            {
                                                m_end = ::std::stoi( token );
                                                if( negative > 1 )
                                                {
                                                    m_end *= -1;
                                                    negative = 0;
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

        int32_t Respond( Socket &a_socket, ::std::string &a_fileName, ::std::string &a_type )
        {
            utils::Lock    lock( this );
            utils::Lock    valueLock( &a_socket );
            auto sendb = ::std::make_shared< Buffer >( 2048 );
            auto file  = ::std::make_shared< File >( a_fileName.c_str() );

            if( !a_socket.Valid() || !sendb || !file || ( ( ( m_method == "HEAD" ) || ( m_method == "GET" ) ) && !file->Exists() ) )
            {
                if( a_socket.Valid() && sendb )
                {
                    sendb->Write( ( const uint8_t * )m_version.c_str(), m_version.length() );
                    sendb->Write( ( const uint8_t * )" 404 NOT FOUND\r\n" );
                    sendb->Write( ( const uint8_t * )"Content-type: text/html\r\n" );
                    sendb->Write( ( const uint8_t * )"Content-length: 57\r\n\r\n" );
                    sendb->Write( ( const uint8_t * )"<html><head><center>Not Found!</center></head></html>\r\n\r\n" );
                    while( a_socket.Write( sendb ) );
                    return 404;
                }
                return -1;
            }
            if( m_timeout || ( 0 == m_version.length() ) )
            {
                sendb->Write( ( const uint8_t * )"HTTP/1.1 408 TIMEOUT\r\n" );
                sendb->Write( ( const uint8_t * )"Content-type: text/html\r\n" );
                sendb->Write( ( const uint8_t * )"Content-length: 55\r\n\r\n" );
                sendb->Write( ( const uint8_t * )"<html><head><center>Timeout!</center></head></html>\r\n\r\n" );
                while( a_socket.Write( sendb ) );
                return 408;
            }
            else if( ( ( m_method == "HEAD" ) ||
                     ( m_method == "GET" ) ) &&
                     ( ( m_version == "HTTP/1.1" ) ||
                     ( m_version == "HTTP/1.0" ) ) )
            {
                if( a_type.length() )
                {
                    char buffer[ 4096 ];
                    if( ( m_method == "GET" ) && ( m_start >= 0 ) )
                    {
                        if( m_end < m_start )
                        {
                            m_end = file->Size() - 1;
                        }
                        sendb->Write( ( const uint8_t * )m_version.c_str(), m_version.length() );
                        sendb->Write( ( const uint8_t * )" 206 PARTIAL CONTENT\r\n" );
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
                        while( sendb->Length() && a_socket.Valid() )
                        {
                            a_socket.Write( sendb );
                        }
                        if( ( m_end >= m_start ) && file->Seek( m_start ) )
                        {
                            while( ( file->Position() < ( m_end + 1 ) ) && a_socket.Valid() )
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
                                while( ( total < result ) && a_socket.Valid() )
                                {
                                    total += sendb->Write( ( const uint8_t * )buffer + total, result - total );
                                    if( sendb->Length() > 0 )
                                    {
                                        a_socket.Write( sendb );
                                    }
                                }
                            }
                        }
                        while( sendb->Length() && a_socket.Valid() )
                        {
                            a_socket.Write( sendb );
                        }
                        return 206;
                    }
                    else
                    {
                        snprintf( buffer, sizeof( buffer ), "%lu", file->Size() );
                        sendb->Write( ( const uint8_t * )m_version.c_str(), m_version.length() );
                        sendb->Write( ( const uint8_t * )" 200 OK\r\n" );
                        sendb->Write( ( const uint8_t * )"Content-type: " );
                        sendb->Write( ( const uint8_t * )a_type.c_str(), a_type.length() );
                        sendb->Write( ( const uint8_t * )"\r\n" );
                        sendb->Write( ( const uint8_t * )"Accept-ranges: bytes\r\n" );
                        sendb->Write( ( const uint8_t * )"Content-length: " );
                        sendb->Write( ( const uint8_t * )buffer, strlen( buffer ) );
                        sendb->Write( ( const uint8_t * )"\r\n\r\n" );
                        while( sendb->Length() && a_socket.Valid() )
                        {
                            a_socket.Write( sendb );
                        }
                        if( m_method == "GET" )
                        {
                            while( ( ( file->Size() - file->Position() ) > 0 ) && a_socket.Valid() )
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
                                while( ( total < result ) && a_socket.Valid() )
                                {
                                    if( result > 0 )
                                    {
                                        total += sendb->Write( ( const uint8_t * )buffer + total, result - total );
                                    }
                                    if( sendb->Length() > 0 )
                                    {
                                        a_socket.Write( sendb );
                                    }
                                }
                            }
                        }
                        while( sendb->Length() && a_socket.Valid() )
                        {
                            a_socket.Write( sendb );
                        }
                        return 200;
                    }
                }
                else
                {
                    sendb->Write( ( const uint8_t * )m_version.c_str(), m_version.length() );
                    sendb->Write( ( const uint8_t * )" 404 NOT FOUND\r\n" );
                    sendb->Write( ( const uint8_t * )"Content-type: text/html\r\n" );
                    sendb->Write( ( const uint8_t * )"Content-length: 57\r\n\r\n" );
                    sendb->Write( ( const uint8_t * )"<html><head><center>Not Found!</center></head></html>\r\n\r\n" );
                    while( a_socket.Write( sendb ) );
                    return 404;
                }
            }
            else
            {
                sendb->Write( ( const uint8_t * )"HTTP/1.1 405 METHOD NOT ALLOWED\r\n" );
                sendb->Write( ( const uint8_t * )"Allow: GET, HEAD\r\n" );
                sendb->Write( ( const uint8_t * )"Content-type: text/html\r\n" );
                sendb->Write( ( const uint8_t * )"Content-length: 59\r\n\r\n" );
                sendb->Write( ( const uint8_t * )"<html><head><center>Not Allowed!</center></head></html>\r\n\r\n" );
                while( a_socket.Write( sendb ) );
                return 405;
            }
        }

        void Print()
        {
            utils::Lock  lock( this );
            ::std::shared_ptr< KeyValuePair< ::std::string, ::std::string > > start = m_meta;
            printf( " [*] Remote: %s:%d\n", m_host.c_str(), m_port );
            printf( " [>] %s %s %s\n", m_method.c_str(), m_uri.c_str(), m_version.c_str() );
            while( start )
            {
                printf( " [>] %s: %s\n", start->Key().c_str(), start->Value().c_str() );
                start = start->Next();
            }
        }

        void Log( LogFile &a_logger )
        {
            utils::Lock  lock( this );
            utils::Lock  valueLock( &a_logger );
            char buffer[ 32 ];
            ::std::shared_ptr< KeyValuePair< ::std::string, ::std::string > > start = m_meta;

            snprintf( buffer, sizeof( buffer ), "%u", m_port );
            a_logger.Log( m_host, true, false );
            a_logger.Log( ":", false, false );
            a_logger.Log( buffer, false, false );
            a_logger.Log( " - ", false, false );
            a_logger.Log( m_method, false, false );
            a_logger.Log( " ", false, false );
            a_logger.Log( m_uri, false, false );
            a_logger.Log( " ", false, false );
            a_logger.Log( m_version, false, true );

            while( start )
            {
                a_logger.Log( m_host, true, false );
                a_logger.Log( ":", false, false );
                a_logger.Log( buffer, false, false );
                a_logger.Log( " - ", false, false );
                a_logger.Log( start->Key(), false, false );
                a_logger.Log( " = ", false, false );
                a_logger.Log( start->Value(), false, true );
                start = start->Next();
            }

            if( m_body.length() > 0 )
            {
                a_logger.Log( m_host, true, false );
                a_logger.Log( ":", false, false );
                a_logger.Log( buffer, false, false );
                a_logger.Log( " - ", false, false );
                a_logger.Log( m_body, false, true );
            }
        }
};

void *ProcessClient( void *a_client )
{
    ThreadCTX   *context = ( ThreadCTX * ) a_client;
    HttpRequest *httpRequest = new HttpRequest();
    ::std::string      *host = new ::std::string();
    uint32_t       port;

    if( ( NULL == context          ) ||
       !( context->settings        ) ||
       !( context->logger          ) ||
       !( context->socket          ) ||
       !( context->socket->Valid() ) ||
        ( NULL == httpRequest      ) ||
        ( NULL == host             ) )
    {
        printf( " [!] Client processing failed\n" );
        if( httpRequest ) delete httpRequest;
        if( host ) delete host;
        pthread_exit( NULL );
    }

    context->socket->GetRemoteAddress( *host, port );
    {
        utils::Lock lock( &( *( context->logger ) ) );
        context->logger->Log( *host, true, false );
        context->logger->Log( ":", false, false );
        context->logger->Log( port, false, false );
        context->logger->Log( " - Connected", false, true );
    }

    utils::Lock *lock = new Lock( context );

    printf( " [+] Processing client\n" );
    while( context->socket->Valid() && httpRequest->Read( *( context->socket ) ) )
    {
        printf( " [+] Got HTTP request\n" );
        ::std::shared_ptr< ::std::string > fileName   = ::std::make_shared< ::std::string >();
        ::std::shared_ptr< ::std::string > fileType   = ::std::make_shared< ::std::string >();
        ::std::shared_ptr< ::std::string > mimeType   = ::std::make_shared< ::std::string >();
        ::std::shared_ptr< ::std::string > hostHome   = ::std::make_shared< ::std::string >();
        ::std::shared_ptr< ::std::string > defaultDoc = ::std::make_shared< ::std::string >();
        if( fileName && fileType && mimeType && hostHome && defaultDoc )
        {
            int response = 0;
            httpRequest->Print();
            httpRequest->Log( *( context->logger ) );

            if( ( context->settings->ReadValue( "path", httpRequest->Host().c_str(), *hostHome ) ||
                  context->settings->ReadValue( "path", "default", *hostHome ) ) &&
                ( context->settings->ReadValue( "document", httpRequest->Host().c_str(), *defaultDoc ) ||
                  context->settings->ReadValue( "document", "default", *defaultDoc ) ) )
            {
                *fileName = httpRequest->Uri();
                // Decode the URI and lookup the matching mime-type or use the default
                if( !UriDecode( *hostHome, *defaultDoc, *fileName, *fileType ) ||
                    ( !context->settings->ReadValue( "mime-types", (*fileType).c_str(), *mimeType ) 
                    && !context->settings->ReadValue( "mime-types", DEFMIME, *mimeType ) ) )
                {
                    fileName->clear();
                    mimeType->clear();
                }
            }

            response = httpRequest->Respond( *( context->socket ), *fileName, *mimeType );

            printf( " [+] Response: %d\n", response );
            {
                utils::Lock lock( &( *( context->logger ) ) );
                context->logger->Log( *host, true, false );
                context->logger->Log( ":", false, false );
                context->logger->Log( port, false, false );
                context->logger->Log( " - Response: ", false, false );
                if( response > 0 )
                {
                    context->logger->Log( response, false, true );
                }
                else
                {
                    context->logger->Log( "INTERNAL ERROR", false, true );
                }
            }
        }
    }

    {
        utils::Lock lock( &( *( context->logger ) ) );
        context->logger->Log( *host, true, false );
        context->logger->Log( ":", false, false );
        context->logger->Log( port, false, false );
        context->logger->Log( " - Disconnected", false, true );
    }
    delete httpRequest;
    httpRequest = NULL;

    printf( " [+] Finished processing client\n" );
    context->socket->Shutdown();
    printf( " [+] Thread exiting\n" );
    delete lock;
    pthread_exit( NULL );
}

uint8_t CharToHex( char a_value )
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

uint32_t UriDecode( ::std::string &a_uri, ::std::string &a_ext )
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
                    char a = CharToHex( a_uri[ i + 1 ] );
                    char b = CharToHex( a_uri[ i + 2 ] );
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

bool UriDecode( ::std::string &a_base, ::std::string &a_defaultDoc, ::std::string &a_uri, ::std::string &a_ext )
{
    printf( " [*] Decoding: [%s]%s\n", a_base.c_str(), a_uri.c_str() );
    while( UriDecode( a_uri, a_ext ) != 0 );

    ::std::string newUri;
    newUri = a_base;
    if( ( newUri.length() > 0 ) && ( '/' != newUri[ newUri.length() - 1 ] ) )
    {
        newUri += '/';
    }
    newUri += a_uri;
    if( ( newUri.length() > 0 ) && ( '/' == newUri[ newUri.length() - 1 ] ) )
    {
        while( UriDecode( a_defaultDoc, a_ext ) != 0 );
        newUri += a_defaultDoc;
    }
    if( a_ext.length() == 0 )
    {
        a_ext = DEFMIME;
    }
    a_uri = newUri;

    if( a_uri.length() > 0 )
    {
        printf( " [*] Decoded:  %s [%s]\n", a_uri.c_str(), a_ext.c_str() );
        return true;
    }
    printf( " [!] URI decode failed!\n" );
    return false;
}
