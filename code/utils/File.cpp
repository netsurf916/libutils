/**
    File.cpp : File class implementation
    Copyright 2015-2019 Daniel Wilson
*/

#include <utils/File.hpp>
#include <sys/stat.h>
#include <cstring>
#include <stdio.h>

namespace utils
{
    File::File( FILE *a_file, uint32_t a_mode /*= FileMode::DefaultRead*/ )
    : m_mode( a_mode )
    , m_file( a_file )
    , m_ready( true )
    {
    }

    File::File( String &a_fileName, uint32_t a_mode /*= FileMode::DefaultRead*/ )
    : m_fileName( a_fileName )
    , m_mode( a_mode )
    , m_file( nullptr )
    , m_ready( true )
    {
    }

    File::File( const char *a_fileName, uint32_t a_mode /*= FileMode::DefaultRead*/ )
    : m_fileName( a_fileName )
    , m_mode( a_mode )
    , m_file( nullptr )
    , m_ready( true )
    {
    }

    File::~File()
    {
        ::utils::Lock lock( this );
        Close();
    }

    String &File::Name()
    {
        ::utils::Lock( this );
        return m_fileName;
    }

    File::operator const char *()
    {
        ::utils::Lock( this );
        return ( const char * )m_fileName;
    }

    uint8_t File::Type() noexcept
    {
        return static_cast< uint8_t >( SerializableType::File );
    }

    bool File::Serialize( Writable &a_out ) noexcept
    {
        ::utils::Lock lock( this );
        ::utils::Lock valueLock( &a_out );
        bool ok = m_ready && SerializeType( a_out );
        ok = ok && m_fileName.Serialize( a_out );
        uint64_t length = ToNetwork( static_cast< uint64_t >( Size() ) );
        ok = ok && ( sizeof( length ) == a_out.Write( ( const uint8_t * )&length,   sizeof( length ) ) );
        if( ok && ( length > 0 ) )
        {
            uint32_t mode = m_mode;
            SetMode( FileMode::DefaultRead );
            ok = Seek( 0 );
            uint8_t *data = new uint8_t[ 1024 ];
            if( nullptr != data )
            {
                length = FromNetwork( length );
                while( ok && ( length > 0 ) )
                {
                    uint32_t read = Read( data, ( length < 1024 )? length : 1024 );
                    if( 0 == read )
                    {
                        ok = false;
                    }
                    else if( read != a_out.Write( data, read ) )
                    {
                        ok = false;
                    }
                    length -= read;
                }
                memset( data, 0, 1024 );
                delete [] data;
                data = nullptr;
            }
            SetMode( mode );
        }
        return ok;
    }

    bool File::Deserialize( Readable &a_in ) noexcept
    {
        ::utils::Lock lock( this );
        ::utils::Lock valueLock( &a_in );
        bool ok = m_ready && DeserializeType( a_in );
        ok = ok && m_fileName.Deserialize( a_in );
        uint64_t length = 0;
        ok = ok && ( sizeof( length ) == a_in.Read( ( uint8_t * )&length, sizeof( length ) ) );
        if( ok )
        {
            length = FromNetwork( length );
        }
        if( ok && ( length > 0 ) )
        {
            uint32_t mode = m_mode;
            SetMode( FileMode::DefaultWrite );
            ok = Seek( 0 );
            uint8_t *data = new uint8_t[ 1024 ];
            if( nullptr != data )
            {
                while( ok && ( length > 0 ) )
                {
                    uint32_t read = a_in.Read( data, ( length < 1024 )? length : 1024 );
                    if( 0 == read )
                    {
                        ok = false;
                    }
                    else if( read != Write( data, read ) )
                    {
                        ok = false;
                    }
                    length -= read;
                }
                memset( data, 0, 1024 );
                delete [] data;
                data = nullptr;
            }
            SetMode( mode );
        }
        return ok;
    }

    void File::SetMode( uint32_t a_mode /*= FileMode::DefaultRead*/ )
    {
        ::utils::Lock lock( this );
        if( ( a_mode != m_mode ) && ( nullptr != m_file ) )
        {
            Close();
        }
        else if( a_mode == m_mode )
        {
            Seek( 0 );
        }
        m_mode = a_mode;
    }

    uint64_t File::Size()
    {
        ::utils::Lock lock( this );
        struct stat fileStat;
        uint64_t length = 0;
        if( nullptr == m_file )
        {
            Open();
        }
        if( nullptr != m_file )
        {
            if( ( 0 == fstat( fileno( m_file ), &fileStat ) ) && ( fileStat.st_size > 0 ) )
            {
                length = static_cast< uint64_t >( fileStat.st_size );
            }
        }
        return length;
    }

    int64_t File::Position()
    {
        ::utils::Lock lock( this );
        if( nullptr == m_file )
        {
            Open();
        }
        if( nullptr != m_file )
        {
            int64_t position = ftell( m_file );
            if( position > 0 )
            {
                return position;
            }
        }
        return 0;
    }

    bool File::Exists()
    {
        ::utils::Lock lock( this );
        struct stat fileStat;
        if( nullptr == m_file )
        {
            Open();
        }
        if( nullptr != m_file )
        {
            return ( 0 == fstat( fileno( m_file ), &fileStat ) );
        }
        return false;
    }

    uint32_t File::ModificationTime()
    {
        ::utils::Lock lock( this );
        struct stat fileStat;
        if( nullptr == m_file )
        {
            Open();
        }
        if( m_ready && ( nullptr != m_file ) )
        {
            if( 0 == fstat( fileno( m_file ), &fileStat ) )
            {
                return static_cast< uint32_t >( fileStat.st_mtime );
            }
        }
        return 0;
    }

    bool File::Seek( int64_t a_position )
    {
        ::utils::Lock lock( this );
        if( nullptr == m_file )
        {
            Open();
        }
        if( m_ready && ( nullptr != m_file ) && ( a_position >= 0 ) )
        {
            return ( 0 == fseek( m_file, a_position, SEEK_SET ) );
        }
        return false;
    }

    bool File::IsReadable() noexcept
    {
        ::utils::Lock lock( this );
        // Read, Append, or Update
        return ( ( FileMode::Read   == ( m_mode & FileMode::Read   ) ) ||
                 ( FileMode::Append == ( m_mode & FileMode::Append ) ) ||
                 ( FileMode::Update == ( m_mode & FileMode::Update ) ) );
    }

    bool File::Read( uint8_t &a_value, bool a_block /*= false*/ ) noexcept
    {
        UNUSED( a_block );
        ::utils::Lock lock( this );
        if( nullptr == m_file )
        {
            Open();
        }
        bool ok = m_ready;
        ok = ok && ( nullptr != m_file );
        ok = ok && ( 1 == fread( &a_value, sizeof( uint8_t ), 1, m_file ) );
        return ok;
    }

    uint32_t File::Read( uint8_t *a_value, uint32_t a_length, bool a_block /*= false*/ ) noexcept
    {
        UNUSED( a_block );
        if( ( nullptr == a_value ) || ( 0 == a_length ) )
        {
            return 0;
        }
        ::utils::Lock lock( this );
        if( nullptr == m_file )
        {
            Open();
        }
        if( m_ready && ( nullptr != m_file ) )
        {
            return fread( a_value, sizeof( uint8_t ), a_length, m_file );
        }
        return 0;
    }

    bool File::Read( ::std::shared_ptr< Buffer > &a_buffer, bool a_block /*= false*/ ) noexcept
    {
        UNUSED( a_block );
        ::utils::Lock lock( this );
        if( !a_buffer || ( 0 == a_buffer->Space() ) )
        {
            return false;
        }
        bool ok = false;
        if( nullptr == m_file )
        {
            Open();
        }
        if( m_ready && ( nullptr != m_file ) )
        {
            uint32_t dataLen = a_buffer->Space();
            uint8_t *data = new uint8_t[ dataLen ];
            if( nullptr != data )
            {
                uint32_t read = fread( data, sizeof( uint8_t ), dataLen, m_file );
                ok = ( read > 0 ) && ( a_buffer->Write( data, read ) > 0 );
                memset( data, 0, dataLen );
                delete [] data;
                data = nullptr;
            }
        }
        return ok;
    }

    bool File::Peek( uint8_t &a_value ) noexcept
    {
        ::utils::Lock lock( this );
        bool  ok = false;
        if( nullptr == m_file )
        {
            Open();
        }
        if( m_ready && ( nullptr != m_file ) )
        {
            int64_t position = Position();
            if( 1 == fread( &a_value, sizeof( uint8_t ), 1, m_file ) )
            {
                ok = true;
                Seek( position );
            }
        }
        return ok;
    }

    uint32_t File::Peek( uint8_t *a_value, uint32_t a_length ) noexcept
    {
        if( ( nullptr == a_value ) || ( 0 == a_length ) )
        {
            return 0;
        }
        ::utils::Lock lock( this );
        uint32_t read = 0;
        if( nullptr == m_file )
        {
            Open();
        }
        if( m_ready && ( nullptr != m_file ) )
        {
            int64_t position = Position();
            read = fread( a_value, sizeof( uint8_t ), a_length, m_file );
            if( read > 0 )
            {
                Seek( position );
            }
        }
        return read;
    }

    bool File::Peek( ::std::shared_ptr< Buffer > &a_buffer ) noexcept
    {
        ::utils::Lock lock( this );
        if( !a_buffer || ( 0 == a_buffer->Space() ) )
        {
            return false;
        }
        bool  ok = false;
        if( nullptr == m_file )
        {
            Open();
        }
        if( m_ready && ( nullptr != m_file ) )
        {
            uint32_t dataLen = a_buffer->Space();
            uint8_t *data = new uint8_t[ dataLen ];
            if( data )
            {
                int64_t position = Position();
                uint32_t read = fread( data, sizeof( uint8_t ), dataLen, m_file );
                ok = ( read > 0 ) && ( a_buffer->Write( data, read ) > 0 );
                Seek( position );
                memset( data, 0, dataLen );
                delete [] data;
                data = nullptr;
            }
        }
        return ok;
    }

    bool File::IsWritable() noexcept
    {
        ::utils::Lock lock( this );
        // Write, Append, or Update
        return ( ( FileMode::Write  == ( m_mode & FileMode::Write  ) ) ||
                 ( FileMode::Append == ( m_mode & FileMode::Append ) ) ||
                 ( FileMode::Update == ( m_mode & FileMode::Update ) ) );
    }

    bool File::Write( const uint8_t &a_value ) noexcept
    {
        ::utils::Lock lock( this );
        if( nullptr == m_file )
        {
            Open();
        }
        bool ok = m_ready;
        ok = ok && ( nullptr != m_file );
        ok = ok && ( 1 == fwrite( &a_value, sizeof( uint8_t ), 1, m_file ) );
        return ok;
    }

    uint32_t File::Write( const uint8_t *a_value, uint32_t a_length ) noexcept
    {
        if( ( nullptr == a_value ) || ( 0 == a_length ) )
        {
            return 0;
        }
        ::utils::Lock lock( this );
        if( nullptr == m_file )
        {
            Open();
        }
        if( m_ready && ( nullptr != m_file ) )
        {
            return fwrite( a_value, sizeof( uint8_t ), a_length, m_file );
        }
        return 0;
    }

    bool File::Write( ::std::shared_ptr< Buffer > &a_buffer ) noexcept
    {
        ::utils::Lock lock( this );
        if( !a_buffer || ( 0 == a_buffer->Length() ) )
        {
            return false;
        }
        a_buffer->Defragment();
        if( nullptr == m_file )
        {
            Open();
        }
        bool ok = m_ready;
        ok = ok && ( nullptr != m_file );
        if( ok )
        {
            uint32_t written = fwrite( a_buffer->Value(), sizeof( uint8_t ), a_buffer->Length(), m_file );
            ok = ( written > 0 );
            if( ok )
            {
                a_buffer->TrimLeft( written );
            }
        }
        return ok;
    }

    bool File::Delete()
    {
        ::utils::Lock lock( this );
        if( m_fileName )
        {
            return ( Close() && ( 0 == remove( m_fileName ) ) );
        }
        return false;
    }

    bool File::Close()
    {
        ::utils::Lock lock( this );
        if( ( nullptr != m_file ) && ( stdin != m_file ) && ( stdout != m_file ) )
        {
            if( IsWritable() )
            {
                fflush( m_file );
            }
            if( 0 == fclose( m_file ) )
            {
                m_file = nullptr;
                return true;
            }
        }
        return false;
    }

    bool File::Open()
    {
        ::utils::Lock lock( this );
        // The only valid modes are:
        // r, rb          * Read
        // w, wb          * Write/Truncate
        // a, ab          * Append
        // r+, rb+, r+b   * Update
        // w+, wb+, w+b   * Write/Truncate
        // a+, ab+, a+b   * Append
        //
        // To enforce these modes, an order of precedence
        // will be used: 'r', 'w', 'a' followed by 'b' then '+'
        String mode;

        // Set either read, write, or append
        if( FileMode::Read == ( m_mode & FileMode::Read ) )
        {
            mode += 'r';
        }
        else if( FileMode::Write == ( m_mode & FileMode::Write ) )
        {
            mode += 'w';
        }
        else if( FileMode::Append == ( m_mode & FileMode::Append ) )
        {
            mode += 'a';
        }

        // Set binary reading
        if( FileMode::Binary == ( m_mode & FileMode::Binary ) )
        {
            mode += 'b';
        }

        // Set update
        if( FileMode::Update == ( m_mode & FileMode::Update ) )
        {
            mode += '+';
        }

        if( !m_ready || !m_fileName || !mode )
        {
            return false;
        }

        m_file = fopen( m_fileName, mode );
        return ( nullptr != m_file );
    }
}
