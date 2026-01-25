/**
    HttpHelpers.hpp : HttpHelpers definition
    Description: HTTP helper utilities and constants.
    Copyright 2026 Daniel Wilson
    SPDX-License-Identifier: MIT
*/

#pragma once

#ifndef _HTTPHELPERS_HPP_
#define _HTTPHELPERS_HPP_

#include <utils/Types.hpp>
#include <string>

namespace utils
{
    /**
     * @brief Collection of HTTP-related helper utilities.
     * @details Provides small, stateless helpers for encoding/decoding and
     *          path inspection. Functions operate on caller-provided strings
     *          and return new values or update outputs.
     */
    class HttpHelpers
    {
        public:
            /**
             * @brief Convert a single hexadecimal character to its integer value.
             * @param a_value Hexadecimal character (0-9, a-f, A-F).
             * @return Integer value 0-15; undefined if input is not hex.
             */
            static uint8_t       HexToInt( char a_value );

            /**
             * @brief Convert a 0-15 integer value to a hex character.
             * @param a_value Integer value in the range 0-15.
             * @return Lowercase hex character representing the value.
             */
            static char          IntToHex( uint8_t a_value );

            /**
             * @brief HTML-escape a string for safe embedding in markup.
             * @param a_string Source string to escape.
             * @return Escaped copy of the input string.
             */
            static ::std::string HtmlEscape( const ::std::string &a_string );

            /**
             * @brief URI-encode a string.
             * @param a_string Source string to encode.
             * @return Percent-encoded copy of the input string.
             */
            static ::std::string UriEncode( const ::std::string &a_string );

            /**
             * @brief URI-decode a string.
             * @param a_string Source string to decode.
             * @return Decoded copy of the input string.
             */
            static ::std::string UriDecode( const ::std::string &a_string );

            /**
             * @brief Decode a URI and split out the extension.
             * @param a_uri Input URI; updated in place with decoded value.
             * @param a_ext Output extension portion (without dot).
             * @return Length of the decoded URI.
             */
            static uint32_t      UriDecode( ::std::string &a_uri, ::std::string &a_ext );

            /**
             * @brief Decode a URI and resolve defaults for paths and mime types.
             * @param a_base Base directory used for resolution.
             * @param a_defaultDoc Default document name to apply for directories.
             * @param a_uri Input URI; updated in place with decoded value.
             * @param a_ext Output extension portion (without dot).
             * @param a_defmime Default mime type if extension is missing.
             * @return True if decoding/resolution succeeded; false otherwise.
             */
            static bool          UriDecode( ::std::string &a_base, ::std::string &a_defaultDoc, ::std::string &a_uri, ::std::string &a_ext, ::std::string &a_defmime );

            /**
             * @brief Check whether a path refers to a directory.
             * @param a_path Filesystem path to test.
             * @return True if the path is a directory; false otherwise.
             */
            static bool          IsDirectory( ::std::string &a_path );

            /**
             * @brief Check whether a path refers to a regular file.
             * @param a_path Filesystem path to test.
             * @return True if the path is a file; false otherwise.
             */
            static bool          IsFile( ::std::string &a_path );
    };
}

#endif // _HTTPHELPERS_HPP_
