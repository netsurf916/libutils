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
    /**
     * @brief Simple log file writer.
     * @details Appends log entries to a file, optionally adding timestamps and
     *          newlines. The log file path must be valid and writable.
     */
    class LogFile : public Lockable
    {
        private:
            ::std::string m_file;

        public:
            /**
             * @brief Construct a logger for a specific file path.
             * @param a_file Path to the log file; must be non-null.
             */
            LogFile( const char *a_file );

            /**
             * @brief Log a string message.
             * @param a_message Message to log.
             * @param a_timestamp Whether to prefix a timestamp.
             * @param a_newline Whether to append a newline.
             * @return True on success; false on write errors.
             */
            bool Log( ::std::string  &a_message, bool a_timestamp = true, bool a_newline = true );
            /**
             * @brief Log a C-string message.
             * @param a_value Message to log; must be non-null.
             * @param a_timestamp Whether to prefix a timestamp.
             * @param a_newline Whether to append a newline.
             * @return True on success; false on write errors.
             */
            bool Log( const char     *a_value,   bool a_timestamp = true, bool a_newline = true );
            /**
             * @brief Log a signed integer value.
             * @param a_value Integer value to log.
             * @param a_timestamp Whether to prefix a timestamp.
             * @param a_newline Whether to append a newline.
             * @return True on success; false on write errors.
             */
            bool Log( const int32_t  &a_value,   bool a_timestamp = true, bool a_newline = true );
            /**
             * @brief Log an unsigned integer value.
             * @param a_value Integer value to log.
             * @param a_timestamp Whether to prefix a timestamp.
             * @param a_newline Whether to append a newline.
             * @return True on success; false on write errors.
             */
            bool Log( const uint32_t &a_value,   bool a_timestamp = true, bool a_newline = true );

            /**
             * @brief Destroy the logger.
             */
            ~LogFile();
    };
}

#endif // _LOGFILE_HPP_
