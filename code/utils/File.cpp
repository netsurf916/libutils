/**
    File.cpp : File class implementation
    Copyright 2015-2026 Daniel Wilson
    SPDX-License-Identifier: MIT
*/

#include <utils/File.hpp>
#include <cstring>
#include <stdio.h>
#include <sys/stat.h>

namespace utils
{
    File::File( FILE *a_file, uint32_t a_mode /*= FileMode::DefaultRead*/ )
    : m_mode( a_mode )
    , m_modTime( 0 )
    , m_file( a_file )
    , m_ready( false )
    {
        ::utils::Lock lock( this );
        struct stat fileStat{};
        if( m_file != nullptr )
        {
            int fd = fileno( m_file );
            if( ( fd >= 0 ) && ( fstat( fd, &fileStat ) == 0 ) )
            {
                m_modTime = static_cast< uint32_t >( fileStat.st_mtime );
                m_ready = S_ISREG( fileStat.st_mode );
            }
        }
    }

    File::File( const char *a_fileName, uint32_t a_mode /*= FileMode::DefaultRead*/ )
    : m_fileName( a_fileName )
    , m_mode( a_mode )
    , m_modTime( 0 )
    , m_file( nullptr )
    , m_ready( false )
    {
        ::utils::Lock lock( this );
        struct stat fileStat{};
        if( !m_fileName.empty() && ( stat( m_fileName.c_str(), &fileStat ) == 0 ) )
        {
            m_modTime = static_cast< uint32_t >( fileStat.st_mtime );
            m_ready = S_ISREG( fileStat.st_mode );
        }
    }

    File::~File()
    {
        ::utils::Lock lock( this );
        Close();
    }

    ::std::string &File::Name()
    {
        ::utils::Lock( this );
        return m_fileName;
    }

    File::operator const char *()
    {
        ::utils::Lock( this );
        return m_fileName.c_str();
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
        struct stat fileStat{};
        bool ok = false;
        if( m_file != nullptr )
        {
            int fd = fileno( m_file );
            if( fd >= 0 )
            {
                ok = ( fstat( fd, &fileStat ) == 0 );
            }
        }
        if( !ok && !m_fileName.empty() )
        {
            ok = ( stat( m_fileName.c_str(), &fileStat ) == 0 );
        }
        if( ok )
        {
            return static_cast< uint64_t >( fileStat.st_size );
        }
        return 0;
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
            if( position >= 0 )
            {
                return position;
            }
        }
        return -1;
    }

    bool File::Exists()
    {
        ::utils::Lock lock( this );
        struct stat fileStat{};
        bool ok = false;
        if( m_file != nullptr )
        {
            int fd = fileno( m_file );
            if( fd >= 0 )
            {
                ok = ( fstat( fd, &fileStat ) == 0 );
            }
        }
        if( !ok && !m_fileName.empty() )
        {
            ok = ( stat( m_fileName.c_str(), &fileStat ) == 0 );
        }
        if( ok )
        {
            return S_ISREG( fileStat.st_mode ) || S_ISDIR( fileStat.st_mode );
        }
        return false;
    }

    bool File::IsFile()
    {
        ::utils::Lock lock( this );
        struct stat fileStat{};
        bool ok = false;
        if( m_file != nullptr )
        {
            int fd = fileno( m_file );
            if( fd >= 0 )
            {
                ok = ( fstat( fd, &fileStat ) == 0 );
            }
        }
        if( !ok && !m_fileName.empty() )
        {
            ok = ( stat( m_fileName.c_str(), &fileStat ) == 0 );
        }
        if( ok )
        {
            return S_ISREG( fileStat.st_mode );
        }
        return false;
    }

    bool File::IsDirectory()
    {
        ::utils::Lock lock( this );
        struct stat fileStat{};
        bool ok = false;
        if( m_file != nullptr )
        {
            int fd = fileno( m_file );
            if( fd >= 0 )
            {
                ok = ( fstat( fd, &fileStat ) == 0 );
            }
        }
        if( !ok && !m_fileName.empty() )
        {
            ok = ( stat( m_fileName.c_str(), &fileStat ) == 0 );
        }
        if( ok )
        {
            return S_ISDIR( fileStat.st_mode );
        }
        return false;
    }

    bool File::IsModified()
    {
        uint32_t modTime = ModificationTime();
        bool modified = modTime != m_modTime;
        if( modified )
        {
            m_modTime = modTime;
        }
        return modified;
    }

    uint32_t File::ModificationTime()
    {
        ::utils::Lock lock( this );
        struct stat fileStat{};
        bool ok = false;
        if( m_file != nullptr )
        {
            int fd = fileno( m_file );
            if( fd >= 0 )
            {
                ok = ( fstat( fd, &fileStat ) == 0 );
            }
        }
        if( !ok && !m_fileName.empty() )
        {
            ok = ( stat( m_fileName.c_str(), &fileStat ) == 0 );
        }
        if( ok )
        {
            return static_cast< uint32_t >( fileStat.st_mtime );
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
        if( m_fileName.length() > 0 )
        {
            return ( Close() && ( 0 == remove( m_fileName.c_str() ) ) );
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
        ::std::string mode;

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

        if( !m_ready || ( m_fileName.length() <= 0 ) || ( mode.length() <= 0 ) )
        {
            return false;
        }

        m_file = fopen( m_fileName.c_str(), mode.c_str() );
        return ( nullptr != m_file );
    }
}
