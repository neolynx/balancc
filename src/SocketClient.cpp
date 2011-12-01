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
  Lock( );
}

SocketClient::~SocketClient( )
{
}

void SocketClient::Connected( int client )
{
  if( self )
    Send( "+get", 5 );
  else
    Send( "get", 4 );
}

void SocketClient::Disconnected( int client, bool error )
{
  printf( "%d: client disconnected, error=%d\n", client, error );
  Unlock( );
}

void SocketClient::DataReceived( int client, const char *buffer, int length )
{
  printf( "%d: got %d bytes\n", client, length );
  if( length < 64 && buffer[length - 1] == '\0' )
    host = buffer;
  else
    printf( "strange response received\n" );
  up = false;
  Unlock( );
}

std::string &SocketClient::GetHost( )
{
  Lock( );
  return host;
}

