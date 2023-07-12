/**
    Window.cpp : Window helper class implementation for ncurses
    Copyright 2023 Daniel Wilson
*/

#include <utils/Window.hpp>
#include <utils/Lock.hpp>
#include <ncurses.h>
#include <string.h>
#include <time.h>
#include <math.h>

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

    void Window::Update()
    {
        ::utils::Lock lock( this );
        int mrow = 0,
            mcol = 0;
        GetMax( mrow, mcol );

        row %= mrow;
        col %= mcol;

        refresh();
    }

    void Window::Put( char a_ch, int a_color )
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
        ++col;
        Update();
    }

    void Window::PutRND( char a_ch, int a_color )
    {
        ::utils::Lock lock( this );
        int mrow = 0,
            mcol = 0;
        GetMax( mrow, mcol );

        row = rand() % mrow;
        col = rand() % mcol;

        Put( a_ch, a_color );
    }

    void Window::PutRND( const char *a_str, int a_color )
    {
        ::utils::Lock lock( this );
        int slen = strlen( a_str ),
            mrow = 0,
            mcol = 0;
        GetMax( mrow, mcol );

        if( slen > mcol ) slen = mcol;
        row = rand() % mrow;
        if( slen == mcol )
        {
            col = 0;
        }
        else
        {
            col = rand() % ( mcol - slen );
        }

        for( int i = 0; i < slen; ++i )
        {
            Put( a_str[ i ], a_color );
        }
    }
}