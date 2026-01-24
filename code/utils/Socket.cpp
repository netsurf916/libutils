/**
    Socket.cpp : Socket implementation
    Copyright 2014-2026 Daniel Wilson
    SPDX-License-Identifier: MIT
*/

#include <utils/Socket.hpp>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/poll.h>
#include <fcntl.h>
#include <errno.h>
#include <netdb.h>
#include <stdio.h>
#include <cstring>
#include <unistd.h>
#include <signal.h>

// Timeout for calls to poll()
#define RWTIMEOUTMS 250

namespace utils
{
    Socket::Socket( int32_t a_sockfd, uint32_t a_flags /*= 0*/ )
    : m_sockfd  ( a_sockfd )
    , m_valid   ( a_sockfd >= 0 )
    , m_error   ( 0 )
    , m_flags   ( a_flags )
    {
    }

    Socket::Socket( const char *a_address, uint32_t a_port, uint32_t a_flags /*= 0*/ )
    : m_valid ( false )
    , m_error ( 0 )
    , m_flags ( a_flags )
    {
        char port[ 16 ]; // 32-bit input could be 10 digits + 1 for NULL terminator, so 11 minimum should be allocated
        snprintf( port, sizeof( port ), "%d", a_port );
        m_sockfd = Initialize( a_address, port );
        if( m_sockfd < 0 )
        {
            m_valid = false;
        }
    }

    Socket::~Socket()
    {
        ::utils::Lock lock( this );
        Shutdown();
    }

    int32_t Socket::Initialize( const char *a_address, const char *a_service )
    {
        int32_t sockfd = -1;
        if( a_address )
        {
            struct addrinfo  hints;
            struct addrinfo *result = nullptr;
            memset( &hints, 0, sizeof( hints ) );
            hints.ai_family    = AF_UNSPEC;
            hints.ai_socktype  = ( m_flags[ SocketFlag::TCP ] )? SOCK_STREAM: SOCK_DGRAM;
            hints.ai_flags     = ( m_flags[ SocketFlag::Server ] )? AI_PASSIVE: AI_ADDRCONFIG;
            if( 0 == getaddrinfo( a_address, a_service, &hints, &result ) )
            {
                for ( struct addrinfo *item = result; nullptr != item; item = item->ai_next )
                {
                    sockfd = socket( item->ai_family, item->ai_socktype, item->ai_protocol );
                    if ( sockfd < 0 )
                    {
                        continue;
                    }
                    m_valid = true;
                    if( m_flags[ SocketFlag::Server ] )
                    {
                        int yes = 1;
                        if( 0 == setsockopt( sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof( int ) ) )
                        {
                            if( 0 == bind( sockfd, item->ai_addr, item->ai_addrlen ) )
                            {
                                if( !m_flags[ SocketFlag::TCP ] )
                                {
                                    break;
                                }
                                if( 0 == listen( sockfd, 100 ) )
                                {
                                    break;
                                }
                                m_error = errno;
                                Shutdown();
                            }
                            else
                            {
                                m_error = errno;
                                Shutdown();
                            }
                        }
                        else
                        {
                            m_error = errno;
                            Shutdown();
                        }
                    }
                    else
                    {
                        if( 0 == connect( sockfd, item->ai_addr, item->ai_addrlen ) )
                        {
                            break;
                        }
                        else
                        {
                            m_error = errno;
                            Shutdown();
                        }
                    }
                }
            }
            if( result )
            {
                freeaddrinfo( result );
            }
        }
        else
        {
            m_error = errno;
            Shutdown();
        }
        if( m_valid )
        {
            // Ignore SIGPIPE
            struct sigaction act;
            act.sa_handler = SIG_IGN;
            sigemptyset( &act.sa_mask );
            act.sa_flags = 0;
            sigaction( SIGPIPE, &act, nullptr );
        }
        return sockfd;
    }

    bool Socket::Valid()
    {
        ::utils::Lock lock( this );
        // The socket is valid if m_valid is true and one of the following is true:
        // * running a listening socket
        // * the fd can be written to
        // * the fd can be read from
        // The order is important because IsReadable() waits for data and should not be
        // called unless necessary.
        m_valid = m_valid && ( m_flags.IsSet( SocketFlag::Server ) || IsWritable() || IsReadable() );
        return m_valid;
    }

    ::std::shared_ptr< Socket > Socket::Accept( ::std::string &a_address, uint32_t &a_port )
    {
        ::utils::Lock lock( this );
        struct sockaddr_storage address;
        socklen_t length    = sizeof( address );
        int32_t   client_fd = accept( m_sockfd, ( struct sockaddr * )&address, &length );
        if( client_fd >= 0 )
        {
            uint32_t port = 0;
            char     addr[ INET6_ADDRSTRLEN + 1 ] = { 0 };
            if ( address.ss_family == AF_INET )
            {
                struct sockaddr_in *s = ( struct sockaddr_in * )&address;
                port = ntohs( s->sin_port );
                inet_ntop( AF_INET, &( s->sin_addr ), addr, sizeof( addr ) );
            }
            else
            {
                struct sockaddr_in6 *s = ( struct sockaddr_in6 * )&address;
                port = ntohs( s->sin6_port );
                inet_ntop( AF_INET6, &( s->sin6_addr ), addr, sizeof( addr ) );
            }
            a_port    = port;
            a_address = addr;
            utils::BitMask flags( m_flags );
            flags.SetBit( SocketFlags::Server, false );
            ::std::shared_ptr< Socket > client = ::std::make_shared< Socket >( client_fd, ( unsigned int )flags );
            if( client )
            {
                client->m_flags.SetBit( SocketFlags::Server, false );
            }
            return client;
        }
        m_error = errno;
        if( ( m_error != EAGAIN ) && ( m_error != EWOULDBLOCK ) )
        {
            Shutdown();
        }
        return nullptr;
    }

    int32_t Socket::LastError()
    {
        ::utils::Lock lock( this );
        return m_error;
    }

    void Socket::Shutdown()
    {
        ::utils::Lock lock( this );
        if( m_sockfd >= 0 )
        {
            fsync( m_sockfd );
            close( m_sockfd );
        }
        m_sockfd = -1;
        m_valid  = false;
        m_error  = errno;
    }

    bool Socket::IsReadable() noexcept
    {
        // Cannot use Valid() since Valid() may call IsReadable()
        ::utils::Lock lock( this );
        if( m_valid )
        {
            struct pollfd pfd;
            pfd.fd = m_sockfd;
            pfd.events = ( POLLIN | POLLPRI | POLLRDBAND );
            if( poll( &pfd, 1, RWTIMEOUTMS ) < 0 )
            {
                // Shutdown() sets m_valid to false
                Shutdown();
            }
        }
        return m_valid;
    }

    bool Socket::Read( uint8_t &a_value, bool a_block /*= false*/ ) noexcept
    {
        ::utils::Lock lock( this );
        bool ok = Valid();
        if( ok )
        {
            int32_t result = recv( m_sockfd, &a_value, sizeof( uint8_t ), MSG_PEEK | MSG_DONTWAIT );
            if( ( sizeof( a_value ) == result ) || a_block )
            {
                result = recv( m_sockfd, &a_value, sizeof( uint8_t ), 0 );
            }
            if( result <= 0 )
            {
                m_error = errno;
                if( ( m_error != EAGAIN ) && ( m_error != EWOULDBLOCK ) )
                {
                    Shutdown();
                }
                ok = false;
            }
        }
        return ok;
    }

    uint32_t Socket::Read( uint8_t *a_value, uint32_t a_length, bool a_block /*= false*/ ) noexcept
    {
        ::utils::Lock lock( this );
        if( ( nullptr == a_value ) || ( 0 == a_length ) )
        {
            return Valid();
        }
        int32_t read = 0;
        if( Valid() && ( a_length > 0 ) )
        {
            read = recv( m_sockfd, a_value, a_length, MSG_PEEK | MSG_DONTWAIT );
            if( ( read > 0 ) || a_block )
            {
                read = recv( m_sockfd, a_value, a_length, 0 );
            }
            if( read <= 0 )
            {
                m_error = errno;
                if( ( m_error != EAGAIN ) && ( m_error != EWOULDBLOCK ) )
                {
                    Shutdown();
                }
                read = 0;
            }
        }
        return static_cast< uint32_t >( read );
    }

    bool Socket::Read( ::std::shared_ptr< Buffer > &a_buffer, bool a_block /*= false*/ ) noexcept
    {
        ::utils::Lock lock( this );
        if( !a_buffer )
        {
            return Valid();
        }
        ::utils::Lock valueLock( a_buffer.get() );
        uint32_t size = a_buffer->Space();
        if( size > 0 )
        {
            uint8_t *data = new uint8_t[ size ];
            if( data )
            {
                uint32_t read = Read( data, size, a_block );
                if( read > 0 )
                {
                    a_buffer->Write( data, read );
                }
                delete [] data;
                data = nullptr;
            }
        }
        return Valid();
    }

    bool Socket::Peek( uint8_t &a_value ) noexcept
    {
        ::utils::Lock lock( this );
        bool ok = Valid();
        if( ok )
        {
            int32_t result = recv( m_sockfd, &a_value, sizeof( uint8_t ), MSG_PEEK | MSG_DONTWAIT );
            if( result <= 0 )
            {
                m_error = errno;
                if( ( m_error != EAGAIN ) && ( m_error != EWOULDBLOCK ) )
                {
                    Shutdown();
                }
                ok = false;
            }
        }
        return ok;
    }

    uint32_t Socket::Peek( uint8_t *a_value, uint32_t a_length ) noexcept
    {
        ::utils::Lock lock( this );
        if( ( nullptr == a_value ) || ( 0 == a_length ) )
        {
            return Valid();
        }
        int32_t read = 0;
        if( Valid() && ( a_length > 0 ) )
        {
            read = recv( m_sockfd, a_value, a_length, MSG_PEEK | MSG_DONTWAIT );
            if( read <= 0 )
            {
                m_error = errno;
                if( ( m_error != EAGAIN ) && ( m_error != EWOULDBLOCK ) )
                {
                    Shutdown();
                }
                read = 0;
            }
        }
        return static_cast< uint32_t >( read );
    }

    bool Socket::Peek( ::std::shared_ptr< Buffer > &a_buffer ) noexcept
    {
        ::utils::Lock lock( this );
        if( !a_buffer )
        {
            return Valid();
        }
        ::utils::Lock valueLock( a_buffer.get() );
        uint32_t size = a_buffer->Space();
        if( size > 0 )
        {
            uint8_t *data = new uint8_t[ size ];
            if( data )
            {
                uint32_t read = Peek( data, size );
                if( read > 0 )
                {
                    a_buffer->Write( data, read );
                }
                delete [] data;
                data = nullptr;
            }
        }
        return Valid();
    }

    bool Socket::IsWritable() noexcept
    {
        // Cannot use Valid() since Valid() may call IsWritable()
        ::utils::Lock lock( this );
        if( m_valid )
        {
            struct pollfd pfd;
            pfd.fd = m_sockfd;
            pfd.events = ( POLLOUT | POLLWRNORM | POLLWRBAND );
            if( poll( &pfd, 1, RWTIMEOUTMS ) < 0 )
            {
                // Shutdown() sets m_valid to false
                Shutdown();
            }
        }
        return m_valid;
    }

    bool Socket::Write( const uint8_t &a_value ) noexcept
    {
        ::utils::Lock lock( this );
        bool ok = Valid();
        if( ok )
        {
            int32_t result = send( m_sockfd, &a_value, sizeof( uint8_t ), 0 );
            if( result < 0 )
            {
                ok = false;
                Shutdown();
            }
        }
        return ok;
    }

    uint32_t Socket::Write( const uint8_t *a_value, uint32_t a_length ) noexcept
    {
        ::utils::Lock lock( this );
        if( nullptr == a_value )
        {
            return Valid();
        }
        uint32_t total = 0;
        while( Valid() && ( a_length > total ) )
        {
            int32_t result = send( m_sockfd, a_value + total, ( a_length - total ), 0 );
            if( result > 0 )
            {
                total += result;
            }
            if( result <= 0 )
            {
                Shutdown();
            }
        }
        return total;
    }

    bool Socket::Write( ::std::shared_ptr< Buffer > &a_buffer ) noexcept
    {
        ::utils::Lock lock( this );
        if( !a_buffer || !Valid() )
        {
            return false;
        }
        ::utils::Lock valueLock( a_buffer.get() );
        uint32_t sent = 0;
        if( a_buffer->Length() > 0 )
        {
            a_buffer->Defragment();
            int32_t result = Write( a_buffer->Value(), a_buffer->Length() );
            if( result > 0 )
            {
                a_buffer->TrimLeft( result );
                sent += result;
            }
        }
        return ( sent > 0 );
    }
}
