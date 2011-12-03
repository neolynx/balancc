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

SocketServer::SocketServer( BalanccClient &balancclient ) : balancclient(balancclient)
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
  Lock( );
  for( std::deque<int>::iterator i = requests.begin( ); i != requests.end( ); i++ )
    if( *i == client )
    {
      requests.erase( i );
      break;
    }
  Unlock( );
}

int SocketServer::DataReceived( int client, const char *buffer, int length )
{
  int len = 0;
  if( strncmp( buffer, "get", 4 ) == 0 )
  {
    len = 4;
    Lock( );
    requests.push_back( client );
    Unlock( );
    char buf[64];
    snprintf( buf, sizeof( buf ), "get %d", client );
    balancclient.Send( buf, strlen( buf ) + 1 ); // include \0
  }
  else if( strncmp( buffer, "+get", 5 ) == 0 )
  {
    len = 5;
    Lock( );
    requests.push_back( client );
    Unlock( );
    char buf[64];
    snprintf( buf, sizeof( buf ), "+get %d", client );
    balancclient.Send( buf, strlen( buf ) + 1 ); // include \0
  }
  else
  {
    printf( "unknown message: %s\n", buffer );
  }
  return len;
}

bool SocketServer::Reply( const char *buffer, int length )
{
  bool ret = false;
  //printf( "got reply\n" );
  Lock( );
  if( !requests.empty( ))
  {
    ret = Send( requests[0], buffer, length );
    requests.pop_front( );
  }
  Unlock( );
  return ret;
}

