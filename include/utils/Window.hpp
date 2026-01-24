/**
    Window.hpp : Window helper class for ncurses
    Description: Curses window wrapper.
    Copyright 2023-2026 Daniel Wilson
    SPDX-License-Identifier: MIT
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
        /**
         * @brief Predefined ncurses color pair identifiers.
         */
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
        /**
         * @brief Direction values for text placement updates.
         */
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

    /**
     * @brief Minimal ncurses window wrapper.
     * @details Provides helpers to write characters/strings while tracking the
     *          cursor position. Requires ncurses initialization outside.
     * @note Not safe for concurrent access without external synchronization.
     */
    class Window : public Lockable
    {
        private:
            int row, col;
            /**
             * @brief Update cursor position based on direction.
             * @param a_dir Direction to move after writing.
             */
            void Update( int a_dir = TextDirection::Right );

        public:
            /**
             * @brief Construct a window wrapper.
             */
            Window();
            /**
             * @brief Destroy the window wrapper.
             */
            ~Window();

            /**
             * @brief Get maximum window bounds.
             * @param a_row Output row count.
             * @param a_col Output column count.
             */
            void GetMax( int &a_row, int &a_col );
            /**
             * @brief Put a character at the current position.
             * @param a_ch Character to write.
             * @param a_color Color pair to apply.
             * @param a_dir Direction to move after writing.
             */
            void Put( char a_ch, int a_color = ColorPair::WhiteOnBlack, int a_dir = TextDirection::Right );
            /**
             * @brief Put a character at a random position.
             * @param a_ch Character to write.
             * @param a_color Color pair to apply.
             * @param a_dir Direction to move after writing.
             */
            void PutRND( char a_ch, int a_color = ColorPair::WhiteOnBlack, int a_dir = TextDirection::Right );
            /**
             * @brief Put a string at a random position.
             * @param a_str Null-terminated string to write; must be non-null.
             * @param a_color Color pair to apply.
             * @param a_dir Direction to move after writing.
             */
            void PutRND( const char *a_str, int a_color = ColorPair::WhiteOnBlack, int a_dir = TextDirection::Right );
    };
}

#endif // _WINDOW_HPP_
