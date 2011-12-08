/*
 * balancc - load balancer
 *
 * author     Andre Roth <neolynx@gmail.com>
 * copyright  GNU General Public License, see COPYING file
 */

#include "BalanccServer.h"

#include <unistd.h> // sleep
//#include <stdio.h>  // printf
#include <signal.h>

int PORT = 1977;

BalanccServer *server = NULL;

void sighandler( int signum )
{
  if( server )
    server->Stop();
}

int main( int argc, char *argv[] )
{
  signal( SIGINT, sighandler ); // FIXME: use sigaction
  signal( SIGTERM, sighandler ); // FIXME: use sigaction

  SocketHandler::OpenLog( argv[0] );
  //FIXME: parse arguments

  if( !SocketHandler::daemonize( "balancc" ))
  {
    SocketHandler::Log( "failed to create daemon" );
    return -1;
  }

  server = new BalanccServer( );
  if( !server->StartTCP( PORT ))
  {
    SocketHandler::Log( "unable to start server" );
  }

  int c = 0;
  while( server->isUp( ))
  {
    if( c++ % 10 == 0 )
      server->Polling( );
    sleep( 1 );
  }
  server->Stop();
  delete server;
  SocketHandler::Log( "terminated" );
  return 0;
}

