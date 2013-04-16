/*
 * balancc - load balancer
 *
 * author     Andre Roth <neolynx@gmail.com>
 * copyright  GNU General Public License, see COPYING file
 */

#include <unistd.h> // sleep
#include <stdio.h>  // printf
#include <signal.h>
#include <string.h> // strncmp, strlen
#include <stdlib.h> // exit
#include <sstream>
#include <errno.h>
#include <sys/wait.h>
#include <sys/stat.h> // chmod
#include <math.h>   // NAN

#include "BalanccClient.h"
#include "SocketClient.h"
#include "SocketServer.h"
#include "Host.h"

BalanccClient *client = NULL;
SocketServer *server = NULL;
SocketClient *socketclient = NULL;

#define BALANCC_SOCK "/var/run/balancc.sock"

#define SLOTS_LOCAL      2

void sighandler( int signum )
{
  if( client )
    client->Stop();
  if( server )
    server->Stop();
  if( socketclient )
    socketclient->Stop();
}

void usage( char *prog )
{
  printf( "Usage: %s [-h] [-s server] [-p port]\n", prog );
  exit( -1 );
}

int main( int argc, char *argv[] )
{
  int   serverport  = 1977;
  char *servername  = NULL;
  bool  excludeself = false;
  float loadlimit   = NAN;
  int   slots       = 0;
  bool  debug       = false;
  bool  info        = false;

  signal( SIGINT, sighandler );
  signal( SIGTERM, sighandler );

  int nextarg = 1;
  for( ; nextarg < argc; nextarg++ )
  {
    if( strcmp( "-s", argv[nextarg] ) == 0 ) // server
    {
      if( ++nextarg >= argc )
      {
        usage( argv[0] );
        return -1;
      }
      servername = argv[nextarg];
    }
    else if( strcmp( "-d", argv[nextarg] ) == 0 ) // debug
    {
      debug = true;
    }
    else if( strcmp( "-p", argv[nextarg] ) == 0 ) // port
    {
      if( ++nextarg >= argc )
      {
        usage( argv[0] );
        return -1;
      }
      serverport = atoi( argv[nextarg] );
    }
    else if( strcmp( "-h", argv[nextarg] ) == 0 ) // help
    {
      usage( argv[0] );
      return 0;
    }
    else if( strcmp( "-e", argv[nextarg] ) == 0 ) // exclude self
    {
      excludeself = true;
      break;
    }
    else if( strcmp( "-i", argv[nextarg] ) == 0 ) // info
    {
      info = true;
      break;
    }
    else if( strcmp( "-l", argv[nextarg] ) == 0 ) // load limit
    {
      if( ++nextarg >= argc )
      {
        usage( argv[0] );
        return -1;
      }
      loadlimit = atof( argv[nextarg] );
      break;
    }
    else if( strcmp( "-n", argv[nextarg] ) == 0 ) // number of slots
    {
      if( ++nextarg >= argc )
      {
        usage( argv[0] );
        return -1;
      }
      slots = atoi( argv[nextarg] );
      break;
    }
    else
    {
      break;
    }
  }

  if( !debug )
    SocketHandler::OpenLog( argv[0] );

  if( servername )
  {
    SocketHandler::Log( "balancc started" );
    client = new BalanccClient( loadlimit, slots );
    if( !client->CreateClientTCP( servername, serverport, true ))
    {
      SocketHandler::LogError( "tcp client creation failed" );
      delete client;
      return -1;
    }

    server = new SocketServer( *client );
    if( !server->CreateServerUnix( BALANCC_SOCK ))
    {
      delete client;
      delete server;
      return -1;
    }
    chmod( BALANCC_SOCK, 0666 );
    client->SetSocketServer( server );

    if( !debug )
      if( !server->Daemonize( "balancc", "/var/run/balancc.pid" ))
      {
        SocketHandler::LogError( "failed to create daemon" );
        delete client;
        delete server;
        return -1;
      }

    if( !client->Start( ))
    {
      SocketHandler::LogError( "connect failed" );
      delete client;
      return -1;
    }

    if( !server->Start( ))
    {
      SocketHandler::LogError( "socket server failed to start" );
      delete client;
      delete server;
      return -1;
    }

    int c = 0;
    while( client->isUp( ))
    {
      if( c++ % HEARTBEAT == 0 )
        client->SendLoad( );
      sleep( 1 );
    }
    client->Stop();

    delete client;
    delete server;
    SocketHandler::Log( "balancc terminated" );
  }
  else // Get info/host from server
  {
    if( slots == 0 )
      slots = SLOTS_LOCAL;

    std::string host;

    socketclient = new SocketClient( );
    socketclient->CreateClientUnix( BALANCC_SOCK );
    socketclient->Start( );
    if( socketclient->isConnected( ))
    {
      if( info )
        socketclient->GetInfo( );
      else
        host = socketclient->GetHost( excludeself );
    }
    else
      SocketHandler::LogError( "unable to connect to %s", BALANCC_SOCK );

    if( !info )
    {
      if( host == "!" || host == "" )
      {
        host = "localhost";
      }
      if( setenv( "BALANCC_HOST", host.c_str( ), 1 ) != 0 )
      {
        SocketHandler::LogError( "Cannot set BALANCC_HOST environment variable" );
      }
      if( host == "localhost" )
      {
        char t[32];
        snprintf( t, sizeof( t ), "/%d", slots );
        host += t;
      }
      if( setenv( "DISTCC_HOSTS", host.c_str( ), 1 ) != 0 )
      {
        SocketHandler::LogError( "Cannot set DISTCC_HOSTS environment variable" );
      }
      int status = 0;
      if( nextarg < argc )
      {
        int pid = fork( );
        if( pid == 0 )
        {
          /* child process */
          if( execvp( argv[nextarg], &argv[nextarg]) < 0 )
          {
            SocketHandler::LogError( "execvp failed: err %s", strerror( errno ));
            return -1;
          }
        }
        else if( pid < 0)
        {
          SocketHandler::LogError( "can not fork" );
          return -1;
        }

        // parent
        wait( &status );
        socketclient->Stop( );
        delete socketclient;

        return WEXITSTATUS(status);
      }
    } // !inifo
    socketclient->Stop( );
    delete socketclient;
  } // get host from server
  return 0;
}

