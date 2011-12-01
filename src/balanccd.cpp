/*
 * balancc - load balancer
 *
 * author     Andre Roth <neolynx@gmail.com>
 * copyright  GNU General Public License, see COPYING file
 */

#include "BalanccServer.h"

#include <unistd.h> // sleep
#include <stdio.h> // printf
#include <signal.h> // printf

int PORT = 1977;

BalanccServer *server = NULL;

void sighandler( int signum )
{
  if( server )
    server->Stop();
}

int main()
{
  signal( SIGINT, sighandler );

  server = new BalanccServer( );

  if( !server->StartTCP( PORT ))
  {
    printf( "unable to start server\n" );
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
  return 0;
}

