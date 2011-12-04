/*
 * balancc - load balancer
 *
 * author     Andre Roth <neolynx@gmail.com>
 * copyright  GNU General Public License, see COPYING file
 */

#include "SocketClient.h"

#include <stdio.h> // printf

SocketClient::SocketClient( bool self ) : self(self)
{
  if( sem_init( &host_available, 0, 0 ))
  {
    printf("Could not initialize semaphore\n");
  }
}

SocketClient::~SocketClient( )
{
  sem_destroy( &host_available );
}

void SocketClient::Connected( int client )
{
  if( self )
    Send( "+get\n", 5 );
  else
    Send( "get\n", 4 );
}

void SocketClient::Disconnected( int client, bool error )
{
  //printf( "%d: client disconnected, error=%d\n", client, error );
}

const std::string &SocketClient::GetHost( )
{
  sem_wait( &host_available );
  return host;
}

void SocketClient::HandleMessage( const int client, const SocketHandler::Message &msg )
{
  if( msg.getLine( ).length( ) < 64 )
  {
    host = msg.getLine( );
    up = false;
  }
  else
    printf( "strange response received\n" );
  sem_post( &host_available );
}

