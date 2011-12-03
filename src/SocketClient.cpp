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
}

SocketClient::~SocketClient( )
{
}

void SocketClient::Connected( int client )
{
  printf( "lock SocketClient::Connected\n" );
  Lock( );
  if( self )
    Send( "+get", 5 );
  else
    Send( "get", 4 );
}

void SocketClient::Disconnected( int client, bool error )
{
  //printf( "%d: client disconnected, error=%d\n", client, error );
  printf( "unlock SocketClient::Disnnected\n" );
  Unlock( );
}

int SocketClient::DataReceived( int client, const char *buffer, int length )
{
  printf( "SocketClient %d: got %d bytes: %s\n", client, length, buffer );
  if( length < 64 )
    host = buffer;
  else
    printf( "strange response received\n" );
  Unlock( );
  return host.length( ) + 1;
}

const std::string &SocketClient::GetHost( )
{
  printf( "lock &SocketClient::GetHost\n" );
  Lock( );
  up = false;
  return host;
}

