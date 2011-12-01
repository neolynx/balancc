/*
 * balancc - load balancer
 *
 * author     Andre Roth <neolynx@gmail.com>
 * copyright  GNU General Public License, see COPYING file
 */

#include "SocketServer.h"

#include <stdio.h> // printf
#include <string.h> // strncmp

#include "BalanccClient.h"

SocketServer::SocketServer( BalanccClient &balancclient ) : balancclient(balancclient), current(-1)
{
}

SocketServer::~SocketServer( )
{
}

void SocketServer::Connected( int client )
{
  //printf( "%d: got new client\n", client );
}

void SocketServer::Disconnected( int client, bool error )
{
  //  printf( "%d: client disconnected, error=%d\n", client, error );
  char buf[64];
  snprintf( buf, sizeof( buf ), "done %d", client );
  balancclient.Send( buf, strlen( buf ) + 1 ); // include \0
  Unlock( );
}

void SocketServer::DataReceived( int client, const char *buffer, int length )
{
  if( strncmp( buffer, "get", 4 ) == 0 )
  {
    Lock( );
    current = client;
    char buf[64];
    snprintf( buf, sizeof( buf ), "get %d", client );
    balancclient.Send( buf, strlen( buf ) + 1 ); // include \0
    return;
  }
  if( strncmp( buffer, "+get", 5 ) == 0 )
  {
    Lock( );
    current = client;
    char buf[64];
    snprintf( buf, sizeof( buf ), "+get %d", client );
    balancclient.Send( buf, strlen( buf ) + 1 ); // include \0
  }
}

bool SocketServer::Reply( const char *buffer, int length )
{
  bool ret = false;
  if( current > 0 )
  {
    ret = Send( current, buffer, length );
    current = 0;
    Unlock( );
  }
  return ret;
}

