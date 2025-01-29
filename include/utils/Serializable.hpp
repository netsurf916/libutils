/**
    Serializable.hpp : Serializable interface definitions
    Copyright 2015-2016 Daniel Wilson
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
        enum Types
        {
            KeyValuePair = 0xF6, // 246
            BlockHeader  = 0xF7, // 247
            Block        = 0xF8, // 248
            NetInfo      = 0xF9, // 249
            Primitive    = 0xFA, // 250
            BitMask      = 0xFB, // 251
            VirtualFile  = 0xFC, // 252
            FileBlock    = 0xFD, // 253
            File         = 0xFE, // 254
            String       = 0xFF, // 255
            Marker       = 0xCA, // 202 [11001010]
        };
    }
    typedef SerializableTypes::Types SerializableType;

    class Serializable : virtual public Lockable
    {
        public:
            virtual uint8_t Type() = 0;
            virtual bool    Serialize  ( Writable &a_out ) = 0;
            virtual bool    Deserialize( Readable &a_in  ) = 0;

        protected:
            virtual ~Serializable() {}

            bool SerializeType( Writable &a_out );
            bool DeserializeType( Readable &a_in );

        public:
            // Serialization helper functions
            uint8_t  ToNetwork  ( uint8_t  a_value );
            uint8_t  FromNetwork( uint8_t  a_value );
            uint16_t ToNetwork  ( uint16_t a_value );
            uint16_t FromNetwork( uint16_t a_value );
            uint32_t ToNetwork  ( uint32_t a_value );
            uint32_t FromNetwork( uint32_t a_value );
            uint64_t ToNetwork  ( uint64_t a_value );
            uint64_t FromNetwork( uint64_t a_value );
    };
}

#endif // _SERIALIZABLE_HPP_
