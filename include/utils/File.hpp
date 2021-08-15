/**
    File.hpp : File definition
    Copyright 2015-2021 Daniel Wilson
*/

#pragma once

#ifndef _FILE_HPP_
#define _FILE_HPP_

#include <utils/Utils.hpp>
#include <stdio.h>

namespace utils
{
    namespace FileModes
    {
        enum Modes : uint8_t
        {
            Read         =  1,  // 'r'
            Write        =  2,  // 'w'
            Append       =  4,  // 'a'
            Binary       =  8,  // 'b'
            Update       = 16,  // '+'
            DefaultRead  = ( Read  | Binary          ), // Used for Serialize()
            DefaultWrite = ( Write | Binary | Update ), // Used for Deserialize()
        };
    }
    typedef FileModes::Modes FileMode;

    class File : public Readable,
                 public Writable
    {
        protected:
            ::std::string  m_fileName;
            uint32_t       m_mode;
            FILE          *m_file;
            bool           m_ready;

        public:
            File( FILE *a_file, uint32_t a_mode = FileMode::DefaultRead );
            File( const char *a_fileName, uint32_t a_mode = FileMode::DefaultRead );
            ~File();

            ::std::string &Name();
            operator const char *();

            void     SetMode( uint32_t a_mode = FileMode::DefaultRead );
            uint64_t Size();
            int64_t  Position();
            bool     Exists();
            uint32_t ModificationTime();
            bool     Seek( int64_t a_position );

            // Read functions
            bool     IsReadable() noexcept final;
            bool     Read( uint8_t &a_value, bool a_block = false ) noexcept final;
            uint32_t Read( uint8_t *a_value, uint32_t a_length, bool a_block = false ) noexcept final;
            bool     Read( ::std::shared_ptr< Buffer > &a_buffer, bool a_block = false ) noexcept;
            bool     Peek( uint8_t &a_value ) noexcept final;
            uint32_t Peek( uint8_t *a_value, uint32_t a_length ) noexcept final;
            bool     Peek( ::std::shared_ptr< Buffer > &a_buffer ) noexcept;

            // Write functions
            bool     IsWritable() noexcept final;
            bool     Write( const uint8_t &a_value ) noexcept final;
            uint32_t Write( const uint8_t *a_value, uint32_t a_length ) noexcept final;
            bool     Write( ::std::shared_ptr< Buffer > &a_buffer ) noexcept;

            bool   Delete();
            bool   Close();

        protected:
            bool   Open();
    };
}

#endif // _FILE_HPP_
