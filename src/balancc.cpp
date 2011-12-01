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

  signal( SIGINT, sighandler );

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

    if( !client->ConnectTCP( servername, serverport, true ))
    {
      printf( "connect failed\n" );
    }

    server = new SocketServer( *client );
    if( !server->StartUnix( BALANCC_SOCK ))
    {
      printf( "socket server failed to start\n" );
    }

    client->SetSocketServer( server );

    while( client->isUp( ))
      sleep( 1 );
    client->Stop();

    delete client;
  }
  else // Get host from server
  {
    std::string host;

    socketclient = new SocketClient( self );
    socketclient->ConnectUnix( BALANCC_SOCK );
    if( socketclient->isConnected( ))
    {
      printf( "getting host\n" );
      host = socketclient->GetHost( );
      printf( "done\n" );
    }
    else
      printf( "unable to connect to %s\n", BALANCC_SOCK );
    socketclient->Stop( );
    delete socketclient;

    if( host == "" )
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

      /* parent process -- just wait for the child */
      wait( &status );

      return WEXITSTATUS(status);
    }
  } // get host from server
  return 0;
}

