/*
 * balancc - load balancer
 *
 * author     Andre Roth <neolynx@gmail.com>
 * copyright  GNU General Public License, see COPYING file
 */

#include "BalanccServer.h"

#include <unistd.h> // sleep
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

  server = new BalanccServer( );
  if( !server->CreateServerTCP( PORT ))
  {
    SocketHandler::Log( "unable to create server" );
    delete server;
    return -1;
  }

  if( !SocketHandler::Daemonize( "balancc", "/var/run/balanccd.pid" ))
  {
    SocketHandler::Log( "failed to create daemon" );
    delete server;
    return -1;
  }

  if( !server->Start( ))
  {
    SocketHandler::Log( "unable to start server" );
    delete server;
    return -1;
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

