/**
    Token.cpp : Token processing helper funcitons
    Copyright 2014-2021 Daniel Wilson
*/

#include <utils/Tokens.hpp>
#include <utils/Buffer.hpp>

namespace utils
{
    TokenType Tokens::GetToken( Readable &a_input, ::std::string &a_token, char a_delim /*= 0*/ )
    {
        bool      ok   = true;
        uint8_t   c    = 0;
        TokenType type = TokenTypes::NotFound;

        if( !a_input.IsReadable() )
        {
            return type;
        }

        // Clear the output token
        a_token.clear();

        // Get a token
        if( a_delim != 0 )
        {
            // If a delimiter is provided, behave like split()
            while( ok && a_input.Peek( c ) && ( c != a_delim ) )
            {
                ok = a_input.Read( c );
                if( ok )
                {
                    a_token += c;
                }
            }
            if( ok && ( c == a_delim ) )
            {
                ok = a_input.Read( c );
            }
            if( ok )
            {
                type = TokenTypes::Delineated;
            }
        }
        else
        {
            // Clear any leading whitespace
            while( ok && a_input.Peek( c ) && IsSpace( c ) )
            {
                ok = a_input.Read( c );
            }

            // Clear any non-printable characters
            while( ok && a_input.Peek( c ) && IsNotPrintable( c ) )
            {
                ok = a_input.Read( c );
            }

            // Get a number
            if( ok && a_input.Peek( c ) && IsNumber( c ) )
            {
                do
                {
                    ok = a_input.Read( c );
                    if( ok )
                    {
                        a_token += c;
                    }
                } while( ok && a_input.Peek( c ) && IsNumber( c ) );
                if( ok && ( a_token.length() > 0 ) )
                {
                    type = TokenTypes::Number;
                }
            }
            // Get a string
            else if( ok && a_input.Peek( c ) && IsLetter( c ) )
            {
                do
                {
                    ok = a_input.Read( c );
                    if( ok )
                    {
                        a_token += c;
                    }
                } while( ok && a_input.Peek( c ) && IsLetter( c ) );
                if( ok && ( a_token.length() > 0 ) )
                {
                    type = TokenTypes::String;
                }
            }
            // Get a symbol
            else if( ok && a_input.Peek( c ) && IsSymbol( c ) )
            {
                ok = a_input.Read( c );
                if( ok )
                {
                    a_token += c;
                    type = TokenTypes::Symbol;
                }
            }
            else if( ok )
            {
                ok = a_input.Read( c );
                if( ok )
                {
                    a_token += c;
                    type = TokenTypes::Unknown;
                }
            }
        }

        return type;
    }

    TokenType Tokens::GetLine( Readable &a_input, ::std::string &a_token )
    {
        bool      ok   = true;
        uint8_t   c    = 0;
        TokenType type = TokenTypes::NotFound;

        if( !a_input.IsReadable() )
        {
            return type;
        }
        a_token.clear();

        // Get a line
        while( ok && a_input.Peek( c ) )
        {
            type = TokenTypes::Line;
            if( IsNewLine( c ) || IsReturn( c ) )
            {
                break;
            }
            ok = ok && a_input.Read( c );
            if( ok )
            {
                a_token += c;
            }
        }

        // Strip off line end (valid endings are "\n" and "\r\n")
        if( ok )
        {
            if( IsReturn( c ) && a_input.Read( c ) )
            {
                ok = a_input.Peek( c );
            }
            if( IsNewLine( c ) )
            {
                ok = a_input.Read( c );
            }
        }

        return type;
    }

    TokenType Tokens::GetLine( Readable &a_input, Writable &a_output )
    {
        bool      ok   = true;
        uint8_t   c    = 0;
        TokenType type = TokenTypes::NotFound;

        if( !a_input.IsReadable() || !a_output.IsWritable() )
        {
            return type;
        }

        // Get a line
        while( ok && a_input.Peek( c ) )
        {
            type = TokenTypes::Line;
            if( IsNewLine( c ) || IsReturn( c ) )
            {
                break;
            }
            ok = ok && a_input.Read( c );
            ok = ok && a_output.Write( c );
        }

        // Strip off line end (valid endings are "\n" and "\r\n")
        if( ok )
        {
            if( IsReturn( c ) && a_input.Read( c ) )
            {
                ok = a_input.Peek( c );
            }
            if( IsNewLine( c ) )
            {
                ok = a_input.Read( c );
            }
        }

        return type;
    }

    ::std::string &Tokens::EscapeJson( ::std::string &a_input, ::std::string &a_output )
    {
        for( uint32_t c = 0; c < a_input.length(); ++c )
        {
            if( ( '\"' == a_input[ c ] ) ||
                ( '\\' == a_input[ c ] ) ||
                ( '/'  == a_input[ c ] ) )
            {
                a_output += "\\";
            }
            else if( '\n' == a_input[ c ] )
            {
                a_output += "\\n";
                continue;
            }
            else if( '\r' == a_input[ c ] )
            {
                a_output += "\\r";
                continue;
            }
            else if( '\t' == a_input[ c ] )
            {
                a_output += "\\t";
                continue;
            }
            else if( '\b' == a_input[ c ] )
            {
                a_output += "\\b";
                continue;
            }
            else if( '\f' == a_input[ c ] )
            {
                a_output += "\\f";
                continue;
            }
            a_output += a_input[ c ];
        }
        return a_output;
    }
}
