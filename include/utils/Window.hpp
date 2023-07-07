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
    class Window : public Lockable
    {
        private:
            int row, col;

        public:
            Window();
            ~Window();

            void Update();
            void GetMax( int &row, int &col );
            void Put( char a );
            void PutRND( char a );
            void PutRND( const char *a );
    };
}

#endif // _WINDOW_HPP_