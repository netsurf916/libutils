/**
    Serializable.hpp : Serializable interface definitions
    Copyright 2015-2019 Daniel Wilson
*/

#pragma once

#ifndef _SERIALIZABLE_HPP_
#define _SERIALIZABLE_HPP_

#include <utils/Types.hpp>
#include <utils/Readable.hpp>
#include <utils/Writable.hpp>
#include <utils/Lock.hpp>
#include <netinet/in.h>

namespace utils
{
    namespace SerializableTypes
    {
        enum Types : uint8_t
        {
            None         = 0x00, //   0
            Marker       = 0xCA, // 202 [11001010]
            KeyValuePair = 0xF8, // 246
            NetInfo      = 0xFA, // 249
            Primitive    = 0xFC, // 250
            BitMask      = 0xFD, // 251
            File         = 0xFE, // 254
            String       = 0xFF, // 255
        };
    }
    typedef SerializableTypes::Types SerializableType;

    class Serializable : virtual public Lockable
    {
        public:
            virtual uint8_t Type() noexcept = 0;
            virtual bool    Serialize  ( Writable &a_out ) noexcept = 0;
            virtual bool    Deserialize( Readable &a_in  ) noexcept = 0;

        protected:
            virtual ~Serializable() {}

            bool SerializeType( Writable &a_out )
            {
                ::utils::Lock lock( this );
                ::utils::Lock valueLock( &a_out );
                bool ok = a_out.IsWritable();
                uint8_t dataType = Type();
                ok = ok && a_out.Write( dataType );
                return ok;
            }

            bool DeserializeType( Readable &a_in )
            {
                ::utils::Lock lock( this );
                ::utils::Lock valueLock( &a_in );
                bool ok = a_in.IsReadable();
                uint8_t dataType = 0;
                ok = ok && a_in.Peek( dataType );
                ok = ok && ( Type() == dataType );
                ok = ok && a_in.Read( dataType );
                return ok;
            }

        public:
            // Serialization helper functions
            static uint8_t  ToNetwork  ( uint8_t  a_value ) { return a_value; }
            static uint8_t  FromNetwork( uint8_t  a_value ) { return a_value; }
            static uint16_t ToNetwork  ( uint16_t a_value ) { return htons( a_value ); }
            static uint16_t FromNetwork( uint16_t a_value ) { return ntohs( a_value ); }
            static uint32_t ToNetwork  ( uint32_t a_value ) { return htonl( a_value ); }
            static uint32_t FromNetwork( uint32_t a_value ) { return ntohl( a_value ); }

            static uint64_t ToNetwork  ( uint64_t a_value )
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

            static uint64_t FromNetwork( uint64_t a_value )
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
    };
}

#endif // _SERIALIZABLE_HPP_
