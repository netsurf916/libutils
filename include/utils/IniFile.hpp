/**
    IniFile.hpp : INI file definition
    Copyright 2014-2019 Daniel Wilson
*/

#pragma once

#ifndef _INIFILE_HPP_
#define _INIFILE_HPP_

#include <utils/Utils.hpp>
#include <utils/File.hpp>
#include <memory>

namespace utils
{
    class IniFileHeading : public Lockable
    {
        private:
            String                                              m_name;
            ::std::shared_ptr< KeyValuePair< String, String > > m_entries;
            ::std::shared_ptr< IniFileHeading >                 m_next;

        public:
            IniFileHeading();
            ~IniFileHeading();

        public:
            String                                              &Name();
            ::std::shared_ptr< IniFileHeading >                 &Next();
            ::std::shared_ptr< KeyValuePair< String, String > > &Entries();

            bool SetValue( const char *a_key, const char *a_value );
            bool GetValue( const char *a_key, String &a_value );
    };

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
            IniFile( const char *a_file );
            ~IniFile();

            bool ReadValue ( const char *a_heading, const char *a_name, String &a_value );
            bool WriteValue( const char *a_heading, const char *a_name, const char *a_value );
    };
}

#endif // _INIFILE_HPP_
