/**
    LogFile.cpp : Log file implementation
    Copyright 2014-2019 Daniel Wilson
*/

#include <utils/LogFile.hpp>
#include <time.h>
#include <stdio.h>

namespace utils
{
    LogFile::LogFile( String &a_file )
    : m_file( a_file )
    {}

    LogFile::LogFile( const char *a_file )
    : m_file( a_file )
    {}

    bool LogFile::Log( String &a_message, bool a_timestamp /*= true*/, bool a_newline /*= true*/ )
    {
        ::utils::Lock lock( this );
        time_t rawtime;
        struct tm *timeinfo = nullptr;
        if( m_file.Length() == 0 )
        {
            return false;
        }
        FILE *output = fopen( m_file.Value(), "a+" );
        if( !output )
        {
            return false;
        }
        if( a_timestamp )
        {
            char buffer[ 128 ];
            for( uint32_t i = 0; i < sizeof( buffer ); i++ )
            {
                buffer[ i ] = 0;
            }
            time( &rawtime );
            timeinfo = localtime( &rawtime );
            if( ( a_message.Length() > 0 ) || !a_newline )
            {
                strftime( buffer, sizeof( buffer ), "%F %T (%Z) - ", timeinfo );
            }
            else
            {
                strftime( buffer, sizeof( buffer ), "%F %T (%Z)", timeinfo );
            }
            fputs( buffer, output );
        }
        if( a_message.Length() > 0 )
        {
            fputs( a_message.Value(), output );
        }
        if( a_newline )
        {
            fputs( "\n", output );
        }
        fflush( output );
        fclose( output );
        return ( ferror( output ) == 0 );
    }

    bool LogFile::Log( const char *a_value, bool a_timestamp /*= true*/, bool a_newline /*= true*/ )
    {
        ::utils::Lock lock( this );
        String data( a_value );
        return Log( data, a_timestamp, a_newline );
    }

    bool LogFile::Log( const int32_t &a_value, bool a_timestamp /*= true*/, bool a_newline /*= true*/ )
    {
        ::utils::Lock lock( this );
        char buffer[ 32 ];
        for( uint32_t i = 0; i < sizeof( buffer ); i++ )
        {
            buffer[ i ] = 0;
        }
        snprintf( buffer, sizeof( buffer ), "%d", a_value );
        return Log( buffer, a_timestamp, a_newline );
    }

    bool LogFile::Log( const uint32_t &a_value, bool a_timestamp /*= true*/, bool a_newline /*= true*/ )
    {
        ::utils::Lock lock( this );
        char buffer[ 32 ];
        for( uint32_t i = 0; i < sizeof( buffer ); i++ )
        {
            buffer[ i ] = 0;
        }
        snprintf( buffer, sizeof( buffer ), "%u", a_value );
        return Log( buffer, a_timestamp, a_newline );
    }

    LogFile::~LogFile()
    {
        ::utils::Lock lock( this );
    }
}
