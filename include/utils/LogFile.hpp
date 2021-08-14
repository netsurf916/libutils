/**
    LogFile.hpp : Log file definition
    Copyright 2014-2019 Daniel Wilson
*/

#pragma once

#ifndef _LOGFILE_HPP_
#define _LOGFILE_HPP_

#include <utils/Types.hpp>
#include <utils/Lock.hpp>
#include <utils/String.hpp>

namespace utils
{
    class LogFile : public Lockable
    {
        private:
            String m_file;

        public:
            LogFile( String &a_file );
            LogFile( const char *a_file );

            bool Log( String &a_message, bool a_timestamp = true, bool a_newline = true );
            bool Log( const char   *a_value, bool a_timestamp = true, bool a_newline = true );
            bool Log( const int32_t  &a_value, bool a_timestamp = true, bool a_newline = true );
            bool Log( const uint32_t &a_value, bool a_timestamp = true, bool a_newline = true );

            ~LogFile();
    };
}

#endif // _LOGFILE_HPP_
