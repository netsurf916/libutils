    /**
    HttpHelpers.hpp : HttpHelpers implementation
    Copyright 2026 Daniel Wilson
*/

#include <utils/HttpHelpers.hpp>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>

#define MAXBUFFERLEN 65536

namespace utils
{    
    uint8_t HttpHelpers::HexToInt( char a_value )
    {
        if( ( a_value >= '0' ) && ( a_value <= '9' ) )
        {
            return ( a_value - '0' );
        }
        if( ( a_value >= 'A' ) && ( a_value <= 'Z' ) )
        {
            a_value += ( 'a' - 'A' );
        }
        switch( a_value )
        {
            case 'a':
                return 10;
            case 'b':
                return 11;
            case 'c':
                return 12;
            case 'd':
                return 13;
            case 'e':
                return 14;
            case 'f':
                return 15;
            default:
                return 128;
        }
    }

    char HttpHelpers::IntToHex( uint8_t a_value )
    {
        const char hex[] = "0123456789ABCDEF";
        return hex[ a_value & 0x0F ];
    }

    ::std::string HttpHelpers::HtmlEscape( const ::std::string &a_string )
    {
        ::std::string newString;

        for( uint32_t i = 0; i < a_string.length(); ++i )
        {
            switch( a_string[ i ] )
            {
                case '&': newString += "&amp;";
                    break;
                case '<': newString += "&lt;";
                    break;
                case '>': newString += "&gt;";
                    break;
                case '"': newString += "&quot;";
                    break;
                case '\'': newString += "&#39;";
                    break;
                default: newString += a_string[ i ];
                    break;
            }
        }

        return newString;
    }


    ::std::string HttpHelpers::UriEncode( const ::std::string &a_string )
    {
        ::std::string newString;

        for( uint32_t i = 0; i < a_string.length(); ++i )
        {
            if( !Tokens::IsLetter( ( uint8_t )a_string[ i ] ) &&
                !Tokens::IsNumber( ( uint8_t )a_string[ i ] ) &&
                ( a_string[ i ] != '-' ) &&
                ( a_string[ i ] != '_' ) &&
                ( a_string[ i ] != '.' ) &&
                ( a_string[ i ] != '~' ) )
            {
                newString += "%";
                newString += HttpHelpers::IntToHex( a_string[ i ] >>   4 );
                newString += HttpHelpers::IntToHex( a_string[ i ] & 0x0F );
            }
            else
            {
                newString += a_string[ i ];
            }
        }

        return newString;
    }

    ::std::string HttpHelpers::UriDecode( const ::std::string &a_string )
    {
        ::std::string newString;

        for( uint32_t i = 0; i < a_string.length(); ++i )
        {
            switch( a_string[ i ] )
            {
                case '%':
                    if( i < ( uint32_t )( a_string.length() - 2 ) )
                    {
                        char a = HttpHelpers::HexToInt( a_string[ i + 1 ] );
                        char b = HttpHelpers::HexToInt( a_string[ i + 2 ] );
                        if( ( a <= 0x0F ) && ( b <= 0x0F ) )
                        {
                            newString += ( char )( ( a << 4 ) & 0xF0 ) +
                                        ( char )( ( b      ) & 0x0F );
                            i += 2;
                        }
                    }
                    break;
                case '+':
                    newString += ' ';
                    break;
                default:
                    newString += a_string[ i ];
                    break;
            }
        }

        return newString;
    }

    uint32_t HttpHelpers::UriDecode( ::std::string &a_uri, ::std::string &a_ext )
    {
        ::std::string newUri;
        uint32_t changes = 0;

        for( uint32_t i = 0; i < a_uri.length(); ++i )
        {
            switch( a_uri[ i ] )
            {
                case '/':
                case '\\':
                    a_ext.clear();
                    if( ( newUri.length() > 0 ) && ( '/' != newUri[ newUri.length() - 1 ] ) )
                    {
                        newUri += '/';
                    }
                    break;
                case '.':
                    a_ext = '.';
                    if( ( newUri.length() > 0 ) && ( '.' != newUri[ newUri.length() - 1 ] ) )
                    {
                        newUri += '.';
                    }
                    break;
                case '%':
                    if( i < ( uint32_t )( a_uri.length() - 2 ) )
                    {
                        char a = HttpHelpers::HexToInt( a_uri[ i + 1 ] );
                        char b = HttpHelpers::HexToInt( a_uri[ i + 2 ] );
                        if( ( a <= 0x0F ) && ( b <= 0x0F ) )
                        {
                            newUri += ( char )( ( a << 4 ) & 0xF0 ) +
                                      ( char )( ( b      ) & 0x0F );
                            ++changes;
                            i += 2;
                        }
                    }
                    break;
                case '+':
                    newUri += ' ';
                    break;
                default:
                    newUri += a_uri[ i ];
                    if( a_ext.length() > 0 )
                    {
                        a_ext += a_uri[ i ];
                    }
                    break;
            }
        }
        Tokens::MakeLower( a_ext );
        a_uri = newUri;

        return changes;
    }

    bool HttpHelpers::UriDecode( ::std::string &a_base, ::std::string &a_defaultDoc, ::std::string &a_uri, ::std::string &a_ext, ::std::string &a_defmime )
    {
        while( HttpHelpers::UriDecode( a_uri, a_ext ) != 0 );

        ::std::string newUri( a_base );
        if( ( newUri.length() > 0 ) && ( '/' != newUri[ newUri.length() - 1 ] ) )
        {
            newUri += '/';
        }
        newUri += a_uri;
        bool isDir = IsDirectory( newUri );
        if( isDir && ( newUri.length() > 0 ) && ( '/' != newUri[ newUri.length() - 1 ] ) )
        {
            newUri += '/';
        }

        // If this is a directory, check if the default document exists
        if( isDir || ( a_ext.length() == 0 ) )
        {
            ::std::string newDefUri( newUri );
            while( HttpHelpers::UriDecode( a_defaultDoc, a_ext ) != 0 );
            newDefUri += a_defaultDoc;

            // Use the default document if it exists
            if( IsFile( newDefUri ) )
            {
                newUri = newDefUri;
            }
            else
            {
                // Clear the extension in the case the default document isn't used
                a_ext.clear();
                // Treating it like a directory either way at this point
                isDir = true;
            }
        }

        if( a_ext.length() == 0 )
        {
            a_ext = a_defmime;
        }
        a_uri = newUri;

        if( a_uri.length() > 0 )
        {
            return true;
        }
        return false;
    }

    bool HttpHelpers::IsDirectory( ::std::string &a_path )
    {
        struct stat st;
        if( stat( a_path.c_str(), &st ) != 0 )
        {
            return false;
        }
        return S_ISDIR( st.st_mode );
    }

    bool HttpHelpers::IsFile( ::std::string &a_path )
    {
        struct stat st;
        if( stat( a_path.c_str(), &st ) != 0 )
        {
            return false;
        }
        return S_ISREG( st.st_mode );
    }
}
