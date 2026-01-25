/**
    Window.cpp : Window helper class implementation for ncurses
    Copyright 2023-2026 Daniel Wilson
    SPDX-License-Identifier: MIT
*/

#include <utils/Window.hpp>
#include <utils/Lock.hpp>
#include <ncurses.h>
#include <string.h>
#include <time.h>

namespace utils
{
    Window::Window()
    : row( 0 )
    , col( 0 )
    {
        initscr();
        noecho();
        curs_set( 0 );

        if( has_colors() )
        {
            start_color();
            init_pair( ColorPair::BlackOnWhite,   COLOR_BLACK,   COLOR_WHITE   );
            init_pair( ColorPair::RedOnBlack,     COLOR_RED,     COLOR_BLACK   );
            init_pair( ColorPair::GreenOnBlack,   COLOR_GREEN,   COLOR_BLACK   );
            init_pair( ColorPair::YellowOnBlack,  COLOR_YELLOW,  COLOR_BLACK   );
            init_pair( ColorPair::BlueOnBlack,    COLOR_BLUE,    COLOR_BLACK   );
            init_pair( ColorPair::MagentaOnBlack, COLOR_MAGENTA, COLOR_BLACK   );
            init_pair( ColorPair::CyanOnBlack,    COLOR_CYAN,    COLOR_BLACK   );
            init_pair( ColorPair::WhiteOnBlack,   COLOR_WHITE,   COLOR_BLACK   );
            init_pair( ColorPair::BlackOnRed,     COLOR_BLACK,   COLOR_RED     );
            init_pair( ColorPair::BlackOnGreen,   COLOR_BLACK,   COLOR_GREEN   );
            init_pair( ColorPair::BlackOnYellow,  COLOR_BLACK,   COLOR_YELLOW  );
            init_pair( ColorPair::BlackOnBlue,    COLOR_BLACK,   COLOR_BLUE    );
            init_pair( ColorPair::BlackOnMagenta, COLOR_BLACK,   COLOR_MAGENTA );
            init_pair( ColorPair::BlackOnCyan,    COLOR_BLACK,   COLOR_CYAN    );
        }

        srand( time( NULL ) );
    }

    Window::~Window()
    {
        ::utils::Lock lock( this );
        endwin();
    }

    void Window::GetMax( int &a_row, int &a_col )
    {
        ::utils::Lock lock( this );
        getmaxyx( stdscr, a_row, a_col );
    }

    void Window::Update( int a_dir )
    {
        ::utils::Lock lock( this );
        int mrow = 0,
            mcol = 0;
        GetMax( mrow, mcol );

        switch( a_dir )
        {
            case TextDirection::Right:
                ++col;
                break;
            case TextDirection::Left:
                --col;
                break;
            case TextDirection::Up:
                --row;
                break;
            case TextDirection::Down:
                ++row;
                break;
            case TextDirection::DownRight:
                ++col;
                ++row;
                break;
            case TextDirection::UpRight:
                ++col;
                --row;
                break;
            case TextDirection::DownLeft:
                --col;
                ++row;
                break;
            case TextDirection::UpLeft:
                --col;
                --row;
                break;
            default: // Default to "Right"
                ++col;
                break;
        }

        // In case something went negative
        row += mrow;
        col += mcol;

        // Normalize the values to within the positive limits
        row %= mrow;
        col %= mcol;

        // Refresh all the screens
        refresh();
    }

    void Window::Put( char a_ch, int a_color, int a_dir )
    {
        ::utils::Lock lock( this );
        if( has_colors() )
        {
            attron( COLOR_PAIR( a_color ) );
        }
        mvaddch( row, col, a_ch );
        if( has_colors() )
        {
            attroff( COLOR_PAIR( a_color ) );
        }
        Update( a_dir );
    }

    void Window::PutRND( char a_ch, int a_color, int a_dir )
    {
        ::utils::Lock lock( this );
        int mrow = 0,
            mcol = 0;
        GetMax( mrow, mcol );

        row = rand() % mrow;
        col = rand() % mcol;

        Put( a_ch, a_color, a_dir );
    }

    void Window::PutRND( const char *a_str, int a_color, int a_dir )
    {
        ::utils::Lock lock( this );
        int slen = strlen( a_str );

        if( slen > 0 )
        {
            PutRND( a_str[ 0 ], a_color, a_dir );
            for( int i = 1; i < slen; ++i )
            {
                Put( a_str[ i ], a_color, a_dir );
            }
        }
    }
}
