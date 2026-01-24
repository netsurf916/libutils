/**
    Socket.hpp : Socket class definition
    Description: Socket class for TCP/UDP networking.
    Copyright 2014-2026 Daniel Wilson
    SPDX-License-Identifier: MIT
*/

#include <utils/Types.hpp>
#include <utils/Buffer.hpp>
#include <utils/BitMask.hpp>
#include <utils/Readable.hpp>
#include <utils/Writable.hpp>
#include <memory>
#include <netinet/in.h>
#include <string>

#pragma once

#ifndef _SOCKET_HPP_
#define _SOCKET_HPP_

namespace utils
{
    namespace SocketFlags
    {
        /**
         * @brief Socket flag values for configuration.
         * @note Values are bit flags and may be combined.
         */
        enum Flags : uint8_t
        {
            Server    = 0,
            TCP       = 1,
            TcpClient = ( BIT(TCP)                             ),
            TcpServer = ( BIT(TCP) | BIT(Server)               ),
            UdpClient = ( 0                                    ),
            UdpServer = (            BIT(Server)               ),
        };
    }
    typedef SocketFlags::Flags SocketFlag;

    /**
     * @brief TCP/UDP socket wrapper implementing Readable/Writable.
     * @details Provides basic blocking/non-blocking reads and writes on a
     *          socket descriptor and handles client/server initialization.
     * @note Not safe for concurrent access without external synchronization.
     */
    class Socket : public Readable,
                   public Writable
    {
        private:
            int32_t  m_sockfd;
            bool     m_valid;
            int32_t  m_error;
            BitMask  m_flags;

        public:
            /**
             * @brief Construct a socket wrapper for an existing descriptor.
             * @param a_sockfd Socket file descriptor.
             * @param a_flags Socket flags (e.g., TcpServer, UdpClient).
             */
            Socket( int32_t a_sockfd, uint32_t a_flags = 0 );
            /**
             * @brief Construct and initialize a socket connection.
             * @param a_address Hostname or address string; must be non-null.
             * @param a_port Port number.
             * @param a_flags Socket flags (e.g., TcpClient, TcpServer).
             */
            Socket( const char *a_address, uint32_t a_port, uint32_t a_flags = 0 );
            /**
             * @brief Destroy the socket wrapper and close the descriptor.
             */
            ~Socket();

        private:
            /**
             * @brief Initialize the socket using host/service strings.
             * @param a_address Hostname or address string; must be non-null.
             * @param a_service Service or port string; must be non-null.
             * @return 0 on success; non-zero on failure.
             */
            int32_t Initialize( const char *a_address, const char *a_service );

        public:
            /**
             * @brief Check whether the socket is valid.
             * @return True if the socket is initialized; false otherwise.
             */
            bool    Valid();
            /**
             * @brief Accept an incoming connection on a listening socket.
             * @param a_address Output remote address string.
             * @param a_port Output remote port number.
             * @return New socket instance on success; nullptr on failure.
             */
            ::std::shared_ptr< Socket > Accept( ::std::string &a_address, uint32_t &a_port );
            /**
             * @brief Retrieve the last socket error code.
             * @return Error code value.
             */
            int32_t LastError();
            /**
             * @brief Shut down the socket for reading and writing.
             */
            void    Shutdown();

            // Read functions
            /**
             * @brief Check if the socket is readable.
             * @return True if readable; false otherwise.
             */
            bool     IsReadable() noexcept final;
            /**
             * @brief Read a single byte from the socket.
             * @param a_value Output byte to receive data.
             * @param a_block If true, the call may block for data.
             * @return True if a byte was read; false on EOF or error.
             */
            bool     Read( uint8_t &a_value, bool a_block = false ) noexcept final;
            /**
             * @brief Read up to a_length bytes from the socket.
             * @param a_value Destination buffer; must be non-null when a_length > 0.
             * @param a_length Number of bytes requested.
             * @param a_block If true, the call may block for data.
             * @return Number of bytes read; 0 on EOF or error.
             */
            uint32_t Read( uint8_t *a_value, uint32_t a_length, bool a_block = false ) noexcept final;
            /**
             * @brief Read socket data into a Buffer.
             * @param a_buffer Destination buffer; must be non-null.
             * @param a_block If true, the call may block for data.
             * @return True if any data was read; false otherwise.
             */
            bool     Read( ::std::shared_ptr< Buffer > &a_buffer, bool a_block = false ) noexcept;
            /**
             * @brief Peek at the next byte without consuming it.
             * @param a_value Output byte to receive data.
             * @return True if a byte is available; false otherwise.
             */
            bool     Peek( uint8_t &a_value ) noexcept final;
            /**
             * @brief Peek up to a_length bytes without consuming them.
             * @param a_value Destination buffer; must be non-null when a_length > 0.
             * @param a_length Number of bytes requested.
             * @return Number of bytes copied; 0 on EOF or error.
             */
            uint32_t Peek( uint8_t *a_value, uint32_t a_length ) noexcept final;
            /**
             * @brief Peek socket data into a Buffer without consuming.
             * @param a_buffer Destination buffer; must be non-null.
             * @return True if any data was peeked; false otherwise.
             */
            bool     Peek( ::std::shared_ptr< Buffer > &a_buffer ) noexcept;

            // Write functions
            /**
             * @brief Check if the socket is writable.
             * @return True if writable; false otherwise.
             */
            bool     IsWritable() noexcept final;
            /**
             * @brief Write a single byte to the socket.
             * @param a_value Byte to write.
             * @return True if written; false on error.
             */
            bool     Write( const uint8_t &a_value ) noexcept final;
            /**
             * @brief Write bytes to the socket.
             * @param a_value Source buffer; must be non-null when a_length > 0.
             * @param a_length Number of bytes to write.
             * @return Number of bytes written; 0 on error.
             */
            uint32_t Write( const uint8_t *a_value, uint32_t a_length ) noexcept final;
            /**
             * @brief Write buffer contents to the socket.
             * @param a_buffer Source buffer; must be non-null.
             * @return True if any data was written; false otherwise.
             */
            bool     Write( ::std::shared_ptr< Buffer > &a_buffer ) noexcept;
    };
}

#endif // _SOCKET_HPP_
