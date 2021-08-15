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

    ::std::string &IniFileHeading::Name()
    {
        ::utils::Lock lock( this );
        return m_name;
    }

    ::std::shared_ptr< IniFileHeading > &IniFileHeading::Next()
    {
        ::utils::Lock lock( this );
        return m_next;
    }

    ::std::shared_ptr< KeyValuePair< ::std::string, ::std::string > > &IniFileHeading::Entries()
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
        ::std::shared_ptr< KeyValuePair< ::std::string, ::std::string > > entry = ::std::make_shared< KeyValuePair< ::std::string, ::std::string > >();
        if( entry )
        {
            entry->Key() = a_key;
            Tokens::MakeUpper( entry->Key() );
            entry->Value() = a_value;
            if( !m_entries )
            {
                m_entries = entry;
                return true;
            }
            else
            {
                ::std::shared_ptr< KeyValuePair< ::std::string, ::std::string > > start = m_entries;
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

    bool IniFileHeading::GetValue( const char *a_key, ::std::string &a_value )
    {
        ::utils::Lock lock( this );
        if( ( nullptr == a_key ) || !m_entries )
        {
            return false;
        }
        ::std::string key( a_key );
        if( key.length() == 0 )
        {
            return false;
        }
        Tokens::MakeUpper( key );
        ::std::shared_ptr< KeyValuePair< ::std::string, ::std::string > > entries = m_entries;
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
        ::std::string heading;
        ::std::shared_ptr< Buffer > buffer = ::std::make_shared< Buffer >( 4096 );
        m_loading = true;
        m_modTime = m_file.ModificationTime();
        m_heading = nullptr;
        m_file.Seek( 0 );
        while( buffer && ( TokenTypes::Line == Tokens::GetLine( m_file, *buffer ) ) )
        {
            ::std::string line;
            Tokens::GetToken( *buffer, line, ';' );
            buffer->Clear();
            if( 0 == line.length() )
            {
                // Skip the comment line
                continue;
            }
            if( ( line.find( "[" ) != ::std::string::npos )
             && ( line.find( "]" ) != ::std::string::npos ) )
            {
                // Get the heading name
                heading.clear();
                bool inHeading = false;
                for( uint16_t i = 0; i < line.length(); ++i )
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
                Tokens::MakeUpper( heading );
            }
            else if( ( heading.length() > 0 )
                  && ( line.find( "=" ) != ::std::string::npos ) )
            {
                // Get an entry
                ::std::string key;
                ::std::string value;
                bool inKey = true;
                for( uint16_t i = 0; i < line.length(); ++i )
                {
                    if( '=' == line[ i ] )
                    {
                        inKey = false;
                        continue;
                    }
                    if( inKey && ( line[ i ] != '\t' ) && ( line[ i ] != ' ' ) )
                    {
                        key += line[ i ];
                    }
                    else if( ( line[ i ] != '\t' ) && ( line[ i ] != ' ' ) )
                    {
                        value += line[ i ];
                    }
                }
                if( ( heading.length() > 0 ) && ( key.length() > 0 ) && ( value.length() > 0 ) )
                {
                    Tokens::MakeUpper( key );
                    WriteValue( heading.c_str(), key.c_str(), value.c_str() );
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
                m_file.Write( ( const uint8_t * )start->Name().c_str(), start->Name().length() );
                m_file.Write( ( const uint8_t * )"]\n", 2 );
                ::std::shared_ptr< KeyValuePair< ::std::string, ::std::string > > entries = start->Entries();
                while( entries )
                {
                    m_file.Write( ( const uint8_t * )entries->Key().c_str(), entries->Key().length() );
                    m_file.Write( ( const uint8_t * )" = ", 3 );
                    m_file.Write( ( const uint8_t * )entries->Value().c_str(), entries->Value().length() );
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

    bool IniFile::ReadValue( const char *a_heading, const char *a_name, ::std::string &a_value )
    {
        ::utils::Lock lock( this );
        if( m_file.ModificationTime() != m_modTime )
        {
            LoadFile();
        }
        if( m_heading )
        {
            ::std::string heading( a_heading );
            if( heading.length() == 0 )
            {
                return false;
            }
            Tokens::MakeUpper( heading ); printf("Heading: %s\n", heading.c_str());
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
                Tokens::MakeUpper( m_heading->Name() );
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
                ::std::string value;
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
