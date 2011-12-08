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

#include "BalanccClient.h"
#include "SocketClient.h"
#include "SocketServer.h"

BalanccClient *client = NULL;
SocketServer *server = NULL;
SocketClient *socketclient = NULL;

#define BALANCC_SOCK "/var/run/balancc.sock"

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
  int serverport   = 1977;
  char *servername = NULL;
  bool self = false;

  SocketHandler::OpenLog( argv[0] );

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
    else if( strcmp( "-i", argv[nextarg] ) == 0 ) // include self
    {
      self = true;
      nextarg++;
      break;
    }
    else
    {
      break;
    }
  }

  if( servername )
  {
    client = new BalanccClient( );
    if( !client->CreateClientTCP( servername, serverport, true ))
    {
      SocketHandler::Log( "tcp client creation failed" );
      delete client;
      return -1;
    }

    server = new SocketServer( *client );
    if( !server->CreateServerUnix( BALANCC_SOCK ))
    {
      SocketHandler::Log( "socket server creation failed" );
      delete client;
      delete server;
      return -1;
    }

    if( !SocketHandler::daemonize( "balancc" ))
    {
      SocketHandler::Log( "failed to create daemon" );
      delete client;
      delete server;
      return -1;
    }

    if( !client->Start( ))
    {
      SocketHandler::Log( "connect failed" );
      delete client;
      return -1;
    }

    if( !server->Start( ))
    {
      SocketHandler::Log( "socket server failed to start" );
      delete client;
      delete server;
      return -1;
    }

    chmod( BALANCC_SOCK, 0666 );
    client->SetSocketServer( server );

    while( client->isUp( ))
      sleep( 1 );
    client->Stop();

    delete client;
    delete server;
    SocketHandler::Log( "terminated" );
  }
  else // Get host from server
  {
    std::string host;

    socketclient = new SocketClient( self );
    socketclient->CreateClientUnix( BALANCC_SOCK );
    socketclient->Start( );
    if( socketclient->isConnected( ))
    {
      host = socketclient->GetHost( );
    }
    else
      fprintf( stderr, "unable to connect to %s\n", BALANCC_SOCK );

    if( host == "!" || host == "" )
      host = "localhost";
    std::ostringstream tmp;
    tmp << "DISTCC_HOSTS=" << host;
    if( putenv((char *) tmp.str().c_str()) != 0 )
    {
      fprintf( stderr, "Error putting DISTCC_HOSTS in the environment\n" );
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
          fprintf( stderr, "execvp failed: err %s\n", strerror( errno ));
          return -1;
        }
      }
      else if( pid < 0)
      {
        fprintf( stderr, "Failed to fork a process!\n" );
        return -1;
      }

      // parent
      wait( &status );
      socketclient->Stop( );
      delete socketclient;

      return WEXITSTATUS(status);
    }
  } // get host from server
  return 0;
}

