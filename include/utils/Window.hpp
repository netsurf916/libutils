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

    class Window : public Lockable
    {
        private:
            int row, col;

        public:
            Window();
            ~Window();

            void Update();
            void GetMax( int &a_row, int &a_col );
            void Put( char a_ch, int a_color = ColorPair::WhiteOnBlack );
            void PutRND( char a_ch, int a_color = ColorPair::WhiteOnBlack );
            void PutRND( const char *a_str, int a_color = ColorPair::WhiteOnBlack );
    };
}

#endif // _WINDOW_HPP_