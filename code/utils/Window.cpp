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
        srand( time( NULL ) );
    }

    Window::~Window()
    {
        ::utils::Lock lock( this );
        endwin();
    }

    void Window::Update()
    {
        ::utils::Lock lock( this );
        int mrow = 0,
            mcol = 0;
        getmaxyx( stdscr, mrow, mcol );

        row %= mrow;
        col %= mcol;

        refresh();
    }

    void Window::Put( char a )
    {
        ::utils::Lock lock( this );
        move( row, col );
        addch( a );
        col++;
        Update();
    }

    void Window::PutRND( char a )
    {
        ::utils::Lock lock( this );
        int maxr = 0,
            maxc = 0;
        getmaxyx( stdscr, maxr, maxc );

        row = rand() % maxr;
        col = rand() % maxc;

        Put( a );
    }

    void Window::PutRND( const char *a )
    {
        ::utils::Lock lock( this );
        int slen = strlen( a ),
            maxr = 0,
            maxc = 0;
        getmaxyx( stdscr, maxr, maxc );
        if( ( slen > maxr ) || ( slen > maxc ) ) return;

        row = rand() % maxr;
        col = rand() % ( maxc - slen );

        for( int i = 0; i < slen; ++i )
        {
            Put( a[ i ] );
        }
    }
}