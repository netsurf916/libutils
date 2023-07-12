#include <ncurses.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#include <utils/Window.hpp>
#include <utils/Thread.hpp>
#include <utils/Staque.hpp>

using namespace utils;

class ThreadCTX : public Lockable
{
    public:
        bool run;
        Staque< char > buffer;
};

void *input( void *a_ctx )
{
    char c   = 0;
    ThreadCTX *context = ( ThreadCTX * ) a_ctx;

    while( context->run && ( c = getch() ) )
    {
        context->buffer.Enqueue( c );
        // Stop on keypress
        context->run = false;
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

    do
    {
        // Choose a random color
        // Colors start with 1 and go to ColorPair::Count
        int color = rand() % ( ColorPair::Count );
        ++color;
        if( argc > 1)
        {
            for( int i = 1; i < argc; ++i )
            {
                w.PutRND( argv[ i ], color );
            }
        }
        else
        {
            char c = charset[ rand() % ( ( sizeof( charset ) / sizeof( char ) ) - 1 ) ];
            w.PutRND( c, color );
        }
        usleep( 50000 );
    }
    while( input_thread->GetContext()->run );

    input_thread->GetContext()->run = false;
    input_thread->Join();

    return 0;
}

