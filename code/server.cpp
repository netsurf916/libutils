#include <utils/Utils.hpp>
#include <unistd.h>
#include <thread>

using namespace utils;

void ProcessClient( int32_t a_clientfd )
{
    SmartPointer< Socket > client = new Socket( a_clientfd, SocketFlag::TcpClient );
    String   remoteIP;
    uint32_t remotePort =  0;
    int32_t  timeout    = 20; // 2 seconds to start talking
    if( client && client->Valid() )
    {
        client->GetRemoteAddress( remoteIP, remotePort );
        printf( "[*] Connection open  : %s:%u\n", ( const char * )remoteIP, remotePort );
    }
    while( client && client->Valid() )
    {
        String rxData;
        if( rxData.Deserialize( *client ) )
        {
            printf( "[>] [%s:%u] %s\n", ( const char * )remoteIP, remotePort, ( const char * )rxData );
            timeout = 100; // 10 seconds to continue talking
        }
        else if( timeout <= 0 )
        {
            client->Shutdown();
        }
        else
        {
            usleep( 100000 );
            --timeout;
        }
    }
    if( remotePort > 0 )
    {
        printf( "[*] Connection closed: %s:%u\n", ( const char * )remoteIP, remotePort );
    }
}

int main( int argc, char *argv[] )
{
    UNUSED( argc ); UNUSED( argv );
    SmartPointer< Socket > server = new Socket( "127.0.0.1", 6000, SocketFlag::TcpServer );

    while( server )
    {
        int32_t clientfd = -1;
        if( server->Accept( clientfd ) )
        {
            std::thread( ProcessClient, clientfd ).detach();
        }
        else
        {
            usleep( 100000 );
        }
    }

    return 0;
}
