/**
    Socket.cpp : Socket implementation
    Copyright 2014-2021 Daniel Wilson
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
    #ifdef USE_SSL
    , m_sslctx  ( nullptr )
    , m_ssl     ( nullptr )
    #endif // USE_SSL
    {
    }

    #ifdef USE_SSL
    Socket::Socket( const char *a_address, uint32_t a_port, uint32_t a_flags /*= 0*/, const char *a_keyfile /*= ""*/, const char *a_certfile /*= ""*/ )
    #else // !USE_SSL
    Socket::Socket( const char *a_address, uint32_t a_port, uint32_t a_flags /*= 0*/ )
    #endif
    : m_valid ( false )
    , m_error ( 0 )
    , m_flags ( a_flags )
    #ifdef USE_SSL
    , m_sslctx  ( nullptr )
    , m_ssl     ( nullptr )
    #endif // USE_SSL
    {
        char port[ 32 ];
        snprintf( port, sizeof( port ), "%d", a_port );
        #ifdef USE_SSL
        if( m_flags.IsSet( SocketFlags::Secure ) )
        {
            SSL_load_error_strings();	
            OpenSSL_add_ssl_algorithms();
        }
        m_sockfd = Initialize( a_address, port, a_keyfile, a_certfile );
        #else // !USE_SSL
        m_sockfd = Initialize( a_address, port );
        #endif
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

    #ifdef USE_SSL
    int32_t Socket::Initialize( const char *a_address, const char *a_service, const char *a_keyfile, const char *a_certfile )
    #else // !USE_SSL
    int32_t Socket::Initialize( const char *a_address, const char *a_service )
    #endif
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
            // Configure TLS
            #ifdef USE_SSL
            if( m_flags.IsSet( SocketFlags::Secure ) )
            {
                m_sslctx = SSL_CTX_new( TLS_method() );
                m_valid = ( m_sslctx != nullptr );
                if( m_valid )
                {
                    SSL_CTX_set_min_proto_version( m_sslctx, TLS1_2_VERSION );
                }
                m_valid = m_valid && ( SSL_CTX_use_certificate_file( m_sslctx, a_certfile, SSL_FILETYPE_PEM ) > 0 );
                m_valid = m_valid && ( SSL_CTX_use_PrivateKey_file( m_sslctx, a_keyfile, SSL_FILETYPE_PEM ) > 0 );
            }
            #endif // USE_SSL
        }
        return sockfd;
    }

    #ifdef USE_SSL
    void Socket::Start_SSL()
    {
        if( m_flags.IsSet( SocketFlags::Secure ) )
        {
            m_valid = ( SSL_accept( m_ssl ) > 0 );
        }
    }
    #endif // USE_SSL

    bool Socket::Valid()
    {
        ::utils::Lock lock( this );
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
                #ifdef USE_SSL
                if( client->m_flags.IsSet( SocketFlags::Secure ) )
                {
                    client->m_ssl = SSL_new( m_sslctx );
                    SSL_set_fd( client->m_ssl, client_fd );
                }
                #endif // USE_SSL
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

    #ifdef USE_SSL
    bool Socket::ReadLine_SSL( ::std::shared_ptr< Buffer > &a_buffer, const uint32_t a_timeout /* = 1000 */ )
    {
        ::utils::Lock lock( this );
        if( !a_buffer )
        {
            return false;
        }
        ::utils::Lock valueLock( a_buffer.get() );
        uint8_t data = 0;
        uint32_t timeout = a_timeout;
        bool done = !Valid();
        while( !done && ( timeout > 0 ) )
        {
            int32_t result = SSL_peek( m_ssl, &data, sizeof( data ) );
            if( ( result > 0 ) && ( a_buffer->Space() > 0 ) )
            {
                timeout = a_timeout;
                result = SSL_read( m_ssl, &data, sizeof( data ) );
                if( result > 0 )
                {
                    switch( data )
                    {
                        case '\n': // Handle '\n' and "\n\r"
                            result = SSL_peek( m_ssl, &data, sizeof( data ) );
                            if( ( result > 0 ) && ( data == '\r' ) )
                            {
                                result = SSL_read( m_ssl, &data, sizeof( data ) );
                            }
                            done = true;
                            break;
                        case '\r': // Handle '\r' and "\r\n"
                            result = SSL_peek( m_ssl, &data, sizeof( data ) );
                            if( ( result > 0 ) && ( data == '\n' ) )
                            {
                                result = SSL_read( m_ssl, &data, sizeof( data ) );
                            }
                            done = true;
                            break;
                        default:
                            a_buffer->Write( data );
                            break;
                    }
                }
            }
            else if( result > 0 )
            {
                usleep( 1000 );
                --timeout;
            }
            if( result <= 0 )
            {
                done    = true;
                m_error = SSL_get_error( m_ssl, result );
                if( ( m_error != SSL_ERROR_WANT_READ ) && ( m_error != SSL_ERROR_NONE ) )
                {
                    Shutdown();
                }
            }
        }
        if( timeout == 0 )
        {
            m_error = ETIMEDOUT;
        }
        return done;
    }
    #endif // USE_SSL

    bool Socket::ReadLine( ::std::shared_ptr< Buffer > &a_buffer, const uint32_t a_timeout /* = 1000 */ )
    {
        #ifdef USE_SSL
        if( m_flags.IsSet( SocketFlags::Secure ) )
        {
            return ReadLine_SSL( a_buffer, a_timeout );
        }
        #endif // USE_SSL
        ::utils::Lock lock( this );
        if( !a_buffer )
        {
            return false;
        }
        ::utils::Lock valueLock( a_buffer.get() );
        uint8_t data = 0;
        uint32_t timeout = a_timeout;
        bool done = !Valid();
        while( !done && ( timeout > 0 ) )
        {
            int32_t result = recv( m_sockfd, &data, sizeof( data ), MSG_PEEK | MSG_DONTWAIT );
            if( ( result > 0 ) && ( a_buffer->Space() > 0 ) )
            {
                timeout = a_timeout;
                result = recv( m_sockfd, &data, sizeof( data ), 0 );
                if( result > 0 )
                {
                    switch( data )
                    {
                        case '\n': // Handle '\n' and "\n\r"
                            result = recv( m_sockfd, &data, sizeof( data ), MSG_PEEK | MSG_DONTWAIT );
                            if( ( result > 0 ) && ( data == '\r' ) )
                            {
                                result = recv( m_sockfd, &data, sizeof( data ), 0 );
                            }
                            done = true;
                            break;
                        case '\r': // Handle '\r' and "\r\n"
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
            else if( result >= 0 )
            {
                usleep( 1000 );
                --timeout;
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
        if( timeout == 0 )
        {
            m_error = ETIMEDOUT;
        }
        return done;
    }

    void Socket::Shutdown()
    {
        ::utils::Lock lock( this );
        if( m_sockfd >= 0 )
        {
            #ifdef USE_SSL
            if( m_ssl != nullptr )
            {
                SSL_shutdown( m_ssl );
                SSL_free( m_ssl );
            }
            #endif // USE_SSL
            fsync( m_sockfd );
            close( m_sockfd );
        }
        #ifdef USE_SSL
        if( m_sslctx != nullptr )
        {
            SSL_CTX_free( m_sslctx );
        }
        #endif // USE_SSL
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

    #ifdef USE_SSL
    bool Socket::Read_SSL( uint8_t &a_value, bool a_block /*= false*/ ) noexcept
    {
        ::utils::Lock lock( this );
        bool ok = Valid() && m_flags.IsSet( SocketFlags::Secure );
        if( ok )
        {
            int32_t result = SSL_peek( m_ssl, &a_value, sizeof( uint8_t ) );
            if( ( result > 0 ) || a_block )
            {
                result = SSL_read( m_ssl, &a_value, sizeof( uint8_t ) );
            }
            if( result <= 0 )
            {
                m_error = SSL_get_error( m_ssl, result );
                if( ( m_error != SSL_ERROR_WANT_READ ) && ( m_error != SSL_ERROR_NONE ) )
                {
                    Shutdown();
                }
                ok = false;
            }
        }
        return ok;
    }
    #endif // USE_SSL

    bool Socket::Read( uint8_t &a_value, bool a_block /*= false*/ ) noexcept
    {
        #ifdef USE_SSL
        if( m_flags.IsSet( SocketFlags::Secure ) )
        {
            return Read_SSL( a_value, a_block );
        }
        #endif // USE_SSL
        ::utils::Lock lock( this );
        bool ok = Valid();
        if( ok )
        {
            int32_t result = recv( m_sockfd, &a_value, sizeof( uint8_t ), MSG_PEEK | MSG_DONTWAIT );
            if( ( sizeof( a_value ) == result ) || a_block )
            {
                result = recv( m_sockfd, &a_value, sizeof( uint8_t ), 0 );
            }
            if( result < 0 )
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

    #ifdef USE_SSL
    uint32_t Socket::Read_SSL( uint8_t *a_value, uint32_t a_length, bool a_block /*= false*/ ) noexcept
    {
        ::utils::Lock lock( this );
        if( ( nullptr == a_value ) || ( 0 == a_length ) || !m_flags.IsSet( SocketFlags::Secure ) )
        {
            return Valid();
        }
        size_t read = 0;
        if( Valid() && ( a_length > 0 ) )
        {
            int32_t result = SSL_peek_ex( m_ssl, a_value, a_length, &read );
            if( ( result > 0 ) || a_block )
            {
                result = SSL_read_ex( m_ssl, a_value, a_length, &read );
            }
            if( result <= 0 )
            {
                m_error = SSL_get_error( m_ssl, result );
                if( ( m_error != SSL_ERROR_WANT_READ ) && ( m_error != SSL_ERROR_NONE ) )
                {
                    Shutdown();
                }
            }
        }
        return static_cast< uint32_t >( read );
    }
    #endif // USE_SSL

    uint32_t Socket::Read( uint8_t *a_value, uint32_t a_length, bool a_block /*= false*/ ) noexcept
    {
        #ifdef USE_SSL
        if( m_flags.IsSet( SocketFlags::Secure ) )
        {
            return Read_SSL( a_value, a_length, a_block );
        }
        #endif // USE_SSL
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

    #ifdef USE_SSL
    bool Socket::Peek_SSL( uint8_t &a_value ) noexcept
    {
        ::utils::Lock lock( this );
        bool ok = Valid() && m_flags.IsSet( SocketFlags::Secure );
        if( ok )
        {
            int32_t result = SSL_peek( m_ssl, &a_value, sizeof( uint8_t ) );
            if( result <= 0 )
            {
                m_error = SSL_get_error( m_ssl, result );
                if( ( m_error != SSL_ERROR_WANT_READ ) && ( m_error != SSL_ERROR_NONE ) )
                {
                    Shutdown();
                }
                ok = false;
            }
        }
        return ok;
    }
    #endif // USE_SSL

    bool Socket::Peek( uint8_t &a_value ) noexcept
    {
        #ifdef USE_SSL
        if( m_flags.IsSet( SocketFlags::Secure ) )
        {
            return Peek_SSL( a_value );
        }
        #endif // USE_SSL
        ::utils::Lock lock( this );
        bool ok = Valid();
        if( ok )
        {
            int32_t result = recv( m_sockfd, &a_value, sizeof( uint8_t ), MSG_PEEK | MSG_DONTWAIT );
            if( result < 0 )
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

    #ifdef USE_SSL
    uint32_t Socket::Peek_SSL( uint8_t *a_value, uint32_t a_length ) noexcept
    {
        ::utils::Lock lock( this );
        if( ( nullptr == a_value ) || ( 0 == a_length ) || !m_flags.IsSet( SocketFlags::Secure ) )
        {
            return Valid();
        }
        size_t read = 0;
        if( Valid() && ( a_length > 0 ) )
        {
            int32_t result = SSL_peek_ex( m_ssl, a_value, a_length, &read );
            if( result <= 0 )
            {
                m_error = SSL_get_error( m_ssl, result );
                if( ( m_error != SSL_ERROR_WANT_READ ) && ( m_error != SSL_ERROR_NONE ) )
                {
                    Shutdown();
                }
            }
        }
        return static_cast< uint32_t >( read );
    }
    #endif // USE_SSL

    uint32_t Socket::Peek( uint8_t *a_value, uint32_t a_length ) noexcept
    {
        #ifdef USE_SSL
        if( m_flags.IsSet( SocketFlags::Secure ) )
        {
            return Peek_SSL( a_value, a_length );
        }
        #endif // USE_SSL
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

    #ifdef USE_SSL
    bool Socket::Write_SSL( const uint8_t &a_value ) noexcept
    {
        ::utils::Lock lock( this );
        bool ok = Valid();
        if( ok )
        {
            int32_t result = SSL_write( m_ssl, &a_value, sizeof( uint8_t ) );
            if( result <= 0 )
            {
                m_error = SSL_get_error( m_ssl, result );
                if( ( m_error != SSL_ERROR_WANT_WRITE ) && ( m_error != SSL_ERROR_NONE ) )
                {
                    Shutdown();
                }
                ok = false;
            }
        }
        return ok;
    }
    #endif // USE_SSL

    bool Socket::Write( const uint8_t &a_value ) noexcept
    {
        #ifdef USE_SSL
        if( m_flags.IsSet( SocketFlags::Secure ) )
        {
            return Write_SSL( a_value );
        }
        #endif // USE_SSL
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

    #ifdef USE_SSL
    uint32_t Socket::Write_SSL( const uint8_t *a_value, uint32_t a_length ) noexcept
    {
        ::utils::Lock lock( this );
        if( nullptr == a_value )
        {
            return Valid();
        }
        uint32_t total = 0;
        while( Valid() && ( a_length > total ) )
        {
            int32_t result = 0;
            size_t  sent = 0;
            result = SSL_write_ex( m_ssl, a_value + total, ( a_length - total ), &sent );
            if( result > 0 )
            {
                total += sent;
            }
            if( result <= 0 )
            {
                m_error = SSL_get_error( m_ssl, result );
                if( ( m_error != SSL_ERROR_WANT_WRITE ) && ( m_error != SSL_ERROR_NONE ) )
                {
                    Shutdown();
                }
                else
                {
                    usleep( 500 );
                }
            }
        }
        return total;
    }
    #endif // USE_SSL

    uint32_t Socket::Write( const uint8_t *a_value, uint32_t a_length ) noexcept
    {
        #ifdef USE_SSL
        if( m_flags.IsSet( SocketFlags::Secure ) )
        {
            return Write_SSL( a_value, a_length );
        }
        #endif // USE_SSL
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
            if( 0 == result )
            {
                usleep( 500 );
            }
            if( result < 0 )
            {
                Shutdown();
            }
        }
        return total;
    }

    bool Socket::Write( ::std::shared_ptr< Buffer > &a_buffer ) noexcept
    {
        ::utils::Lock lock( this );
        if( !a_buffer )
        {
            return Valid();
        }
        ::utils::Lock valueLock( a_buffer.get() );
        uint32_t sent = 0;
        while( Valid() && ( a_buffer->Length() > 0 ) )
        {
            a_buffer->Defragment();
            int32_t result = Write( a_buffer->Value(), a_buffer->Length() );
            if( result > 0 )
            {
                a_buffer->TrimLeft( result );
                sent += result;
            }
        }
        return Valid();
    }
}
