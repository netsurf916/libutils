#include <utils/Utils.hpp>
#include <unistd.h>
#include <thread>

using namespace utils;

int main( int argc, char *argv[] )
{
    UNUSED( argc ); UNUSED( argv );
    if( argc < 2 )
    {
        printf( "[*] Usage: %s \"<string>\"\n", argv[ 0 ] );
        return 0;
    }

    SmartPointer< Socket > client = new Socket( "127.0.0.1", 6000, SocketFlag::TcpClient );

    if( client && client->Valid() )
    {
        for( int32_t i = 1; i < argc; ++i )
        {
            String txData( argv[ i ] );
            if( txData.Serialize( *client ) )
            {
                printf( "[*] --> \"%s\" --> OK\n", argv[ i ] );
            }
            else
            {
                printf( "[*] --x \"%s\" --x ERROR\n", argv[ i ] );
            }
        }
    }

    return 0;
}
