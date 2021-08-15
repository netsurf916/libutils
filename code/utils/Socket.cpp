/**
    Socket.cpp : Socket implementation
    Copyright 2014-2019 Daniel Wilson
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
    , m_port    ( 0 )
    , m_flags   ( a_flags )
    {
        memset( m_addr, 0, sizeof( m_addr ) );
    }

    Socket::Socket( const char *a_address, uint32_t a_port, uint32_t a_flags /*= 0*/ )
    : m_valid ( false )
    , m_error ( 0 )
    , m_port  ( a_port )
    , m_flags ( a_flags )
    {
        char port[ 32 ];
        snprintf( port, sizeof( port ), "%d", a_port );
        m_sockfd = Initialize( a_address, port );
        if( m_sockfd < 0 )
        {
            m_valid = false;
        }
        memset( m_addr, 0, sizeof( m_addr ) );
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
                                    fcntl( sockfd, F_SETFL, O_NONBLOCK );
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
        return m_valid;
    }

    bool Socket::Accept( int32_t &a_client )
    {
        ::utils::Lock lock( this );
        struct sockaddr_storage addr;
        socklen_t len = sizeof( addr );
        a_client = accept( m_sockfd, ( struct sockaddr * )&addr, &len );
        if( a_client >= 0 )
        {
            return true;
        }
        m_error = errno;
        if( ( m_error != EAGAIN ) && ( m_error != EWOULDBLOCK ) )
        {
            Shutdown();
        }
        return false;
    }

    int32_t Socket::LastError()
    {
        ::utils::Lock lock( this );
        return m_error;
    }

    bool Socket::ReadLine( ::std::shared_ptr< Buffer > &a_buffer, bool a_block /*= false*/ )
    {
        if( !a_buffer )
        {
            return false;
        }
        ::utils::Lock lock( this );
        uint8_t data = 0;
        bool done = !Valid();
        while( !done )
        {
            int32_t result = recv( m_sockfd, &data, sizeof( data ), MSG_PEEK | MSG_DONTWAIT );
            if( result > 0 || ( ( 0 == result ) && a_block ) )
            {
                if( ( 0 == a_buffer->Space() ) )
                {
                    if( a_block )
                    {
                        usleep( 500 );
                        continue;
                    }
                    break;
                }
                if( 0 == a_buffer->Space() )
                {
                    if( a_block )
                    {
                        continue;
                    }
                    break;
                }
                result = recv( m_sockfd, &data, sizeof( data ), 0 );
                if( result > 0 )
                {
                    switch( data )
                    {
                        case '\n': // Handle \n and \n\r
                            result = recv( m_sockfd, &data, sizeof( data ), MSG_PEEK | MSG_DONTWAIT );
                            if( ( result > 0 ) && ( data == '\r' ) )
                            {
                                result = recv( m_sockfd, &data, sizeof( data ), 0 );
                            }
                            done = true;
                            break;
                        case '\r': // Handle \r and \r\n
                            result = recv( m_sockfd, &data, sizeof( data ), MSG_PEEK | MSG_DONTWAIT );
                            if( ( result > 0 ) && ( data == '\n' ) )
                            {
                                result = recv( m_sockfd, &data, sizeof( data ), 0 );
                            }
                            done = true;
                            break;
                        default:
                            a_buffer->Write( data );
                            break;
                    }
                }
            }
            else if( result == 0 )
            {
                usleep( 500 );
            }
            if( result < 0 )
            {
                done    = true;
                m_error = errno;
                if( ( m_error != EAGAIN ) && ( m_error != EWOULDBLOCK ) )
                {
                    Shutdown();
                }
            }
        }
        return done;
    }

    bool Socket::GetRemoteAddress( ::std::string &a_address, uint32_t &a_port )
    {
        ::utils::Lock lock( this );
        if( m_flags[ SocketFlag::Server ] )
        {
            return false;
        }
        if( Valid() )
        {
            struct sockaddr_storage addr;
            uint32_t length = sizeof( addr );
            if( 0 == getpeername( m_sockfd, ( struct sockaddr * )&addr, &length ) )
            {
                if ( addr.ss_family == AF_INET )
                {
                    struct sockaddr_in *s = ( struct sockaddr_in * )&addr;
                    m_port = ntohs( ( *s ).sin_port );
                    inet_ntop( AF_INET, &( ( *s ).sin_addr ), m_addr, sizeof( m_addr ) );
                }
                else
                {
                    struct sockaddr_in6 *s = ( struct sockaddr_in6 * )&addr;
                    m_port = ntohs( ( *s ).sin6_port );
                    inet_ntop( AF_INET6, &( ( *s ).sin6_addr ), m_addr, sizeof( m_addr ) );
                }
                a_port    = m_port;
                a_address = m_addr;
            }
            else if( !m_flags[ SocketFlag::Server ] )
            {
                m_error = errno;
                Shutdown();
            }
        }
        return Valid();
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
        ::utils::Lock lock( this );
        if( Valid() )
        {
            struct pollfd pfd;
            pfd.fd = m_sockfd;
            pfd.events = ( POLLIN | POLLPRI | POLLRDBAND );
            if( poll( &pfd, 1, RWTIMEOUTMS ) > 0 )
            {
                return true;
            }
        }
        return false;
    }

    bool Socket::Read( uint8_t &a_value, bool a_block /*= false*/ ) noexcept
    {
        ::utils::Lock lock( this );
        bool ok = Valid();
        if( ok )
        {
            int32_t result = recv( m_sockfd, &a_value, sizeof( a_value ), MSG_PEEK | MSG_DONTWAIT );
            if( ( sizeof( a_value ) == result ) || a_block )
            {
                result = recv( m_sockfd, &a_value, sizeof( a_value ), 0 );
            }
            if( result <= 0 )
            {
                m_error = errno;
                if( ( m_error != EAGAIN ) && ( m_error != EWOULDBLOCK ) )
                {
                    ok = false;
                    Shutdown();
                }
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
        bool ok = Valid();
        if( ok && a_buffer && ( a_buffer->Space() > 0 ) )
        {
            uint8_t *data = new uint8_t[ a_buffer->Space() ];
            if( data )
            {
                int32_t result = recv( m_sockfd, data, a_buffer->Space(), MSG_PEEK | MSG_DONTWAIT );
                if( ( result > 0 ) || a_block )
                {
                    result = recv( m_sockfd, data, a_buffer->Space(), 0 );
                    if( result > 0 )
                    {
                        ok = a_buffer->Write( data, result );
                    }
                }
                if( result <= 0 )
                {
                    m_error = errno;
                    if( ( m_error != EAGAIN ) && ( m_error != EWOULDBLOCK ) )
                    {
                        ok = false;
                        Shutdown();
                    }
                }
                delete [] data;
            }
        }
        return ok;
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
                    ok = false;
                    Shutdown();
                }
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
        bool ok = Valid();
        if( ok && a_buffer && ( a_buffer->Space() > 0 ) )
        {
            uint8_t *data = new uint8_t[ a_buffer->Space() ];
            if( data )
            {
                int32_t result = recv( m_sockfd, data, a_buffer->Space(), MSG_PEEK | MSG_DONTWAIT );
                if( result > 0 )
                {
                    ok = a_buffer->Write( data, result );
                }
                if( result <= 0 )
                {
                    m_error = errno;
                    if( ( m_error != EAGAIN ) && ( m_error != EWOULDBLOCK ) )
                    {
                        ok = false;
                        Shutdown();
                    }
                }
                delete [] data;
            }
        }
        return ok;
    }

    bool Socket::IsWritable() noexcept
    {
        ::utils::Lock lock( this );
        if( Valid() )
        {
            struct pollfd pfd;
            pfd.fd = m_sockfd;
            pfd.events = ( POLLOUT | POLLWRNORM | POLLWRBAND );
            if( poll( &pfd, 1, RWTIMEOUTMS ) > 0 )
            {
                return true;
            }
        }
        return false;
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
        uint32_t sent = 0;
        if( Valid() && ( a_length > 0 ) )
        {
            int32_t result = 0;
            do
            {
                result = send( m_sockfd, a_value + sent, ( a_length - sent ), 0 );
                if( result > 0 )
                {
                    sent += result;
                }
                if( 0 == result )
                {
                    usleep( 500 );
                }
            } while( ( result >= 0 ) && ( sent < a_length ) && Valid() );
            if( result < 0 )
            {
                Shutdown();
            }
        }
        return sent;
    }

    bool Socket::Write( ::std::shared_ptr< Buffer > &a_buffer ) noexcept
    {
        ::utils::Lock lock( this );
        uint32_t sent = 0;
        if( Valid() && a_buffer && ( a_buffer->Length() > 0 ) )
        {
            int32_t result = 0;
            a_buffer->Defragment();
            do
            {
                result = send( m_sockfd, *a_buffer, a_buffer->Length(), 0 );
                if( result > 0 )
                {
                    a_buffer->TrimLeft( result );
                    sent += result;
                }
                if( 0 == result )
                {
                    usleep( 500 );
                }
            } while( ( result >= 0 ) && ( a_buffer->Length() > 0 ) && Valid() );
            if( result < 0 )
            {
                Shutdown();
            }
        }
        return ( sent > 0 );
    }
}
