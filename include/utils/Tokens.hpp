/**
    Token.hpp : Token processing helper funcitons
    Copyright 2014-2019 Daniel Wilson
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
                This function reads a token out of the provided buffer
                and writes it into the provided token.  If the optional
                delin param is provided, this function reads all chars
                up to delin as a single token.  Otherwise, an attempt
                is made to determine the token delineation based on
                content.
            */
            static TokenType GetToken( Readable &a_input, ::std::string &a_token, char a_delim = 0 );
            static TokenType GetLine ( Readable &a_input, ::std::string &a_token );
            static TokenType GetLine ( Readable &a_input, Writable &a_output );

            /**
                This function escapes an input ::std::string for use in JSON formatted data.
                Note: a_input must not be the same string as a_output.
            */
            static ::std::string &EscapeJson( ::std::string &a_input, ::std::string &a_output );
    };
}

#endif // _TOKEN_HPP_
