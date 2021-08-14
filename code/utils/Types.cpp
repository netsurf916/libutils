/**
    Types.cpp : Type size checking implementation
    Copyright 2015-2019 Daniel Wilson
*/

#include <utils/Types.hpp>

namespace utils
{
    /**
        This code checks the defined types against expected sizes at compile time.
        No code needs to call the following macro or function for this to work.

        Any non-zero values passed to the following macro will result in a compile
        time error of passing a negative value as a size.  This works by using bool
        operators to convert the passed in value to either a 1 or a 0.  If 0 is passed
        in, it will be double "NOT"ed and result in 0.  If a value > 0 is passed in,
        the first "NOT" makes it 0 and the second "NOT" makes it 1.  The expression
        will then look like this: char[ -1 ] -- which results in a compile error.
        See: https://scaryreasoner.wordpress.com/2009/02/28/checking-sizeof-at-compile-time/
    */
    #define TYPE_SIZE_CHECK(condition) ( ( void ) sizeof( char[ 1 - 2 * !!( condition ) ] ) )

    void CheckTypeSizes()
    {
        TYPE_SIZE_CHECK( sizeof( int8_t   ) - 1 );
        TYPE_SIZE_CHECK( sizeof( uint8_t  ) - 1 );
        TYPE_SIZE_CHECK( sizeof( int16_t  ) - 2 );
        TYPE_SIZE_CHECK( sizeof( uint16_t ) - 2 );
        TYPE_SIZE_CHECK( sizeof( int32_t  ) - 4 );
        TYPE_SIZE_CHECK( sizeof( uint32_t ) - 4 );
        TYPE_SIZE_CHECK( sizeof( int64_t  ) - 8 );
        TYPE_SIZE_CHECK( sizeof( uint64_t ) - 8 );
    }
}
