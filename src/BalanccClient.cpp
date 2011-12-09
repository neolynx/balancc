/*
 * balancc - load balancer
 *
 * author     Andre Roth <neolynx@gmail.com>
 * copyright  GNU General Public License, see COPYING file
 */

#include "BalanccClient.h"

#include <stdio.h>  // fopen, fgets, fclose
#include <string.h> // strlen
#include <stdlib.h> // getloadavg
#include <unistd.h> // gethostname

#include "SocketServer.h"

#define LINE_MAX 255

BalanccClient::BalanccClient( )
{
  socketserver = NULL;
  if( gethostname( hostname, sizeof( hostname )) < 0 )
  {
    LogError( "error getting hostname" );
  }

  FILE *cpuinfo = fopen( "/proc/cpuinfo", "r" );
  if (cpuinfo)
  {
    char line[LINE_MAX];
    char match[] = "processor";
    while( fgets( line, sizeof( line ), cpuinfo ) != NULL )
    {
      if( strncmp( match, line, sizeof( match ) - 1 ) == 0 )
        ncpu++;
    }
    fclose( cpuinfo );
  }
  else
    ncpu = 1;
}

BalanccClient::~BalanccClient( )
{
}

void BalanccClient::SetSocketServer( SocketServer *socketserver )
{
  this->socketserver = socketserver;
}

void BalanccClient::Connected( int client )
{
  char buf[128];
  snprintf( buf, sizeof( buf ), "host %s %d\n", hostname, ncpu );
  if( !Send( buf, strlen( buf )))
  {
    LogError( "error sending hostname" );
  }
  SendLoad( );
}

void BalanccClient::Disconnected( int client, bool error )
{
  //  Log( "%d: client disconnected, error=%d", client, error );
}

void BalanccClient::SendLoad( )
{
  double load[3];
  if( getloadavg(load, 3) == -1 )
  {
    LogError( "cannot get loadavg" );
  }

  char buf[32];
  snprintf( buf, sizeof( buf ), "load %f\n", load[0] );
  if( !Send( buf, strlen( buf )))
  {
    LogError( "error sending load avg" );
  }
}

void BalanccClient::HandleMessage( const int client, const SocketHandler::Message &msg )
{
  if( msg.getLine( ) == "?" )
    SendLoad( );
  else if( socketserver )
  {
    std::string message = msg.getLine( );
    message += "\n";
    socketserver->Reply( message.c_str( ), message.length( ));
  }
}

