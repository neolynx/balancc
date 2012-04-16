/*
 * balancc - load balancer
 *
 * author     Andre Roth <neolynx@gmail.com>
 * copyright  GNU General Public License, see COPYING file
 */

#include "BalanccServer.h"

#include <unistd.h> // sleep
#include <signal.h>
#include <stdio.h>  // printf
#include <stdlib.h> // exit

int PORT = 1977;

BalanccServer *server = NULL;

void sighandler( int signum )
{
  if( server )
    server->Stop();
}

void usage( char *prog )
{
  printf( "Usage: %s [-d]\n", prog );
  printf( "  -d     debug, do not fork, log to console\n" );
  exit( -1 );
}

int main( int argc, char *argv[] )
{
  signal( SIGINT, sighandler ); // FIXME: use sigaction
  signal( SIGTERM, sighandler ); // FIXME: use sigaction

  bool debug = false;

  int opt;
  while(( opt = getopt( argc, argv, "d" )) != -1 )
  {
    switch( opt )
    {
      case 'd':
        debug = true;
        break;
      default:
        usage( argv[0] );
    }
  }

  if( !debug )
    SocketHandler::OpenLog( argv[0] );

  SocketHandler::Log( "balanccd started" );

  server = new BalanccServer( );
  if( !server->CreateServerTCP( PORT ))
  {
    SocketHandler::LogError( "unable to create server" );
    delete server;
    return -1;
  }

  if( !debug )
    if( !server->Daemonize( "balancc", "/var/run/balanccd.pid" ))
    {
      SocketHandler::LogError( "failed to create daemon" );
      delete server;
      return -1;
    }

  if( !server->Start( ))
  {
    SocketHandler::LogError( "unable to start server" );
    delete server;
    return -1;
  }

  int c = 0;
  while( server->isUp( ))
  {
    if( c++ % 5 == 0 )
      server->Housekeeping( );
    sleep( 1 );
  }
  server->Stop();
  delete server;
  SocketHandler::Log( "balanccd terminated" );
  return 0;
}

