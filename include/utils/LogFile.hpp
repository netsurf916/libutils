/**
    LogFile.hpp : Log file definition
    Description: Log file writer utility.
    Copyright 2014-2026 Daniel Wilson
    SPDX-License-Identifier: MIT
*/

#pragma once

#ifndef _LOGFILE_HPP_
#define _LOGFILE_HPP_

#include <utils/Types.hpp>
#include <utils/Lock.hpp>
#include <string>

namespace utils
{
    class LogFile : public Lockable
    {
        private:
            ::std::string m_file;

        public:
            LogFile( const char *a_file );

            bool Log( ::std::string  &a_message, bool a_timestamp = true, bool a_newline = true );
            bool Log( const char     *a_value,   bool a_timestamp = true, bool a_newline = true );
            bool Log( const int32_t  &a_value,   bool a_timestamp = true, bool a_newline = true );
            bool Log( const uint32_t &a_value,   bool a_timestamp = true, bool a_newline = true );

            ~LogFile();
    };
}

#endif // _LOGFILE_HPP_
