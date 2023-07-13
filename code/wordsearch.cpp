#include <ncurses.h>
#include <stdlib.h>
#include <time.h>
#include <string>

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
    Window w;

    ::std::shared_ptr< Thread< ThreadCTX > > input_thread = ::std::make_shared< Thread< ThreadCTX > >( input );
    input_thread->GetContext()->run = true;
    input_thread->Start();

    Staque< ::std::string > word_list;

    if( argc >= 2 )
    {
        // Read up to 200 words from the provided file
        // Assume one word per line and skip blank lines
        File input_file( argv[ 1 ] );
        ::std::string s;
        while( TokenType::Line == Tokens::GetLine( input_file, s ) && ( word_list.Length() < 200 ) )
        {
            // Skip empty lines
            if( s.length() > 0 )
            {
                word_list.Enqueue( s );
                s.clear();
            }
        }
    }

    if( word_list.Length() > 0 )
    {
        // Set color
        int color = ColorPair::RedOnBlack;
        do
        {
            // Handle the pause case
            if( input_thread->GetContext()->pause )
            {
                usleep( 50000 ); // Wait 50ms
                continue;
            }

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

            // Delay 5ms to keep CPU usage sane
            usleep( 5000 );
        }
        while( input_thread->GetContext()->run );
    }

    input_thread->GetContext()->run = false;
    input_thread->Join();

    return 0;
}

