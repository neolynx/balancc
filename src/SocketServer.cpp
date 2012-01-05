/*
 * balancc - load balancer
 *
 * author     Andre Roth <neolynx@gmail.com>
 * copyright  GNU General Public License, see COPYING file
 */

#include "SocketServer.h"

#include <stdio.h>   // sscanf
#include <string.h>  // strncmp
#include <algorithm> // find

#include "BalanccClient.h"

SocketServer::SocketServer( BalanccClient &balancclient ) : balancclient(balancclient)
{
}

SocketServer::~SocketServer( )
{
}

void SocketServer::Connected( int client )
{
  //Log( "%d: got new client\n", client );
}

void SocketServer::Disconnected( int client, bool error )
{
  //  Log( "%d: client disconnected, error=%d\n", client, error );
  if( balancclient.isConnected( ))
  {
    // if host was not assigned, do not free
    std::vector<int>::iterator i;
    if(( i = std::find( failed.begin( ), failed.end( ), client )) != failed.end( ))
      failed.erase( i );
    else
    {
      char buf[64];
      snprintf( buf, sizeof( buf ), "free %d\n", client );
      balancclient.Send( buf, strlen( buf ));
    }
  }
}

bool SocketServer::Reply( const char *buffer, int length )
{
  int id;
  char hostname[64];
  int r = sscanf( buffer, "%s %d", hostname, &id ); // FIXME: overflow
  if( r == 2 )
  {
    if( hostname[0] == '!' )
      failed.push_back( id ); // remember to not free unassigned host
    strncat( hostname, "\n", sizeof( hostname ));
    return SendToClient( id, hostname, strlen( hostname ));
  }
  // FIXME: else Log
  return false;
}

void SocketServer::HandleMessage( const int client, const SocketHandler::Message &msg )
{
  char buf[64];
  if( msg.getLine( ) == "get" )
  {
    if( balancclient.isConnected( ))
    {
      snprintf( buf, sizeof( buf ), "get %d\n", client );
      balancclient.Send( buf, strlen( buf ));
    }
    else
    {
      snprintf( buf, sizeof( buf ), "! %d\n", client );
      Reply( buf, strlen( buf ));
    }
  }
  else if( msg.getLine( ) == "-get" )
  {
    if( balancclient.isConnected( ))
    {
      snprintf( buf, sizeof( buf ), "-get %d\n", client );
      balancclient.Send( buf, strlen( buf ));
    }
    else
    {
      snprintf( buf, sizeof( buf ), "! %d\n", client );
      Reply( buf, strlen( buf ));
    }
  }
  else
  {
    LogError( "unknown message: %s", msg.getLine( ).c_str( ));
  }
}

