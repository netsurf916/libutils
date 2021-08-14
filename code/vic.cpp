#include <stdio.h>
#include <string>

/* Based on this straddling checker box as used for
 * the Consumer Product Security award coin.
 *      ┌───┬───┬───┬───┬───┬───┬───┬───┬───┬───┐
 *      │ 0 │ 1 │ 2 │ 3 │ 4 │ 5 │ 6 │ 7 │ 8 │ 9 │
 *  ┌───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┤
 *  │   │   │ R │ N │ A │   │ S │ I │ O │ E │ T │
 *  ├───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┤
 *  │ 0 │ D │ Q │ H │ F │ J │ . │ C │ Y │ X │ P │
 *  ├───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┤
 *  │ 4 │ G │ M │ W │ V │ U │ B │ K │ L │ / │ Z │
 *  └───┴───┴───┴───┴───┴───┴───┴───┴───┴───┴───┘
 */

using namespace std;

size_t VicEncode( string &a_input, string &a_output )
{
    size_t count  = 0;
    bool   number = false;
    for( auto i = a_input.begin(); i < a_input.end(); ++i )
    {
        char input = *i;
        // Make lowercase
        if( ( input >= 'A' ) && ( input <= 'Z' ) )
        {
            input -= 'A';
            input += 'a';
        }
        switch( input )
        {
            case 'a':
                a_output += "3";
                ++count;
                break;
            case 'b':
                a_output += "45";
                ++count;
                break;
            case 'c':
                a_output += "06";
                ++count;
                break;
            case 'd':
                a_output += "00";
                ++count;
                break;
            case 'e':
                a_output += "8";
                ++count;
                break;
            case 'f':
                a_output += "03";
                ++count;
                break;
            case 'g':
                a_output += "40";
                ++count;
                break;
            case 'h':
                a_output += "02";
                ++count;
                break;
            case 'i':
                a_output += "6";
                ++count;
                break;
            case 'j':
                a_output += "04";
                ++count;
                break;
            case 'k':
                a_output += "46";
                ++count;
                break;
            case 'l':
                a_output += "47";
                ++count;
                break;
            case 'm':
                a_output += "41";
                ++count;
                break;
            case 'n':
                a_output += "2";
                ++count;
                break;
            case 'o':
                a_output += "7";
                ++count;
                break;
            case 'p':
                a_output += "09";
                ++count;
                break;
            case 'q':
                a_output += "01";
                ++count;
                break;
            case 'r':
                a_output += "1";
                ++count;
                break;
            case 's':
                a_output += "5";
                ++count;
                break;
            case 't':
                a_output += "9";
                ++count;
                break;
            case 'u':
                a_output += "44";
                ++count;
                break;
            case 'v':
                a_output += "43";
                ++count;
                break;
            case 'w':
                a_output += "42";
                ++count;
                break;
            case 'x':
                a_output += "08";
                ++count;
                break;
            case 'y':
                a_output += "07";
                ++count;
                break;
            case 'z':
                a_output += "49";
                ++count;
                break;
            case '.':
                a_output += "05";
                ++count;
                break;
            case '/':
                a_output += "48";
                ++count;
                number = !number;
                break;
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
                if( number )
                {
                    a_output += input;
                    ++count;
                }
                else
                {
                    return count;
                }
                break;
            case ' ':
                ++count;
                break;
            default:
                return count;
                break;
        }
    }
    return count;
}

size_t VicDecode( string &a_input, string &a_output )
{
    size_t count  = 0;
    int    state  = 0;
    for( auto i = a_input.begin(); i < a_input.end(); ++i )
    {
        char input = *i;
        switch( state )
        {
            case '0':
                switch( input )
                {
                    case '0':
                        a_output += "d";
                        ++count;
                        break;
                    case '1':
                        a_output += "q";
                        ++count;
                        break;
                    case '2':
                        a_output += "h";
                        ++count;
                        break;
                    case '3':
                        a_output += "f";
                        ++count;
                        break;
                    case '4':
                        a_output += "j";
                        ++count;
                        break;
                    case '5':
                        a_output += ".";
                        ++count;
                        break;
                    case '6':
                        a_output += "c";
                        ++count;
                        break;
                    case '7':
                        a_output += "y";
                        ++count;
                        break;
                    case '8':
                        a_output += "x";
                        ++count;
                        break;
                    case '9':
                        a_output += "p";
                        ++count;
                        break;
                    default:
                        return count;
                        break;
                }
                state = 0;
                break;
            case '4':
                switch( input )
                {
                    case '0':
                        a_output += "g";
                        state = 0;
                        ++count;
                        break;
                    case '1':
                        a_output += "m";
                        state = 0;
                        ++count;
                        break;
                    case '2':
                        a_output += "w";
                        state = 0;
                        ++count;
                        break;
                    case '3':
                        a_output += "v";
                        state = 0;
                        ++count;
                        break;
                    case '4':
                        a_output += "u";
                        state = 0;
                        ++count;
                        break;
                    case '5':
                        a_output += "b";
                        state = 0;
                        ++count;
                        break;
                    case '6':
                        a_output += "k";
                        state = 0;
                        ++count;
                        break;
                    case '7':
                        a_output += "l";
                        state = 0;
                        ++count;
                        break;
                    case '8':
                        a_output += "/";
                        state = 'n';
                        ++count;
                        break;
                    case '9':
                        a_output += "z";
                        state = 0;
                        ++count;
                        break;
                    default:
                        return count;
                        break;
                }
                break;
            case 'n':
                switch( input )
                {
                    case '4':
                        state = 'N';
                        ++count;
                        break;
                    case '0':
                    case '1':
                    case '2':
                    case '3':
                    case '5':
                    case '6':
                    case '7':
                    case '8':
                    case '9':
                        a_output += input;
                        ++count;
                        break;
                    default:
                        return count;
                        break;
                }
                break;
            case 'N':
                switch( input )
                {
                    case '8':
                        a_output += '/';
                        state = 0;
                        ++count;
                        break;
                    case '0':
                    case '1':
                    case '2':
                    case '3':
                    case '5':
                    case '6':
                    case '7':
                    case '9':
                        a_output += '4';
                        a_output += input;
                        state = 'n';
                        ++count;
                        break;
                    case '4':
                        a_output += '4';
                        state = 'N';
                        ++count;
                        break;
                    default:
                        return count;
                        break;
                }
                break;
            default:
                switch( input )
                {
                    case '0':
                        state = '0';
                        ++count;
                        break;
                    case '1':
                        a_output += "r";
                        ++count;
                        break;
                    case '2':
                        a_output += "n";
                        ++count;
                        break;
                    case '3':
                        a_output += "a";
                        ++count;
                        break;
                    case '4':
                        state = '4';
                        ++count;
                        break;
                    case '5':
                        a_output += "s";
                        ++count;
                        break;
                    case '6':
                        a_output += "i";
                        ++count;
                        break;
                    case '7':
                        a_output += "o";
                        ++count;
                        break;
                    case '8':
                        a_output += "e";
                        ++count;
                        break;
                    case '9':
                        a_output += "t";
                        ++count;
                        break;
                    default:
                        return count;
                        break;
                }
                break;
        }
    }
    if( ( count == a_input.size() ) && ( state != 0 ) )
    {
        // This will most likely add an additional character, but
        // it also prevents data from getting lost.  The issue is
        // that the string could end in either 0 or 4 due to the
        // use of ApplyOffset().  In this case, the 0 or 4 is
        // dropped unless we add additional information (which,
        // of course, violates a core tenant of cryptography).
        // Another solution is to reject offset values which
        // result in this scenario.
        switch( state )
        {
            case '0':
                a_output += 'd';
                break;
            case '4':
                a_output += 'g';
                break;
        }
    }
    return count;
}

size_t ApplyOffset( string &a_offset, string &a_input )
{
    size_t count = 0;
    bool   add   = true;
    if( ( a_offset.size() < 2 ) || ( 0 == a_input.size() ) )
    {
        return count;
    }
    if( a_offset[ 0 ] == '-' )
    {
        add = false;
        a_offset.erase( 0, 1 );
    }
    else if( a_offset[ 0 ] == '+' )
    {
        a_offset.erase( 0, 1 );
    }
    for( auto i = a_input.begin(); i < a_input.end(); ++i )
    {
        if( ( *i < '0' ) || ( *i > '9' ) )
        {
            return count;
        }
        if( ( a_offset[ count % a_offset.size() ] < '0' ) || ( a_offset[ count % a_offset.size() ] > '9' ) )
        {
            return count;
        }
        int input = ( *i - '0' );
        if( add )
        {
            input += ( a_offset[ count % a_offset.size() ] - '0' );
            if( input > 9 )
            {
                input -= 10;
            }
        }
        else
        {
            input -= ( a_offset[ count % a_offset.size() ] - '0' );
            if( input < 0 )
            {
                input += 10;
            }
        }
        *i = ( input + '0' );
        ++count;
    }
    return count;
}

int main( int argc, char *argv[] )
{
    if( argc < 3 )
    {
        printf( "\n\n" );
        printf( "     ┌───┬───┬───┬───┬───┬───┬───┬───┬───┬───┐\n" );
        printf( "     │ 0 │ 1 │ 2 │ 3 │ 4 │ 5 │ 6 │ 7 │ 8 │ 9 │\n" );
        printf( " ┌───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┤\n" );
        printf( " │   │   │ R │ N │ A │   │ S │ I │ O │ E │ T │\n" );
        printf( " ├───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┤\n" );
        printf( " │ 0 │ D │ Q │ H │ F │ J │ . │ C │ Y │ X │ P │\n" );
        printf( " ├───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┤\n" );
        printf( " │ 4 │ G │ M │ W │ V │ U │ B │ K │ L │ / │ Z │\n" );
        printf( " └───┴───┴───┴───┴───┴───┴───┴───┴───┴───┴───┘\n\n" );
        printf( "Usage: %s <[-]number> <text>\n", argv[ 0 ] );
        printf( "    <[-]number>  A positive or negative number where,\n" );
        printf( "                 negative numbers indicate decrypt and\n" );
        printf( "                 positive numbers indicate encrypt.\n" );
        printf( "    <text>       The text to encrypt/decrypt; may be\n" );
        printf( "                 broken up with spaces.\n\n" );
        return 0;
    }

    string plaintext;
    string ciphertext;
    string offset = argv[ 1 ];
    for( int i = 2; i < argc; ++i )
    {
        plaintext += argv[ i ];
    }
    printf( "[*] Input text: %s\n", plaintext.c_str() );

    if( VicEncode( plaintext, ciphertext ) == plaintext.size() )
    {
        printf( "[*] VIC encode: %s\n", ciphertext.c_str() );
        if( ApplyOffset( offset, ciphertext ) == ciphertext.size() )
        {
            printf( "[*] Add offset: %s\n", ciphertext.c_str() );
            plaintext.clear();
            if( VicDecode( ciphertext, plaintext ) == ciphertext.size() )
            {
                printf( "[*] VIC decode: %s\n", plaintext.c_str() );
            }
            else
            {
                printf( "[!] Error encoding plaintext\n" );
            }
        }
        else
        {
            printf( "[!] Error encoding plaintext\n" );
        }
    }
    else
    {
        printf( "[!] Error encoding plaintext\n" );
    }

    return 0;
}

