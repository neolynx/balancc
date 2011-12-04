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
    Send( "+get\n", 6 );
  else
    Send( "get\n", 5 );
}

void SocketClient::Disconnected( int client, bool error )
{
  //printf( "%d: client disconnected, error=%d\n", client, error );
  printf( "unlock SocketClient::Disnnected\n" );
  Unlock( );
}

const std::string &SocketClient::GetHost( )
{
  printf( "lock &SocketClient::GetHost\n" );
  Lock( );
  up = false;
  printf( "returning &SocketClient::GetHost\n" );
  return host;
}

void SocketClient::HandleMessage( const int client, const SocketHandler::Message &msg )
{
//  printf( "SocketClient %d: got %d bytes %s\n", client, msg.getLine( ).length( ), msg.getLine( ).c_str( ));
//  if( msg.getLine( ).length( ) < 64 && msg.getLine( ).length( ) > 0 )
//  {
//    host = msg.getLine( );
//  }
//  else
    printf( "strange response received\n" );
  Unlock( );
}

