/**
    Serializable.hpp : Serializable interface definitions
    Description: Serializable interface for reading and writing.
    Copyright 2015-2026 Daniel Wilson
    SPDX-License-Identifier: MIT
*/

#pragma once

#ifndef _SERIALIZABLE_HPP_
#define _SERIALIZABLE_HPP_

#include <utils/Types.hpp>
#include <utils/Readable.hpp>
#include <utils/Writable.hpp>
#include <utils/Lock.hpp>

namespace utils
{
    namespace SerializableTypes
    {
        /**
         * @brief Type tags used in serialized streams.
         */
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

    /**
     * @brief Interface for serializable objects.
     * @details Provides hooks to serialize/deserialize into Readable/Writable
     *          streams, plus helpers for endian conversion.
     */
    class Serializable : virtual public Lockable
    {
        public:
            /**
             * @brief Get the serialized type tag for this object.
             * @return Type identifier byte.
             */
            virtual uint8_t Type() = 0;

            /**
             * @brief Serialize this object to a writable stream.
             * @param a_out Output stream to write to.
             * @return True on success; false on write failure.
             */
            virtual bool    Serialize  ( Writable &a_out ) = 0;

            /**
             * @brief Deserialize this object from a readable stream.
             * @param a_in Input stream to read from.
             * @return True on success; false on read/format failure.
             */
            virtual bool    Deserialize( Readable &a_in  ) = 0;

        protected:
            /**
             * @brief Protected virtual destructor for interface cleanup.
             */
            virtual ~Serializable() {}

            /**
             * @brief Serialize the type tag to the output stream.
             * @param a_out Output stream to write to.
             * @return True on success; false on write failure.
             */
            bool SerializeType  ( Writable &a_out );

            /**
             * @brief Read and validate the type tag from the input stream.
             * @param a_in Input stream to read from.
             * @return True if the type matches; false otherwise.
             */
            bool DeserializeType( Readable &a_in  );

        public:
            // Serialization helper functions

            /**
             * @brief Convert an 8-bit value to network byte order.
             * @param a_value Host-order value.
             * @return Network-order value.
             */
            static uint8_t  ToNetwork  ( uint8_t  a_value );

            /**
             * @brief Convert an 8-bit value from network byte order.
             * @param a_value Network-order value.
             * @return Host-order value.
             */
            static uint8_t  FromNetwork( uint8_t  a_value );

            /**
             * @brief Convert a 16-bit value to network byte order.
             * @param a_value Host-order value.
             * @return Network-order value.
             */
            static uint16_t ToNetwork  ( uint16_t a_value );

            /**
             * @brief Convert a 16-bit value from network byte order.
             * @param a_value Network-order value.
             * @return Host-order value.
             */
            static uint16_t FromNetwork( uint16_t a_value );

            /**
             * @brief Convert a 32-bit value to network byte order.
             * @param a_value Host-order value.
             * @return Network-order value.
             */
            static uint32_t ToNetwork  ( uint32_t a_value );

            /**
             * @brief Convert a 32-bit value from network byte order.
             * @param a_value Network-order value.
             * @return Host-order value.
             */
            static uint32_t FromNetwork( uint32_t a_value );

            /**
             * @brief Convert a 64-bit value to network byte order.
             * @param a_value Host-order value.
             * @return Network-order value.
             */
            static uint64_t ToNetwork  ( uint64_t a_value );

            /**
             * @brief Convert a 64-bit value from network byte order.
             * @param a_value Network-order value.
             * @return Host-order value.
             */
            static uint64_t FromNetwork( uint64_t a_value );
    };
}

#endif // _SERIALIZABLE_HPP_
