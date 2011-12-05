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
  snprintf( buf, sizeof( buf ), "done %d\n", client );
  balancclient.Send( buf, strlen( buf ));
  //Lock( );
  //for( std::deque<int>::iterator i = requests.begin( ); i != requests.end( ); i++ )
    //if( *i == client )
    //{
      //requests.erase( i );
      //break;
    //}
  //Unlock( );
}

bool SocketServer::Reply( const char *buffer, int length )
{
  //const char *buffer = msg.getLine( ).c_str( );
  //int length = msg.getLine( ).length( );
  int id;
  char hostname[64];
  int r;
  r = sscanf( buffer, "%s %d", hostname, &id ); // FIXME: overflow
  strncat( hostname, "\n", sizeof( hostname ));
  if( r == 2 )
    return Send( id, hostname, strlen( hostname ));
  return false;
}

void SocketServer::HandleMessage( const int client, const SocketHandler::Message &msg )
{
  if( msg.getLine( ) == "get" )
  {
    char buf[64];
    snprintf( buf, sizeof( buf ), "get %d\n", client );
    balancclient.Send( buf, strlen( buf ));
  }
  else if( msg.getLine( ) == "+get" )
  {
    char buf[64];
    snprintf( buf, sizeof( buf ), "+get %d\n", client );
    balancclient.Send( buf, strlen( buf ));
  }
  else
  {
    printf( "unknown message: %s\n", msg.getLine( ).c_str( ));
  }
}

