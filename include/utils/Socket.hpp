/**
    Socket.hpp : Socket class definition
    Copyright 2014-2021 Daniel Wilson
*/

// Enable TLS support
#define USE_SSL

#include <utils/Types.hpp>
#include <utils/Buffer.hpp>
#include <utils/BitMask.hpp>
#include <utils/Readable.hpp>
#include <utils/Writable.hpp>
#include <memory>
#include <netinet/in.h>
#ifdef USE_SSL
#include <openssl/ssl.h>
#include <openssl/err.h>
#endif // USE_SSL

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
            #ifdef USE_SSL
            Secure    = 2,
            #endif // USE_SSL
            TcpClient = ( BIT(TCP)                             ),
            TcpServer = ( BIT(TCP) | BIT(Server)               ),
            UdpClient = ( 0                                    ),
            UdpServer = (            BIT(Server)               ),
            #ifdef USE_SSL
            TlsClient = ( BIT(TCP) |               BIT(Secure) ), // TODO: Not implemented yet
            TlsServer = ( BIT(TCP) | BIT(Server) | BIT(Secure) ),
            #endif // USE_SSL
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
            #ifdef USE_SSL
            SSL_CTX *m_sslctx; // Used by the server
            SSL     *m_ssl;    // Only initialized for clients
            #endif // USE_SSL

        public:
            Socket( int32_t a_sockfd, uint32_t a_flags = 0 );
            #ifdef USE_SSL
            Socket( const char *a_address, uint32_t a_port, uint32_t a_flags = 0, const char *a_keyfile = "", const char *a_certfile = "" );
            #else // !USE_SSL
            Socket( const char *a_address, uint32_t a_port, uint32_t a_flags = 0 );
            #endif // USE_SSL
            ~Socket();

        private:
            #ifdef USE_SSL
            int32_t Initialize( const char *a_address, const char *a_service, const char *a_keyfile, const char *a_certfile );
            #else // !USE_SSL
            int32_t Initialize( const char *a_address, const char *a_service );
            #endif // USE_SSL

            #ifdef USE_SSL
            // SSL read functions
            bool     Read_SSL( uint8_t &a_value, bool a_block = false ) noexcept;
            uint32_t Read_SSL( uint8_t *a_value, uint32_t a_length, bool a_block = false ) noexcept;
            bool     Peek_SSL( uint8_t &a_value ) noexcept;
            uint32_t Peek_SSL( uint8_t *a_value, uint32_t a_length ) noexcept;

            // SSL write functions
            bool     Write_SSL( const uint8_t &a_value ) noexcept;
            uint32_t Write_SSL( const uint8_t *a_value, uint32_t a_length ) noexcept;
            #endif // USE_SSL

        public:
            bool    Valid();
            ::std::shared_ptr< Socket > Accept( ::std::string &a_address, uint32_t &a_port );
            int32_t LastError();
            bool    ReadLine( ::std::shared_ptr< Buffer > &a_buffer, uint32_t a_timeout = 1000 );
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
