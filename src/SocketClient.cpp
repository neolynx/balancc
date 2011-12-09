/*
 * balancc - load balancer
 *
 * author     Andre Roth <neolynx@gmail.com>
 * copyright  GNU General Public License, see COPYING file
 */

#include "SocketClient.h"

#include <errno.h> // errno
#include <time.h>  // clock_gettime

SocketClient::SocketClient( bool self ) : self(self)
{
  if( sem_init( &host_available, 0, 0 ))
  {
    LogError("Could not initialize semaphore");
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
  //Log( "%d: client disconnected, error=%d", client, error );
}

const std::string &SocketClient::GetHost( )
{
  struct timespec ts;
  if( clock_gettime( CLOCK_REALTIME, &ts ) == -1 )
  {
    LogError( "Error getting time" );
    host = "!";
    return host;
  }
  ts.tv_sec++;
  int s;
  while(( s = sem_timedwait( &host_available, &ts )) == -1 && errno == EINTR )
    continue;       /* Restart if interrupted by handler */

  if( s == -1 )
  {
    if( errno == ETIMEDOUT )
      LogWarn( "timeout" );
    else
      LogError( "semaphore error" );
    host = "!";
  }
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
    LogError( "strange response received" );
  sem_post( &host_available );
}

