/**
    IniFile.cpp : INI file implementation
    Copyright 2014-2019 Daniel Wilson
*/

#include <utils/IniFile.hpp>
#include <utils/Buffer.hpp>
#include <utils/KeyValuePair.hpp>
#include <utils/Tokens.hpp>
#include <stdio.h>
#include <time.h>

namespace utils
{
    IniFileHeading::IniFileHeading()
    {
    }

    IniFileHeading::~IniFileHeading()
    {
        ::utils::Lock lock( this );
    }

    String &IniFileHeading::Name()
    {
        ::utils::Lock lock( this );
        return m_name;
    }

    ::std::shared_ptr< IniFileHeading > &IniFileHeading::Next()
    {
        ::utils::Lock lock( this );
        return m_next;
    }

    ::std::shared_ptr< KeyValuePair< String, String > > &IniFileHeading::Entries()
    {
        ::utils::Lock lock( this );
        return m_entries;
    }

    bool IniFileHeading::SetValue( const char *a_key, const char *a_value )
    {
        ::utils::Lock lock( this );
        if( ( nullptr == a_key ) || ( nullptr == a_value ) )
        {
            return false;
        }
        ::std::shared_ptr< KeyValuePair< String, String > > entry = ::std::make_shared< KeyValuePair< String, String > >();
        if( entry )
        {
            entry->Key() = a_key;
            entry->Key().ToUpper();
            entry->Value() = a_value;
            if( !m_entries )
            {
                m_entries = entry;
                return true;
            }
            else
            {
                ::std::shared_ptr< KeyValuePair< String, String > > start = m_entries;
                while( start && ( start->Key() != entry->Key() ) )
                {
                    if( !start->Next() )
                    {
                        start->Next() = entry;
                        return true;
                    }
                    start = start->Next();
                }
                if( start && ( start != entry ) )
                {
                    start->Value() = entry->Value();
                    return true;
                }
            }
        }
        return false;
    }

    bool IniFileHeading::GetValue( const char *a_key, String &a_value )
    {
        ::utils::Lock lock( this );
        ::utils::Lock valueLock( &a_value );
        if( ( nullptr == a_key ) || !m_entries )
        {
            return false;
        }
        String key( a_key );
        if( !key )
        {
            return false;
        }
        key.ToUpper();
        ::std::shared_ptr< KeyValuePair< String, String > > entries = m_entries;
        while( entries )
        {
            if( entries->Key() == key )
            {
                a_value = entries->Value();
                return true;
            }
            entries = entries->Next();
        }
        return false;
    }

    IniFile::IniFile( const char *a_file )
    : m_file( a_file, FileMode::DefaultRead )
    , m_modTime( 0 )
    , m_loading( false )
    {
        LoadFile();
    }

    IniFile::~IniFile()
    {
        ::utils::Lock lock( this );
        ::utils::Lock fileLock( &m_file );
    }

    void IniFile::LoadFile()
    {
        ::utils::Lock lock( this );
        ::utils::Lock fileLock( &m_file );
        String heading;
        ::std::shared_ptr< Buffer > buffer = ::std::make_shared< Buffer >( 4096 );
        m_loading = true;
        m_modTime = m_file.ModificationTime();
        m_heading = nullptr;
        m_file.Seek( 0 );
        while( buffer && ( TokenTypes::Line == Tokens::GetLine( m_file, *buffer ) ) )
        {
            String line;
            Tokens::GetToken( *buffer, line, ';' );
            buffer->Clear();
            if( 0 == line.Length() )
            {
                // Skip the comment line
                continue;
            }
            if( line.Contains( "[" ) && line.Contains( "]" ) )
            {
                // Get the heading name
                heading.Clear();
                bool inHeading = false;
                for( uint16_t i = 0; i < line.Length(); ++i )
                {
                    if( !inHeading && ( '[' == line[ i ] ) )
                    {
                        inHeading = true;
                        continue;
                    }
                    if( inHeading && ( ']' == line[ i ] ) )
                    {
                        inHeading = false;
                        break;
                    }
                    heading += line[ i ];
                }
            }
            else if( ( heading.Length() ) > 0 && line.Contains( "=" ) )
            {
                // Get an entry
                String key;
                String value;
                bool inKey = true;
                for( uint16_t i = 0; i < line.Length(); ++i )
                {
                    if( '=' == line[ i ] )
                    {
                        inKey = false;
                        continue;
                    }
                    if( inKey )
                    {
                        key += line[ i ];
                    }
                    else
                    {
                        value += line[ i ];
                    }
                }
                key.Trim();
                value.Trim();
                if( ( heading.Length() > 0 ) && ( key.Length() > 0 ) && ( value.Length() > 0 ) )
                {
                    heading.ToUpper();
                    key.ToUpper();
                    WriteValue( ( const char * )heading, ( const char * )key, ( const char * )value );
                }
            }
        }
        m_file.Close();
        m_loading = false;
    }

    void IniFile::SaveFile()
    {
        ::utils::Lock lock( this );
        ::utils::Lock fileLock( &m_file );
        if( m_loading )
        {
            return;
        }
        if( m_heading )
        {
            m_file.Seek( 0 );
            m_file.SetMode( FileMode::DefaultWrite );
            ::std::shared_ptr< IniFileHeading > start = m_heading;
            while( start )
            {
                m_file.Write( ( const uint8_t * )"[", 1 );
                m_file.Write( ( const uint8_t * )start->Name(), start->Name().Length() );
                m_file.Write( ( const uint8_t * )"]\n", 2 );
                ::std::shared_ptr< KeyValuePair< String, String > > entries = start->Entries();
                while( entries )
                {
                    m_file.Write( ( const uint8_t * )entries->Key(), entries->Key().Length() );
                    m_file.Write( ( const uint8_t * )" = ", 3 );
                    m_file.Write( ( const uint8_t * )entries->Value(), entries->Value().Length() );
                    m_file.Write( ( const uint8_t * )"\n", 1 );
                    entries = entries->Next();
                }
                m_file.Write( ( const uint8_t * )"\n", 1 );
                start = start->Next();
            }
            m_file.Close();
            m_file.SetMode( FileMode::DefaultRead );
            m_modTime = m_file.ModificationTime();
        }
    }

    bool IniFile::ReadValue( const char *a_heading, const char *a_name, String &a_value )
    {
        ::utils::Lock lock( this );
        ::utils::Lock valueLock( &a_value );
        if( m_file.ModificationTime() != m_modTime )
        {
            LoadFile();
        }
        if( m_heading )
        {
            String heading( a_heading );
            if( !heading )
            {
                return false;
            }
            heading.ToUpper();
            ::std::shared_ptr< IniFileHeading > start = m_heading;
            while( start && ( start->Name() != heading ) )
            {
                start = start->Next();
            }
            if( start )
            {
                return start->GetValue( a_name, a_value );
            }
        }
        return false;
    }

    bool IniFile::WriteValue( const char *a_heading, const char *a_name, const char *a_value )
    {
        ::utils::Lock lock( this );
        if( !m_heading )
        {
            m_heading = ::std::make_shared< IniFileHeading >();
            if( m_heading )
            {
                m_heading->Name() = a_heading;
                m_heading->Name().ToUpper();
                if( !m_heading->SetValue( a_name, a_value ) )
                {
                    m_heading = nullptr;
                }
                else
                {
                    SaveFile();
                }
                return m_heading.get();
            }
        }
        else
        {
            bool updated = false;
            ::std::shared_ptr< IniFileHeading > start = m_heading;
            while( start && ( start->Name() != a_heading ) )
            {
                if( !start->Next() )
                {
                    start->Next() = ::std::make_shared< IniFileHeading >();
                    if( start->Next() )
                    {
                        start->Next()->Name() = a_heading;
                        updated = true;
                    }
                }
                start = start->Next();
            }
            if( start )
            {
                String value;
                if( !updated )
                {
                    if( start->GetValue( a_name, value ) )
                    {
                        if( value == a_value )
                        {
                            return true;
                        }
                    }
                    updated = true;
                }
                if( updated && start->SetValue( a_name, a_value ) )
                {
                    SaveFile();
                    return true;
                }
            }
        }
        return false;
    }
}
