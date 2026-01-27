/**
    HttpAccess.hpp : HttpAccess definition
    Description: HTTP basic authentication helper.
    Copyright 2026 Daniel Wilson
    Created with the help of ChatGPT.
    SPDX-License-Identifier: MIT
*/

#pragma once

#ifndef _HTTPACCESS_HPP_
#define _HTTPACCESS_HPP_

#include <utils/File.hpp>
#include <utils/Lockable.hpp>
#include <utils/Types.hpp>
#include <memory>
#include <string>
#include <vector>

namespace utils
{
    class HttpRequest;
    class IniFile;
    class Socket;

    /**
     * @brief Basic HTTP access control using htpasswd-style credentials.
     * @details Reads a user/password file in the form `user:password` and
     *          validates HTTP Basic Authorization headers. Supports common
     *          htpasswd hashes including crypt-style entries and {SHA}.
     */
    class HttpAccess : public Lockable
    {
        private:
            struct Entry
            {
                ::std::string user;
                ::std::string pass;
            };

            ::std::vector< Entry >             m_entries;
            ::std::string                      m_file;
            ::std::shared_ptr< File >          m_fileHandle;
            uint32_t               m_modTime;
            bool                   m_enabled;
            bool                   m_loaded;
            ::std::string          m_realm;

            bool RefreshIfNeeded();
            bool LoadEntries();
            bool ParseEntry( const ::std::string &a_line );
            bool CheckCredentials( const ::std::string &a_user, const ::std::string &a_pass );
            bool DecodeBase64( const ::std::string &a_input, ::std::string &a_output ) const;
            int  Base64Value( char a_char ) const;
            void Sha1( const uint8_t *a_data, size_t a_len, uint8_t a_output[ 20 ] ) const;
            ::std::string Base64Encode( const uint8_t *a_data, size_t a_len ) const;

        public:
            /**
             * @brief Construct a new access controller.
             */
            HttpAccess();

            /**
             * @brief Configure access from an INI file.
             * @details Reads the htpasswd path from [settings] access and an
             *          optional realm from [settings] realm.
             * @param a_ini INI file to read.
             * @return True if a path was configured; false otherwise.
             */
            bool Configure( IniFile &a_ini );

            /**
             * @brief Check whether access control is enabled.
             * @return True if enabled; false otherwise.
             */
            bool Enabled() const;

            /**
             * @brief Validate the Authorization header for a request.
             * @param a_request Request to inspect.
             * @return True if authorized or access is disabled; false otherwise.
             */
            bool IsAuthorized( HttpRequest &a_request );

            /**
             * @brief Respond with a 401 Unauthorized challenge.
             * @param a_socket Socket to send the response to.
             * @return HTTP status code (401) or negative on error.
             */
            int32_t RespondUnauthorized( ::std::shared_ptr< Socket > &a_socket );
    };
}

#endif // _HTTPACCESS_HPP_
