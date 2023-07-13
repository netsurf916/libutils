/**
    Window.hpp : Window helper class for ncurses
    Copyright 2023 Daniel Wilson
*/

#pragma once

#ifndef _WINDOW_HPP_
#define _WINDOW_HPP_

#include <utils/Lockable.hpp>

namespace utils
{
    namespace ColorPairs
    {
        // The ncurses color pairs can't start with 0
        enum Pairs : uint8_t
        {
            BlackOnWhite   =  1,
            RedOnBlack     =  2,
            GreenOnBlack   =  3,
            YellowOnBlack  =  4,
            BlueOnBlack    =  5,
            MagentaOnBlack =  6,
            CyanOnBlack    =  7,
            WhiteOnBlack   =  8,
            BlackOnRed     =  9,
            BlackOnGreen   = 10,
            BlackOnYellow  = 11,
            BlackOnBlue    = 12,
            BlackOnMagenta = 13,
            BlackOnCyan    = 14,
            Count          = 14,
        };
    }
    typedef ColorPairs::Pairs ColorPair;

    namespace TextDirections
    {
        enum Directions : uint8_t
        {
            Right     = 0,
            Left      = 1,
            Up        = 2,
            Down      = 3,
            DownRight = 4,
            UpRight   = 5,
            DownLeft  = 6,
            UpLeft    = 7,
            Count     = 8,
        };
    }
    typedef TextDirections::Directions TextDirection;

    class Window : public Lockable
    {
        private:
            int row, col;
            void Update( int a_dir = TextDirection::Right );

        public:
            Window();
            ~Window();

            void GetMax( int &a_row, int &a_col );
            void Put( char a_ch, int a_color = ColorPair::WhiteOnBlack, int a_dir = TextDirection::Right );
            void PutRND( char a_ch, int a_color = ColorPair::WhiteOnBlack, int a_dir = TextDirection::Right );
            void PutRND( const char *a_str, int a_color = ColorPair::WhiteOnBlack, int a_dir = TextDirection::Right );
    };
}

#endif // _WINDOW_HPP_
