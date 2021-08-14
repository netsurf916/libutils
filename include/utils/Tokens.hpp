/**
    Token.hpp : Token processing helper funcitons
    Copyright 2014-2019 Daniel Wilson
*/

#pragma once

#ifndef _TOKEN_HPP_
#define _TOKEN_HPP_

#include <utils/Types.hpp>
#include <utils/Readable.hpp>
#include <utils/String.hpp>

namespace utils
{
    namespace TokenTypes
    {
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

    class Tokens
    {
        public:
            static bool IsNewLine( uint8_t a_c )
            {
                return ( a_c == static_cast< uint8_t >( '\n' ) );
            }

            static bool IsReturn( uint8_t a_c )
            {
                return ( a_c == static_cast< uint8_t >( '\r' ) );
            }

            static bool IsSpace( uint8_t a_c )
            {
                return ( a_c == static_cast< uint8_t >( ' ' ) || a_c == static_cast< uint8_t >( '\t' ) );
            }

            static bool IsNumber( uint8_t a_c )
            {
                return ( a_c >= static_cast< uint8_t >( '0' ) && a_c <= static_cast< uint8_t >( '9' ) );
            }

            static bool IsLetter( uint8_t a_c )
            {
                return ( ( a_c >= static_cast< uint8_t >( 'a' ) && a_c <= static_cast< uint8_t >( 'z' ) ) || ( a_c >= static_cast< uint8_t >( 'A' ) && a_c <= static_cast< uint8_t >( 'Z' ) ) );
            }

            static bool IsSymbol( uint8_t a_c )
            {
                return ( ( a_c > static_cast< uint8_t >( ' ' ) ) && ( a_c <= static_cast< uint8_t >( '~' ) ) && !IsNumber( a_c ) && !IsLetter( a_c ) );
            }

            static bool IsPrintable( uint8_t a_c )
            {
                return ( ( a_c >= static_cast< uint8_t >( ' ' ) ) && ( a_c <= static_cast< uint8_t >( '~' ) ) );
            }

            static bool IsNotPrintable( uint8_t a_c )
            {
                return ( ( a_c < static_cast< uint8_t >( ' ' ) ) || ( a_c > static_cast< uint8_t >( '~' ) ) );
            }

            /**
                This function reads a token out of the provided buffer
                and writes it into the provided token.  If the optional
                delin param is provided, this function reads all chars
                up to delin as a single token.  Otherwise, an attempt
                is made to determine the token delineation based on
                content.
            */
            static TokenType GetToken( Readable &a_input, String   &a_token, char a_delim = 0 );
            static TokenType GetLine ( Readable &a_input, String   &a_token );
            static TokenType GetLine ( Readable &a_input, Writable &a_output );

            /**
                This function escapes an input String for use in JSON formatted data.
                Note: a_input must not be the same string as a_output.
            */
            static String &EscapeJson( String &a_input, String &a_output );
    };
}

#endif // _TOKEN_HPP_
