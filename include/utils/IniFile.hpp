/**
    IniFile.hpp : INI file definition
    Description: INI file parser and writer.
    Copyright 2014-2026 Daniel Wilson
    SPDX-License-Identifier: MIT
*/

#pragma once

#ifndef _INIFILE_HPP_
#define _INIFILE_HPP_

#include <utils/Utils.hpp>
#include <utils/File.hpp>
#include <memory>
#include <string>

namespace utils
{
    /**
     * @brief Represents a single INI section and its key/value entries.
     * @details Stores the section name and a linked list of entries.
     * @note Not safe for concurrent access without external synchronization.
     */
    class IniFileHeading : public Lockable
    {
        private:
            ::std::string                                                     m_name;
            ::std::shared_ptr< KeyValuePair< ::std::string, ::std::string > > m_entries;
            ::std::shared_ptr< IniFileHeading >                               m_next;

        public:
            /**
             * @brief Construct an empty INI section.
             */
            IniFileHeading();

            /**
             * @brief Destroy the section and its entries.
             */
            ~IniFileHeading();

        public:
            /**
             * @brief Access the section name.
             * @return Mutable reference to the name string.
             */
            ::std::string                                                     &Name();

            /**
             * @brief Access the head of the entries list.
             * @return Shared pointer to the first entry.
             */
            ::std::shared_ptr< KeyValuePair< ::std::string, ::std::string > > &Entries();

            /**
             * @brief Access the next section in the linked list.
             * @return Shared pointer to the next section.
             */
            ::std::shared_ptr< IniFileHeading >                               &Next();

            /**
             * @brief Set a key/value pair in this section.
             * @param a_key Key name; must be non-null.
             * @param a_value Value string; must be non-null.
             * @return True if the value was set; false on invalid input.
             */
            bool SetValue( const char *a_key, const char *a_value );

            /**
             * @brief Retrieve a key/value pair from this section.
             * @param a_key Key name; must be non-null.
             * @param a_value Output value string.
             * @return True if the key was found; false otherwise.
             */
            bool GetValue( const char *a_key, ::std::string &a_value );
    };

    /**
     * @brief INI file parser and writer.
     * @details Loads and caches INI content from disk, exposing read/write
     *          helpers for sectioned key/value data.
     * @note Not safe for concurrent access without external synchronization.
     */
    class IniFile : public Lockable
    {
        private:
            File                                m_file;
            uint32_t                            m_modTime;
            ::std::shared_ptr< IniFileHeading > m_heading;
            bool                                m_loading;

            void LoadFile();
            void SaveFile();

        public:
            /**
             * @brief Construct an INI file wrapper for a given path.
             * @param a_file Path to the INI file; must be non-null.
             */
            IniFile( const char *a_file );

            /**
             * @brief Destroy the INI file wrapper.
             */
            ~IniFile();

            /**
             * @brief Read a value from a section.
             * @param a_heading Section name; must be non-null.
             * @param a_name Key name; must be non-null.
             * @param a_value Output value string.
             * @return True if the value was found; false otherwise.
             */
            bool ReadValue ( const char *a_heading, const char *a_name, ::std::string &a_value );

            /**
             * @brief Write a value to a section.
             * @param a_heading Section name; must be non-null.
             * @param a_name Key name; must be non-null.
             * @param a_value Value string; must be non-null.
             * @return True if the value was written; false otherwise.
             */
            bool WriteValue( const char *a_heading, const char *a_name, const char *a_value );
    };
}

#endif // _INIFILE_HPP_
