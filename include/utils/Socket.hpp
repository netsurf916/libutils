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

    class Socket : public Readable,
                   public Writable
    {
        private:
            int32_t  m_sockfd;
            bool     m_valid;
            int32_t  m_error;
            BitMask  m_flags;

        public:
            Socket( int32_t a_sockfd, uint32_t a_flags = 0 );
            Socket( const char *a_address, uint32_t a_port, uint32_t a_flags = 0 );
            ~Socket();

        private:
            int32_t Initialize( const char *a_address, const char *a_service );

        public:
            bool    Valid();
            ::std::shared_ptr< Socket > Accept( ::std::string &a_address, uint32_t &a_port );
            int32_t LastError();
            void    Shutdown();

            // Read functions
            bool     IsReadable() noexcept final;
            bool     Read( uint8_t &a_value, bool a_block = false ) noexcept final;
            uint32_t Read( uint8_t *a_value, uint32_t a_length, bool a_block = false ) noexcept final;
            bool     Read( ::std::shared_ptr< Buffer > &a_buffer, bool a_block = false ) noexcept;
            bool     Peek( uint8_t &a_value ) noexcept final;
            uint32_t Peek( uint8_t *a_value, uint32_t a_length ) noexcept final;
            bool     Peek( ::std::shared_ptr< Buffer > &a_buffer ) noexcept;

            // Write functions
            bool     IsWritable() noexcept final;
            bool     Write( const uint8_t &a_value ) noexcept final;
            uint32_t Write( const uint8_t *a_value, uint32_t a_length ) noexcept final;
            bool     Write( ::std::shared_ptr< Buffer > &a_buffer ) noexcept;
    };
}

#endif // _SOCKET_HPP_
