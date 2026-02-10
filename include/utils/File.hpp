/**
    File.hpp : File definition
    Description: File wrapper for buffered file IO.
    Copyright 2015-2026 Daniel Wilson
    SPDX-License-Identifier: MIT
*/

#pragma once

#ifndef _FILE_HPP_
#define _FILE_HPP_

#include <utils/Buffer.hpp>
#include <utils/Readable.hpp>
#include <utils/Types.hpp>
#include <utils/Writable.hpp>
#include <memory>
#include <string>
#include <stdio.h>
#include <sys/stat.h>

namespace utils
{
    namespace FileModes
    {
        /**
         * @brief File mode flags used to open and operate on files.
         * @note These are bitwise flags that can be combined.
         */
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

    /**
     * @brief Buffered file wrapper implementing Readable/Writable interfaces.
     * @details Manages a FILE* handle and provides convenience methods for
     *          common file operations. Operations expect a valid file name or
     *          file handle and may fail if the underlying OS call fails.
     * @note Not safe for concurrent access without external synchronization.
     */
    class File : public Readable,
                 public Writable
    {
        protected:
            ::std::string  m_fileName;
            uint32_t       m_mode;
            struct stat    m_cachedStat;
            bool           m_cachedStatValid;
            bool           m_cachedPathExists;
            FILE          *m_file;
            bool           m_ready;

        public:
            /**
             * @brief Construct a file wrapper from an existing FILE*.
             * @param a_file Existing FILE*; ownership is not transferred.
             * @param a_mode Open mode flags to use for operations.
             */
            File( FILE *a_file, uint32_t a_mode = FileMode::DefaultRead );

            /**
             * @brief Construct a file wrapper for a path.
             * @param a_fileName Path to the file; must be non-null.
             * @param a_mode Open mode flags to use when opening.
             */
            File( const char *a_fileName, uint32_t a_mode = FileMode::DefaultRead );

            /**
             * @brief Destroy the file wrapper and close any open handle.
             */
            ~File();

            /**
             * @brief Access the current file name.
             * @return Reference to the stored file name string.
             */
            ::std::string &Name();

            /**
             * @brief Convert to a C-string of the file name.
             * @return Null-terminated file name string.
             */
            operator const char *();

            /**
             * @brief Set or update the mode flags used for opening.
             * @param a_mode Open mode flags (bitwise combination).
             */
            void     SetMode( uint32_t a_mode = FileMode::DefaultRead );

            /**
             * @brief Get file size in bytes.
             * @return Size in bytes; 0 if unavailable.
             */
            uint64_t Size();

            /**
             * @brief Get the current file position.
             * @return Current offset in bytes; negative on error.
             */
            int64_t  Position();

            /**
             * @brief Check whether the file exists.
             * @return True if the file exists; false otherwise.
             */
            bool     Exists();

            /**
             * @brief Check whether the path refers to a regular file.
             * @return True if the path is a file; false otherwise.
             */
            bool     IsFile();

            /**
             * @brief Check whether the path refers to a directory.
             * @return True if the path is a directory; false otherwise.
             */
            bool     IsDirectory();

            /**
             * @brief Check whether the path has been modified.
             * @return True if the path has been modified.
             */
            bool     IsModified();

            /**
             * @brief Get the file's modification time.
             * @return Modification time as a UNIX timestamp (seconds).
             */
            uint32_t ModificationTime();

            /**
             * @brief Seek to a new file position.
             * @param a_position Absolute byte offset to seek to.
             * @return True on success; false on error or unopened file.
             */
            bool     Seek( int64_t a_position );

            // Read functions

            /**
             * @brief Check if the file is ready for reading.
             * @return True if readable; false otherwise.
             */
            bool     IsReadable() noexcept final;

            /**
             * @brief Read a single byte from the file.
             * @param a_value Output byte to receive data.
             * @param a_block Ignored; included for interface compatibility.
             * @return True if a byte was read; false on EOF or error.
             */
            bool     Read( uint8_t &a_value, bool a_block = false ) noexcept final;

            /**
             * @brief Read up to a_length bytes from the file.
             * @param a_value Destination buffer; must be non-null when a_length > 0.
             * @param a_length Number of bytes to read.
             * @param a_block Ignored; included for interface compatibility.
             * @return Number of bytes read; 0 on EOF or error.
             */
            uint32_t Read( uint8_t *a_value, uint32_t a_length, bool a_block = false ) noexcept final;

            /**
             * @brief Read file data into a Buffer.
             * @param a_buffer Destination buffer; must be non-null.
             * @param a_block Ignored; included for interface compatibility.
             * @return True if any data was read; false otherwise.
             */
            bool     Read( ::std::shared_ptr< Buffer > &a_buffer, bool a_block = false ) noexcept;

            /**
             * @brief Peek at the next byte without advancing the position.
             * @param a_value Output byte to receive the data.
             * @return True if a byte is available; false on EOF or error.
             */
            bool     Peek( uint8_t &a_value ) noexcept final;

            /**
             * @brief Peek up to a_length bytes without advancing the position.
             * @param a_value Destination buffer; must be non-null when a_length > 0.
             * @param a_length Number of bytes to peek.
             * @return Number of bytes copied; 0 on EOF or error.
             */
            uint32_t Peek( uint8_t *a_value, uint32_t a_length ) noexcept final;

            /**
             * @brief Peek file data into a Buffer without advancing the position.
             * @param a_buffer Destination buffer; must be non-null.
             * @return True if any data was peeked; false otherwise.
             */
            bool     Peek( ::std::shared_ptr< Buffer > &a_buffer ) noexcept;

            // Write functions

            /**
             * @brief Check if the file is ready for writing.
             * @return True if writable; false otherwise.
             */
            bool     IsWritable() noexcept final;

            /**
             * @brief Write a single byte to the file.
             * @param a_value Byte to write.
             * @return True if the byte was written; false otherwise.
             */
            bool     Write( const uint8_t &a_value ) noexcept final;

            /**
             * @brief Write bytes to the file.
             * @param a_value Source buffer; must be non-null when a_length > 0.
             * @param a_length Number of bytes to write.
             * @return Number of bytes written; 0 on error.
             */
            uint32_t Write( const uint8_t *a_value, uint32_t a_length ) noexcept final;

            /**
             * @brief Write buffer contents to the file.
             * @param a_buffer Source buffer; must be non-null.
             * @return True if any data was written; false otherwise.
             */
            bool     Write( ::std::shared_ptr< Buffer > &a_buffer ) noexcept;

            /**
             * @brief Delete the file from the filesystem.
             * @return True if deletion succeeded; false otherwise.
             */
            bool     Delete();

            /**
             * @brief Close the file handle if open.
             * @return True on success; false if close failed.
             */
            bool     Close();

        protected:
            /**
             * @brief Mark cached filesystem metadata as stale.
             */
            void     InvalidateStatCache();

            /**
             * @brief Refresh cached stat metadata.
             * @param a_forceRefresh Force a filesystem query even if cached.
             * @return True if stat data is available for this path/handle.
             */
            bool     RefreshStatCache( bool a_forceRefresh = false );

            /**
             * @brief Open the file using the stored name and mode flags.
             * @return True on success; false otherwise.
             */
            bool     Open();
    };
}

#endif // _FILE_HPP_
