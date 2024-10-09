#include <ncurses.h>
#include <stdlib.h>
#include <time.h>
#include <string>
#include <unistd.h>

#include <utils/Window.hpp>
#include <utils/Thread.hpp>
#include <utils/Staque.hpp>
#include <utils/File.hpp>
#include <utils/Tokens.hpp>

using namespace utils;

class ThreadCTX : public Lockable
{
    public:
        bool run;
        bool pause;
        //Staque< char > buffer;
};

void *input( void *a_ctx )
{
    char c   = 0;
    ThreadCTX *context = ( ThreadCTX * ) a_ctx;

    while( context->run && ( c = getch() ) )
    {
        //context->buffer.Enqueue( c );
        switch( c )
        {
            case 'p':
            case 'P':
                context->pause = !( context->pause );
                break;
            case 'q':
            case 'Q':
                context->run = false;
                break;
        }
    }
    pthread_exit( nullptr );
}

static const char charset[] = "abcdefghijklmnopqrstuvwxyz";

int main( int argc, char *argv[] )
{
    // Create the window
    Window w;

    // Background thread for handling keyboard input
    ::std::shared_ptr< Thread< ThreadCTX > > input_thread = ::std::make_shared< Thread< ThreadCTX > >( input );
    input_thread->GetContext()->run = true;
    input_thread->Start();

    // List of words to use in the word search
    Staque< ::std::string > word_list;

    // Read input file, if provided
    if( argc >= 2 )
    {
        // Read up to 200 words from the provided file
        // Assume one word per line and skip blank lines
        File input_file( argv[ 1 ] );
        ::std::string s;
        while( TokenType::Line == Tokens::GetLine( input_file, s ) && ( word_list.Length() < 200 ) )
        {
            // Only allow words with no symbols
            for( size_t i = 0; i < s.length(); ++i )
            {
                if( !( Tokens::IsLetter( s[ i ] ) ) )
                {
                    s.clear();
                    break;
                }
            }

            // Skip empty lines
            if( s.length() > 0 )
            {
                Tokens::MakeUpper( s );
                word_list.Enqueue( s );
                s.clear();
            }
        }
    }

    // Only create the word search if there are words provided
    if( word_list.Length() > 0 )
    {
        do
        {
            // Handle the pause case
            if( input_thread->GetContext()->pause )
            {
                usleep( 50000 ); // Wait 50ms
                continue;
            }

            // Choose a random color
            // Colors start at 1 and go to ColorPair::Count
            int color = rand() % ColorPair::Count;
            ++color;

            // Choose a random direction
            // Directions start at 0 and go to TextDirection::Count - 1
            int direction = rand() % TextDirection::Count;

            // Select a random word from the input file
            int i = rand() % word_list.Length();
            ::std::string s;
            if( word_list.GetAt( i, s ) )
            {
                w.PutRND( s.c_str(), color, direction );
            }

            // Delay 10ms to keep CPU usage sane
            usleep( 10000 );
        }
        while( input_thread->GetContext()->run );
    }

    input_thread->GetContext()->run = false;
    input_thread->Join();

    return 0;
}

