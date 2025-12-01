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

#define NUMTHREADS 64
#define DEFMIME    "none" // Make sure this is defined in the ini file

using namespace utils;
using namespace std;

struct ThreadCTX : public Lockable
{
    shared_ptr< Socket >  socket;
    shared_ptr< LogFile > logger;
    shared_ptr< IniFile > settings;
    string                address;
    uint32_t              port;
    uint32_t              id;
    bool                  running;
};

void *ProcessClient( void *a_client );

int main( int argc, char *argv[] )
{
    UNUSED( argc );
    UNUSED( argv );

    shared_ptr< LogFile > logger   = make_shared< LogFile >( "httpd.log" );
    shared_ptr< IniFile > settings = make_shared< IniFile >( "httpd.ini" );

    if( !( logger ) || !( settings ) )
    {
        return 0;
    }

    string port;
    string address;
    settings->ReadValue( "settings", "port",    port );
    settings->ReadValue( "settings", "address", address );

    if( ( 0 == port.length() ) || ( 0 == address.length() ) )
    {
        printf( " [!] Failed to read configuration\n" );
        return 0;
    }

    // Try for root if the requested port is < 1024
    uid_t runningAs = getuid();
    bool  gotRoot = ( ( 0 != runningAs ) && ( stoi( port ) < 1024 ) && ( 0 == setuid( 0 ) ) );

    // Start the listener
    uint32_t flags = SocketFlags::TcpServer;
    shared_ptr< Socket > listener =
        make_shared< Socket >( address.c_str(), stoi( port ), flags );
    if( !listener || !listener->Valid() )
    {
        printf( " [!] Error listening on: %s:%s\n", address.c_str(), port.c_str() );
        return 0;
    }

    // Give up root
    if( gotRoot )
    {
        setuid( runningAs );
    }

    printf( " [+] Listening for incoming connnections on: %s:%s\n",
        address.c_str(), port.c_str() );
    logger->Log( "Listening for incoming connections on: ", true, false );
    logger->Log( address, false, false );
    logger->Log( ":", false, false );
    logger->Log( port, false, true );

    // Track the available threads
    uint16_t availableThreads = NUMTHREADS;
    shared_ptr< Thread< ThreadCTX > > clients[ NUMTHREADS ];

    while( listener->Valid() )
    {
        string   address;
        uint32_t port = 0;
        shared_ptr< Socket > client = ( availableThreads > 0 )? listener->Accept( address, port ): nullptr;

        if( client && client->Valid() && listener->Valid() )
        {
            printf( " [*] Client connected: %s:%u\n", address.c_str(), port );
            for( uint32_t c = 0; ( c < NUMTHREADS ); ++c )
            {
                if( !( clients[ c ] ) )
                {
                    clients[ c ] = make_shared< Thread< ThreadCTX > >( ProcessClient );
                    if( clients[ c ] && clients[ c ]->GetContext() )
                    {
                        clients[ c ]->GetContext()->socket   = client;
                        clients[ c ]->GetContext()->logger   = logger;
                        clients[ c ]->GetContext()->settings = settings;
                        clients[ c ]->GetContext()->address  = address;
                        clients[ c ]->GetContext()->port     = port;
                        clients[ c ]->GetContext()->id       = c;
                        clients[ c ]->GetContext()->running  = true;
                        if( clients[ c ]->GetContext()->socket &&
                            clients[ c ]->GetContext()->socket->Valid() )
                        {
                            clients[ c ]->Start();
                            client = nullptr;
                            --availableThreads;
                        }
                        else
                        {
                            clients[ c ].reset();
                        }
                    }
                }
            }
        } else { sleep(1); } // 1 second delay in case there are no free threads
        printf( " [+] Client thread started (%s:%u)\n", address.c_str(), port );

        // Iterate through the threads to free up any that have finished.
        // Calling reset() will destruct the object and call ~Thread() which
        // will call pthread_kill() and pthread_join().
        for( uint32_t c = 0; c < NUMTHREADS; ++c )
        {
            if( clients[ c ] && !( clients[ c ]->GetContext()->running ) )
            {
                clients[ c ].reset();
                ++availableThreads;
            }
        }
    }

    pthread_exit( nullptr );
}

void *ProcessClient( void *a_clientCtx )
{
    ThreadCTX *context = ( ThreadCTX * ) a_clientCtx;
    shared_ptr< HttpRequest > httpRequest = make_shared< HttpRequest >();
    shared_ptr< utils::Lock > lock        = make_shared< Lock >( context );

    if( ( nullptr == context       ) ||
       !( context->settings        ) ||
       !( context->logger          ) ||
       !( context->socket          ) ||
       !( context->socket->Valid() ) ||
        ( nullptr == httpRequest   ) )
    {
        printf( " [!] Client processing failed\n" );
        pthread_exit( nullptr );
    }

    {
        utils::Lock logLock( context->logger.get() );
        context->logger->Log( context->address, true, false );
        context->logger->Log( ":", false, false );
        context->logger->Log( context->port, false, false );
        context->logger->Log( " - Connected", false, true );
    }

    printf( " [+] Processing client (id: %u)\n", context->id );

    if( context->socket->Valid() && httpRequest->Read( context->socket ) )
    {
        httpRequest->RemoteAddress() = context->address;
        httpRequest->RemotePort()    = context->port;
        printf( " [+] Got HTTP request\n" );
        string fileName;
        string fileType;
        string mimeType;
        string hostHome;
        string defaultDoc;
        string listDirs;
        bool   bListDirs = false;
        int response = 0;
        printf( " [*] Remote: %s:%u\n", context->address.c_str(), context->port );
        httpRequest->Log( *( context->logger ) );

        if( ( context->settings->ReadValue( "path", httpRequest->Host().c_str(), hostHome ) ||
              context->settings->ReadValue( "path", "default", hostHome ) ) &&
            ( context->settings->ReadValue( "document", httpRequest->Host().c_str(), defaultDoc ) ||
              context->settings->ReadValue( "document", "default", defaultDoc ) ) )
        {
            if( context->settings->ReadValue( "document", "directory", listDirs ) && ( listDirs == "list" ) )
            {
                bListDirs = true;
            }
            fileName = httpRequest->Uri();
            mimeType = DEFMIME;
            // Decode the URI and lookup the matching mime-type or use the default
            if( !HttpHelpers::UriDecode( hostHome, defaultDoc, fileName, fileType, mimeType ) ||
                ( !context->settings->ReadValue( "mime-types", fileType.c_str(), mimeType )
                && !context->settings->ReadValue( "mime-types", DEFMIME, mimeType ) ) )
            {
                fileName.clear();
                mimeType.clear();
            }
        }
        printf( " [*] Filename: %s; Mime: %s\n", fileName.c_str(), mimeType.c_str() );

        // Process internal operation requests
        if( mimeType == "internal" )
        {
            // Default mime type for internal responses
            mimeType = "text/plain";
            string operation = fileName;
            auto start = operation.rfind( '/' );
            auto end   = operation.rfind( '.' );
            operation = operation.substr( start + 1, end - start - 1 );
            if( operation.length() > 0 )
            {
                Tokens::MakeLower( operation );
                printf( " [@] Internal operation: %s\n", operation.c_str() );
                {
                    utils::Lock logLock( context->logger.get() );
                    context->logger->Log( context->address, true, false );
                    context->logger->Log( ":", false, false );
                    context->logger->Log( context->port, false, false );
                    context->logger->Log( " - Internal operation: ", false, false );
                    bool printable = true;
                    for( size_t i = 0; ( i < operation.length() ) && printable; ++i )
                    {
                        printable = Tokens::IsPrintable( operation[ i ] );
                    }
                    if( printable )
                    {
                        context->logger->Log( operation.c_str(), false, true );
                    }
                    else
                    {
                        context->logger->Log( "UNKNOWN", false, true );
                    }
                }
                if( "ip" == operation )
                {
                    // Reuse mime type: "text/plain"
                    httpRequest->Response() += context->address;
                }
                else if( "request" == operation )
                {
                    mimeType = "text/html";
                    shared_ptr< KeyValuePair< string, string > > meta = httpRequest->Meta();
                    httpRequest->Response() += "<html>\n <head>\n  <title>Client Request</title>\n </head>\n<body>";
                    httpRequest->Response() += "Client: ";
                    httpRequest->Response() += context->address;
                    httpRequest->Response() += ":";
                    httpRequest->Response() += to_string( context->port );
                    httpRequest->Response() += "<br><br>\n";
                    httpRequest->Response() += httpRequest->Method();
                    httpRequest->Response() += " ";
                    httpRequest->Response() += httpRequest->Uri();
                    httpRequest->Response() += " ";
                    httpRequest->Response() += httpRequest->Version();
                    httpRequest->Response() += "<br>\n";
                    httpRequest->Response() += "<table>\n";
                    while( meta )
                    {
                        httpRequest->Response() += " <tr>\n";
                        httpRequest->Response() += "  <td>";
                        httpRequest->Response() += meta->Key();
                        httpRequest->Response() += "</td>\n";
                        httpRequest->Response() += "  <td>";
                        httpRequest->Response() += meta->Value();
                        httpRequest->Response() += "</td>\n";
                        httpRequest->Response() += " </tr>\n";
                        meta = meta->Next();
                    }
                    httpRequest->Response() += "</table>\n";
                    httpRequest->Response() += "</body></html>\n";
                }
            }
        }

        response = httpRequest->Respond( context->socket, fileName, mimeType, bListDirs );

        printf( " [+] Response: %d\n", response );
        {
            utils::Lock logLock( context->logger.get() );
            context->logger->Log( context->address, true, false );
            context->logger->Log( ":", false, false );
            context->logger->Log( context->port, false, false );
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

    {
        utils::Lock logLock( context->logger.get() );
        context->logger->Log( context->address, true, false );
        context->logger->Log( ":", false, false );
        context->logger->Log( context->port, false, false );
        context->logger->Log( " - Disconnected", false, true );
    }

    printf( " [+] Finished processing client (%s:%u)\n", context->address.c_str(), context->port );
    context->socket->Shutdown();
    printf( " [+] Thread exiting (id: %u)\n", context->id );
    context->running = false;
    pthread_exit( nullptr );
}

