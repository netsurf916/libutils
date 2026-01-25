/**
    Description: Tokenization helpers.
    Token.hpp : Token processing helper funcitons
    Copyright 2014-2026 Daniel Wilson
    SPDX-License-Identifier: MIT
*/

#pragma once

#ifndef _TOKEN_HPP_
#define _TOKEN_HPP_

#include <utils/Types.hpp>
#include <utils/Readable.hpp>
#include <utils/Writable.hpp>
#include <string>

namespace utils
{
    namespace TokenTypes
    {
        /**
         * @brief Token classification identifiers.
         */
        enum Types : uint8_t
        {
            NotFound   = 0,
            String     = 1,
            Number     = 2,
            Symbol     = 3,
            Delineated = 4,
            Delimited  = 4,
            Line       = 5,
            Unknown    = 6,
            Start      = NotFound,
            End        = Unknown,
        };
    }
    typedef TokenTypes::Types TokenType;

    /**
     * @brief Tokenization and character classification helpers.
     * @details Provides static utility functions for parsing tokens from
     *          Readable streams and transforming strings.
     */
    class Tokens
    {
        public:
            /**
             * @brief Check if a character is a newline.
             * @param a_c Character value to test.
             * @return True if newline; false otherwise.
             */
            static bool IsNewLine( const uint8_t a_c )
            {
                return ( a_c == static_cast< uint8_t >( '\n' ) );
            }

            /**
             * @brief Check if a character is a carriage return.
             * @param a_c Character value to test.
             * @return True if carriage return; false otherwise.
             */
            static bool IsReturn( const uint8_t a_c )
            {
                return ( a_c == static_cast< uint8_t >( '\r' ) );
            }

            /**
             * @brief Check if a character is space or tab.
             * @param a_c Character value to test.
             * @return True if whitespace; false otherwise.
             */
            static bool IsSpace( const uint8_t a_c )
            {
                return ( a_c == static_cast< uint8_t >( ' ' ) || a_c == static_cast< uint8_t >( '\t' ) );
            }

            /**
             * @brief Check if a character is a decimal digit.
             * @param a_c Character value to test.
             * @return True if 0-9; false otherwise.
             */
            static bool IsNumber( const uint8_t a_c )
            {
                return ( a_c >= static_cast< uint8_t >( '0' ) && a_c <= static_cast< uint8_t >( '9' ) );
            }

            /**
             * @brief Check if a string represents a numeric value.
             * @param a_string String to test.
             * @return True if numeric (optionally signed); false otherwise.
             */
            static bool IsNumber( const ::std::string &a_string )
            {
                bool numeric = false;
                for( size_t i = 0; i < a_string.length(); ++i )
                {
                    if( ( i == 0 ) && ( ( a_string[ i ] == '+' ) || ( a_string[ i ] == '-' ) ) )
                    {
                        continue;
                    }
                    numeric = IsNumber( a_string[ i ] );
                    if( !numeric ) break;
                }
                return numeric;
            }

            /**
             * @brief Check if a character is alphabetic.
             * @param a_c Character value to test.
             * @return True if A-Z or a-z; false otherwise.
             */
            static bool IsLetter( const uint8_t a_c )
            {
                return ( ( a_c >= static_cast< uint8_t >( 'a' ) && a_c <= static_cast< uint8_t >( 'z' ) ) || ( a_c >= static_cast< uint8_t >( 'A' ) && a_c <= static_cast< uint8_t >( 'Z' ) ) );
            }

            /**
             * @brief Check if a character is a symbol (printable, non-alnum).
             * @param a_c Character value to test.
             * @return True if symbol; false otherwise.
             */
            static bool IsSymbol( const uint8_t a_c )
            {
                return ( ( a_c > static_cast< uint8_t >( ' ' ) ) && ( a_c <= static_cast< uint8_t >( '~' ) ) && !IsNumber( a_c ) && !IsLetter( a_c ) );
            }

            /**
             * @brief Check if a character is printable ASCII.
             * @param a_c Character value to test.
             * @return True if printable ASCII; false otherwise.
             */
            static bool IsPrintable( const uint8_t a_c )
            {
                return ( ( a_c >= static_cast< uint8_t >( ' ' ) ) && ( a_c <= static_cast< uint8_t >( '~' ) ) );
            }

            /**
             * @brief Check if a character is non-printable ASCII.
             * @param a_c Character value to test.
             * @return True if non-printable; false otherwise.
             */
            static bool IsNotPrintable( const uint8_t a_c )
            {
                return ( ( a_c < static_cast< uint8_t >( ' ' ) ) || ( a_c > static_cast< uint8_t >( '~' ) ) );
            }

            /**
             * @brief Trim leading/trailing spaces from a string.
             * @param a_string String to modify in place.
             */
            static void TrimSpace( ::std::string &a_string )
            {
                ::std::string temp;
                for( size_t i = 0; i < a_string.length(); ++i )
                {
                    if( ( temp.length() == 0 ) && IsSpace( a_string[ i ] ) )
                    {
                        continue;
                    }
                    temp += a_string[ i ];
                }
                while( ( temp.length() > 0 ) && IsSpace( temp[ temp.length() - 1 ] ) )
                {
                    temp.pop_back();
                }
                a_string = temp;
            }

            /**
             * @brief Convert a string to uppercase ASCII in place.
             * @param a_string String to modify.
             */
            static void MakeUpper( ::std::string &a_string )
            {
                for( size_t i = 0; i < a_string.length(); ++i )
                {
                    if( ( a_string[ i ] >= 'a' ) && ( a_string[ i ] <= 'z' ) )
                    {
                        a_string[ i ] -= 'a';
                        a_string[ i ] += 'A';
                    }
                }
            }

            /**
             * @brief Convert a string to lowercase ASCII in place.
             * @param a_string String to modify.
             */
            static void MakeLower( ::std::string &a_string )
            {
                for( size_t i = 0; i < a_string.length(); ++i )
                {
                    if( ( a_string[ i ] >= 'A' ) && ( a_string[ i ] <= 'Z' ) )
                    {
                        a_string[ i ] -= 'A';
                        a_string[ i ] += 'a';
                    }
                }
            }

            /**
             * @brief Read the next token from a readable stream.
             * @details If a_delim is provided, reads until that delimiter
             *          as a single token. Otherwise, token boundaries are
             *          inferred from content type.
             * @param a_input Input stream to read from.
             * @param a_token Output token string.
             * @param a_delim Optional delimiter character; 0 for auto.
             * @return TokenType indicating what was read or NotFound.
             */
            static TokenType GetToken( Readable &a_input, ::std::string &a_token, char a_delim = 0 );

            /**
             * @brief Read a line from a readable stream into a string.
             * @param a_input Input stream to read from.
             * @param a_token Output line string (without newline).
             * @return TokenType::Line on success, or NotFound on EOF/error.
             */
            static TokenType GetLine ( Readable &a_input, ::std::string &a_token );

            /**
             * @brief Read a line from a readable stream into a writable stream.
             * @param a_input Input stream to read from.
             * @param a_output Output stream to write the line to.
             * @return TokenType::Line on success, or NotFound on EOF/error.
             */
            static TokenType GetLine ( Readable &a_input, Writable &a_output );

            /**
             * @brief Escape a string for safe JSON output.
             * @param a_input Input string to escape; must differ from a_output.
             * @param a_output Output string to receive escaped data.
             * @return Reference to a_output.
             * @note a_input must not be the same object as a_output.
             */
            static ::std::string &EscapeJson( ::std::string &a_input, ::std::string &a_output );
    };
}

#endif // _TOKEN_HPP_
