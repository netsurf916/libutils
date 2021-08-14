#include <utils/Utils.hpp>

#include <unistd.h>
#include <stdio.h>
#include <memory>
#include <cstring>
#include <chrono>

using namespace utils;

bool TestNetInfo()
{
    ::std::shared_ptr< NetInfo > interfaces = NetInfo::GetInterfaces();
    for( interfaces = NetInfo::GetInterfaces(); nullptr != interfaces; interfaces = interfaces->Next() )
    {
        if( !interfaces->Name() ) { return false; }
    }
    return true;
}

bool TestStaque()
{
    bool ok = true;
    uint32_t temp = 0;
    Staque< uint32_t > staque;
    for( uint32_t i = 0; i < 20; ++i )
    {
        ok = ok && staque.Enqueue( i );
    }
    for( uint32_t i = 0; i < 20; ++i )
    {
        ok = ok && staque.GetAt( i, temp ) && ( i == temp );
    }
    staque.Clear();
    for( uint32_t i = 0; i < 20; ++i )
    {
        ok = ok && staque.Push( i );
    }
    for( uint32_t i = 0; i < 20; ++i )
    {
        ok = ok && staque.GetAt( i, temp ) && ( ( 19 - i ) == temp );
    }
    return ok;
}

bool TestBuffer()
{
    SmartPointer< Buffer > temp = new Buffer();
    bool ok = temp;
    uint8_t ctemp = 0;
    for( uint32_t i = 0; ( i < 1024 ) && ok; ++i )
    {
        ok = ok && temp->Write( static_cast< uint8_t >( 'A' ) );
    }
    for( uint32_t i = 0; ( i < 2048 ) && ok; ++i )
    {
        if( i < 1024 )
        {
            ok = ok && temp->Read( ctemp );
        }
        else
        {
            ok = ok && !( temp->Read( ctemp ) );
        }
    }
    return ok;
}

bool TestSocket()
{
    String transferMe( "Hello Network!" );
    String receiveMe;
    Primitive< uint32_t > transferMe2( 1024 );
    Primitive< uint32_t > receiveMe2;
    BitMask transferMe3( 0 );
    BitMask receiveMe3;
    transferMe3.SetBit( 5, 1 );
    transferMe3.SetBit( 2, 1 );
    SmartPointer< Socket > listener = new Socket( "127.0.0.1", 6000, SocketFlag::TcpServer );
    SmartPointer< Socket > client   = new Socket( "127.0.0.1", 6000, SocketFlag::TcpClient );
    bool ok = listener && client && listener->Valid() && client->Valid();
    if( ok )
    {
        int32_t clientfd = -1;
        if( listener->Accept( clientfd ) )
        {
            SmartPointer< Socket > connection = new Socket( clientfd, SocketFlag::TcpClient );
            ok = connection && connection->Valid();
            ok = ok && transferMe.Serialize( *connection );
            ok = ok && transferMe2.Serialize( *connection );
            ok = ok && transferMe3.Serialize( *connection );
            ok = ok && receiveMe.Deserialize( *client );
            ok = ok && receiveMe2.Deserialize( *client );
            ok = ok && receiveMe3.Deserialize( *client );
        }
    }
    ok = ok && ( receiveMe  == transferMe  );
    ok = ok && ( receiveMe2 == transferMe2 );
    ok = ok && ( receiveMe3 == transferMe3 );
    return ok;
}

bool TestFile()
{
    File fileTest( "test.out", FileMode::DefaultWrite );
    fileTest.Delete();
    String fileWriteTest( "Hello Filesystem!" );
    bool ok = fileWriteTest.Serialize( fileTest );
    if( ok )
    {
        fileTest.SetMode( FileMode::DefaultRead );
        fileTest.Seek( 0 );
    }
    String fileReadTest;
    ok = ok && fileReadTest.Deserialize( fileTest );
    ok = ok && ( fileReadTest == fileWriteTest );
    fileTest.Delete();
    return ok;
}

void RunTest( const char *a_name, bool(*a_test)() )
{
    printf( "[+] Testing: %s\n", a_name );
    auto start = ::std::chrono::steady_clock::now();
    bool result = a_test();
    auto stop  = ::std::chrono::steady_clock::now();
    printf( "    > Result: %s", ( result )? "OK": "ERROR" );
    printf( " [%8.0f ns]\n", ::std::chrono::duration< double, ::std::nano >( stop - start ).count() );
}

int main( int argc, char *argv[] )
{
    UNUSED( argc );
    UNUSED( argv );

    RunTest( "NetInfo", TestNetInfo );
    RunTest( "Staque",  TestStaque  );
    RunTest( "Buffer",  TestBuffer  );
    RunTest( "Socket",  TestSocket  );
    RunTest( "File",    TestFile    );

    return 0;
}
