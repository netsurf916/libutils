/**
    Serializable.cpp : Serializable interface implementation
    Copyright 2015-2025 Daniel Wilson
*/

#include <utils/Serializable.hpp>

namespace utils
{
    bool Serializable::SerializeType( Writable &a_out )
    {
        utils::Lock lock( this );
        utils::Lock valueLock( &a_out );
        bool ok = a_out.IsWritable();
        uint8_t dataType = Type();
        ok = ok && ( sizeof( dataType ) == a_out.Write( dataType ) );
        return ok;
    }

    bool Serializable::DeserializeType( Readable &a_in )
    {
        utils::Lock lock( this );
        utils::Lock valueLock( &a_in );
        bool ok = a_in.IsReadable();
        uint8_t dataType = 0;
        ok = ok && ( sizeof( dataType ) == a_in.Peek( dataType ) );
        ok = ok && ( Type() == dataType );
        ok = ok && ( sizeof( dataType ) == a_in.Read( dataType ) );
        return ok;
    }

    // Serialization helper functions
    uint8_t  Serializable::ToNetwork  ( uint8_t  a_value ) { return a_value; }
    uint8_t  Serializable::FromNetwork( uint8_t  a_value ) { return a_value; }
    uint16_t Serializable::ToNetwork  ( uint16_t a_value ) { return htons( a_value ); }
    uint16_t Serializable::FromNetwork( uint16_t a_value ) { return ntohs( a_value ); }
    uint32_t Serializable::ToNetwork  ( uint32_t a_value ) { return htonl( a_value ); }
    uint32_t Serializable::FromNetwork( uint32_t a_value ) { return ntohl( a_value ); }

    uint64_t Serializable::ToNetwork  ( uint64_t a_value )
    {
        uint16_t testValue = 1;
        if( testValue == ToNetwork( testValue ) )
        {
            return a_value;
        }
        uint64_t value = 0;
        ( ( uint8_t * )&value )[ 0 ] = ( ( uint8_t * )&a_value )[ 7 ];
        ( ( uint8_t * )&value )[ 1 ] = ( ( uint8_t * )&a_value )[ 6 ];
        ( ( uint8_t * )&value )[ 2 ] = ( ( uint8_t * )&a_value )[ 5 ];
        ( ( uint8_t * )&value )[ 3 ] = ( ( uint8_t * )&a_value )[ 4 ];
        ( ( uint8_t * )&value )[ 4 ] = ( ( uint8_t * )&a_value )[ 3 ];
        ( ( uint8_t * )&value )[ 5 ] = ( ( uint8_t * )&a_value )[ 2 ];
        ( ( uint8_t * )&value )[ 6 ] = ( ( uint8_t * )&a_value )[ 1 ];
        ( ( uint8_t * )&value )[ 7 ] = ( ( uint8_t * )&a_value )[ 0 ];
        return value;
    }

    uint64_t Serializable::FromNetwork( uint64_t a_value )
    {
        uint16_t testValue = 1;
        if( testValue == FromNetwork( testValue ) )
        {
            return a_value;
        }
        uint64_t value = 0;
        ( (uint8_t *)&value )[ 0 ] = ( (uint8_t *)&a_value )[ 7 ];
        ( (uint8_t *)&value )[ 1 ] = ( (uint8_t *)&a_value )[ 6 ];
        ( (uint8_t *)&value )[ 2 ] = ( (uint8_t *)&a_value )[ 5 ];
        ( (uint8_t *)&value )[ 3 ] = ( (uint8_t *)&a_value )[ 4 ];
        ( (uint8_t *)&value )[ 4 ] = ( (uint8_t *)&a_value )[ 3 ];
        ( (uint8_t *)&value )[ 5 ] = ( (uint8_t *)&a_value )[ 2 ];
        ( (uint8_t *)&value )[ 6 ] = ( (uint8_t *)&a_value )[ 1 ];
        ( (uint8_t *)&value )[ 7 ] = ( (uint8_t *)&a_value )[ 0 ];
        return value;
    }
}

