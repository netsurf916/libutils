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

#define NUMTHREADS 16
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
};

void *ProcessClient( void *a_client );
void  PrintHttpRequest( shared_ptr< HttpRequest > &a_request );

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

    #ifdef USE_SSL
    string keyfile;
    string certfile;
    bool useTls = settings->ReadValue( "settings", "keyfile",  keyfile );
    useTls = useTls && settings->ReadValue( "settings", "certfile", certfile );
    useTls = useTls && ( keyfile.length() > 0 ) && ( certfile.length() > 0 );
    #endif // USE_SSL

    // Try for root if the requested port is < 1024
    uid_t runningAs = getuid();
    bool  gotRoot = ( ( 0 != runningAs ) && ( stoi( port ) < 1024 ) && ( 0 == setuid( 0 ) ) );

    // Start the listener
    uint32_t flags = SocketFlags::TcpServer;
    #ifdef USE_SSL
    if( useTls )
    {
        flags = SocketFlags::TlsServer;
    }
    #endif // USE_SSL
    shared_ptr< Socket > listener =
        #ifdef USE_SSL
        make_shared< Socket >( address.c_str(), stoi( port ), flags, keyfile.c_str(), certfile.c_str() );
        #else // !USE_SSL
        make_shared< Socket >( address.c_str(), stoi( port ), flags );
        #endif // USE_SSL
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

    shared_ptr< Thread< ThreadCTX > > clients[ NUMTHREADS ];

    while( listener->Valid() )
    {
        string   address;
        uint32_t port = 0;
        shared_ptr< Socket > client = listener->Accept( address, port );
        if( client )
        {
            bool found = false;
            printf( " [*] Client connected: %s:%u\n", address.c_str(), port );
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
                        // Set to true here because it doesn't make sense to retry if the following fails
                        found = true;
                        clients[ c ] = make_shared< Thread< ThreadCTX > >( ProcessClient );
                        if( clients[ c ] && clients[ c ]->GetContext() )
                        {
                            clients[ c ]->GetContext()->socket   = client;
                            clients[ c ]->GetContext()->logger   = logger;
                            clients[ c ]->GetContext()->settings = settings;
                            clients[ c ]->GetContext()->address  = address;
                            clients[ c ]->GetContext()->port     = port;
                            clients[ c ]->GetContext()->id       = c;
                            if( clients[ c ]->GetContext()->socket &&
                                clients[ c ]->GetContext()->socket->Valid() )
                            {
                                clients[ c ]->Start();
                            }
                            else
                            {
                                clients[ c ].reset();
                            }
                        }
                    }
                }
                if( !found )
                {
                    // Wait for 1/10th of a second for a thread to finish
                    usleep( 100000 );
                }
            }
            printf( " [+] Client thread started (%s:%u)\n", address.c_str(), port );
        }
        else
        {
            // Wait for 10ms for a client connection
            usleep( 10000 );
        }
        // Iterate through the threads to free up any that have finished.
        // Calling reset() will destruct the object and call ~Thread() which
        // will call pthread_kill() and pthread_join().
        for( uint32_t c = 0; c < NUMTHREADS; ++c )
        {
            if( clients[ c ] && !( clients[ c ]->IsRunning() ) )
            {
                clients[ c ].reset();
            }
        }
    }

    pthread_exit( nullptr );
}

void *ProcessClient( void *a_client )
{
    ThreadCTX *context = ( ThreadCTX * ) a_client;
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
    #ifdef USE_SSL
    context->socket->Start_SSL();
    #endif // USE_SSL

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
        int response = 0;
        printf( " [*] Remote: %s:%u\n", context->address.c_str(), context->port );
        PrintHttpRequest( httpRequest );
        httpRequest->Log( *( context->logger ) );

        if( ( context->settings->ReadValue( "path", httpRequest->Host().c_str(), hostHome ) ||
              context->settings->ReadValue( "path", "default", hostHome ) ) &&
            ( context->settings->ReadValue( "document", httpRequest->Host().c_str(), defaultDoc ) ||
              context->settings->ReadValue( "document", "default", defaultDoc ) ) )
        {
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

        response = httpRequest->Respond( context->socket, fileName, mimeType );

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
    pthread_exit( nullptr );
}

void PrintHttpRequest( shared_ptr< HttpRequest > &a_request )
{
    if( a_request == nullptr )
    {
        return;
    }
    shared_ptr< KeyValuePair< string, string > > start = a_request->Meta();
    printf( " [+] %s %s %s\n", a_request->Method().c_str(), a_request->Uri().c_str(), a_request->Version().c_str() );
    while( start )
    {
        printf( " [+] %s: %s\n", start->Key().c_str(), start->Value().c_str() );
        start = start->Next();
    }
}
