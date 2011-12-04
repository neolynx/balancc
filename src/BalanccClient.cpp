/*
 * balancc - load balancer
 *
 * author     Andre Roth <neolynx@gmail.com>
 * copyright  GNU General Public License, see COPYING file
 */

#include "BalanccClient.h"

#include <stdio.h> // printf
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
    printf( "error getting hostname\n" );
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
    printf( "error sending hostname\n" );
  }
  SendLoad( );
}

void BalanccClient::Disconnected( int client, bool error )
{
  //  printf( "%d: client disconnected, error=%d\n", client, error );
}

void BalanccClient::SendLoad( )
{
  double load[3];
  if( getloadavg(load, 3) == -1 )
  {
    printf( "cannot get loadavg\n" );
  }

  char buf[32];
  snprintf( buf, sizeof( buf ), "load %f\n", load[0] );
  if( !Send( buf, strlen( buf )))
  {
    printf( "error sending load avg\n" );
  }
}

void BalanccClient::HandleMessage( const int client, const SocketHandler::Message &msg )
{
  if( msg.getLine( ) == "?" )
    SendLoad( );
  else if( socketserver )
  {
    std::string host = msg.getLine( );
    host += "\n";
    socketserver->Reply( host.c_str( ), host.length( ));
  }
}

