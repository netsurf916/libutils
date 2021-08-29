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
#include <utils/HttpRequest.hpp>
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
                *mimeType = DEFMIME;
                // Decode the URI and lookup the matching mime-type or use the default
                if( !HttpHelpers::UriDecode( *hostHome, *defaultDoc, *fileName, *fileType, *mimeType ) ||
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

