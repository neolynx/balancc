/*
 * balancc - load balancer
 *
 * author     Andre Roth <neolynx@gmail.com>
 * copyright  GNU General Public License, see COPYING file
 */

#include "SocketHandler.h"

#include <netinet/in.h> // sockaddr_in
#include <linux/un.h>   // sockaddr_un, UNIX_PATH_MAX
#include <sys/socket.h> // fd_set
#include <netdb.h>      // getaddrinfo
#include <sys/time.h>   // timeval
#include <unistd.h>     // sleep
#include <stdlib.h>     // calloc
#include <string.h>     // memset
#include <stdio.h>      // printf

SocketHandler::SocketHandler() : up(false), connected(false), autoreconnect(false), sd(0)
{
  pthread_mutex_init( &mutex, 0 );
}

SocketHandler::~SocketHandler()
{
  Stop( );
}

bool SocketHandler::ConnectTCP( const char *host, int port, bool autoreconnect )
{
  this->host = (char *) host;
  this->port = port;
  this->autoreconnect = autoreconnect;
  this->sockettype = TCP;
  this->role = CLIENT;
  return StartClient( );
}

bool SocketHandler::ConnectUnix( const char *socket, bool autoreconnect )
{
  this->socket = (char *) socket;
  this->autoreconnect = autoreconnect;
  this->sockettype = UNIX;
  this->role = CLIENT;
  return StartClient( );
}

bool SocketHandler::Bind( )
{
  switch( sockettype )
  {
    case UNIX:
      {
        struct sockaddr_un addr;
        memset( &addr, 0, sizeof( struct sockaddr_un ));
        addr.sun_family = AF_UNIX;
        strncat( addr.sun_path, this->socket, UNIX_PATH_MAX - 1 );
        // FIXME: verify it's a socket
        unlink( this->socket );
        return ( bind( sd, (struct sockaddr*) &addr, sizeof( addr )) == 0 );
      }
    case TCP:
      {
        struct sockaddr_in addr;
        memset( &addr, 0, sizeof( struct sockaddr_in ));
        addr.sin_family      = AF_INET;
        addr.sin_port        = htons( port );
        addr.sin_addr.s_addr = INADDR_ANY;
        return ( bind( sd, (struct sockaddr*) &addr, sizeof( addr )) == 0 );
      }
  }
}

bool SocketHandler::Connect( )
{
  switch( sockettype )
  {
    case TCP:
      {
        struct sockaddr_in addr;
        struct addrinfo hints;
        struct addrinfo *result;
        memset( &addr,  0, sizeof( struct sockaddr_in ));
        memset( &hints, 0, sizeof( struct addrinfo ));
        hints.ai_family = PF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_flags |= AI_CANONNAME;

        int s = getaddrinfo( host, NULL, &hints, &result );
        if( s != 0 )
        {
          return false;
        }

        while( result )
        {
          if( result->ai_family == AF_INET )
            break;
          result = result->ai_next;
        }
        addr.sin_addr.s_addr = ((struct sockaddr_in *) result->ai_addr )->sin_addr.s_addr;
        addr.sin_family      = AF_INET;
        addr.sin_port        = htons( port );

        freeaddrinfo( result );

        return ( connect( sd, (struct sockaddr*) &addr, sizeof( addr )) == 0 );
      }
    case UNIX:
      {
        struct sockaddr_un addr;
        memset( &addr, 0, sizeof( struct sockaddr_un ));
        addr.sun_family = AF_UNIX;
        strncat( addr.sun_path, this->socket, UNIX_PATH_MAX - 1 );
        // FIXME: verify it's a socket
        //unlink( this->socket );
        return ( connect( sd, (struct sockaddr*) &addr, sizeof( addr )) == 0 );
      }
  }
}

bool SocketHandler::CreateSocket( )
{
  switch( this->sockettype )
  {
    case UNIX:
      sd = ::socket( PF_UNIX, SOCK_STREAM, 0 );
      break;
    case TCP:
      sd = ::socket( PF_INET, SOCK_STREAM, 0 );
      break;
  }
  if( sd < 0 )
  {
    sd = 0;
    printf( "socket creation failed\n" );
    return false;
  }
  return true;
}

bool SocketHandler::StartServer( SocketType sockettype, int port, const char *socket )
{
  if( sd )
    return false;

  int backlog = 15;
  this->role = SERVER;
  this->sockettype = sockettype;
  this->port = port;
  this->socket = (char *) socket;

  if( !CreateSocket( ))
  {
    printf( "socket creation failed\n" );
    return false;
  }

  int yes = 1;
  if( setsockopt( sd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(  int )) != 0 )
  {
    printf( "setsockopt failed\n" );
    goto errorexit;
  }

  if( !Bind( ))
  {
    printf( "bind failed\n" );
    goto errorexit;
  }

  if( listen( sd, backlog ) != 0 )
  {
    printf( "listen failed\n" );
    goto errorexit;
  }

  up = true;
  if( pthread_create( &handler, NULL, run, (void *) this ) != 0 )
  {
    printf( "thread creation failed\n" );
    goto errorexit;
  }
  return true;

errorexit:
  close( sd );
  sd = 0;
  return false;
}

bool SocketHandler::StartClient( )
{
  if( sd )
    return false;

  if( !CreateSocket( ))
    return false;

  if( Connect( ))
  {
    connected = true;
    Connected( sd );
  }
  else if( !autoreconnect )
  {
    printf( "Connection refused\n" );
    goto errorexit;
  }

  up = true;
  if( pthread_create( &handler, NULL, run, (void*) this ) != 0 )
  {
    printf( "thread creation failed\n" );
    goto errorexit;
  }
  return true;

errorexit:
  close( sd );
  sd = 0;
  return false;
}

void SocketHandler::Stop( )
{
  if( up )
  {
    up = false;
    pthread_join( handler, NULL);
  }
  if( sd )
  {
    close( sd );
    sd = 0;
  }
}

void *SocketHandler::run( void *ptr )
{
  SocketHandler *sh = (SocketHandler *) ptr;
  sh->Run( );
  // FIXME: pthread destroy
  return NULL;
}

void SocketHandler::Run( )
{
  int fdmax;
  fd_set fds;
  fd_set tmp_fds;
  char buf[2048];
  int readpos = 0;
  int writepos = 0;

  FD_ZERO( &fds );
  //if( role == SERVER )
  //{
  fdmax = sd;
  FD_SET ( sd, &fds );
  //}

  while( up )
  {
    if( role == CLIENT && !connected )
    {
      if( !Connect( ))
      {
        if( autoreconnect )
        {
          sleep( 1 );
          continue;
        }
        else
        {
          printf( "Connection refused\n" );
          up = false;
        }
      }
      else
      {
        connected = true;
        fdmax = sd;
        FD_SET ( sd, &fds );
        Connected( sd );
      }
    }

    tmp_fds = fds;

    struct timeval timeout = { 1, 0 }; // 1 sec
    if( select( fdmax + 1, &tmp_fds, NULL, NULL, &timeout ) == -1 )
    {
      printf( "select error\n" );
      up = false;
      continue;
    }

    switch( role )
    {
      case SERVER:
        for( int i = 0; i <= fdmax; i++ )
        {
          if( FD_ISSET( i, &tmp_fds ))
          {
            if( i == sd ) // new connection
            {
              struct sockaddr_in clientaddr;
              socklen_t addrlen = sizeof(clientaddr);
              int newfd;
              if(( newfd = accept( sd, (struct sockaddr *) &clientaddr, &addrlen )) == -1 )
              {
                printf( "accept error\n" );
                continue;
              }

              FD_SET( newfd, &fds );
              if( newfd > fdmax )
                fdmax = newfd;

              Connected( newfd );
            }
            else // handle data
            {
              int len = recv( i, buf + writepos, sizeof( buf ) - writepos, 0 );
              if( len <= 0 )
              {
                if( len != 0 )
                  printf( "Error receiving data...\n" );
                Disconnected( i, len != 0 );
                close( i );
                FD_CLR( i, &fds );
              }
              else
              {
                writepos += len;
                if( writepos == sizeof( buf ))
                  writepos = 0;
                while( readpos != writepos && up ) // handle all data
                {
                  readpos += DataReceived( i, buf + readpos, len );
                  if( readpos == sizeof( buf ))
                    readpos = 0;
                }
                printf( "writepos: %d, readpos: %d\n", writepos, readpos );
              }
            }
          } // if( FD_ISSET( i, &tmp_fds ))
        } // for( int i = 0; i <= fdmax; i++ )
        break;

      case CLIENT:
        if( FD_ISSET( sd, &tmp_fds ))
        {
          int len = recv( sd, buf + writepos, sizeof( buf ) - writepos, 0 );
          if( len <= 0 )
          {
            if( len != 0 )
              printf( "Error receiving data...\n" );
            Disconnected( sd, len != 0 );
            connected = false;
            close( sd );
            if( autoreconnect )
            {
              if( !CreateSocket( ))
                up = false;
              {
                fdmax = sd;
                FD_SET ( sd, &fds );
              }
            }
            else
            {
              sd = 0;
              up = false;
            }
          }
          else
          {
            writepos += len;
            if( writepos == sizeof( buf ))
              writepos = 0;
            while( readpos != writepos && up ) // handle all data
            {
              readpos += DataReceived( sd, buf + readpos, len );
              if( readpos == sizeof( buf ))
                readpos = 0;
            }
          }
        }
        break;
    } // switch( type )
  } // while( up )
}

bool SocketHandler::Send( const char *buffer, int len )
{
  if( role != CLIENT || !connected )
    return false;

  int n = write( sd, buffer, len );
  if( n < 0 )
  {
    printf( "error writing to socket\n" );
    up = false;
    return false;
  }
  if( n != len )
  {
    printf( "short write\n" );
    return false;
  }
  return true;
}

bool SocketHandler::Send( int client, const char *buffer, int len )
{
  if( role != SERVER )
    return false;

  int n = write( client, buffer, len );
  if( n < 0 )
  {
    printf( "error writing to socket\n" );
    up = false;
    return false;
  }
  if( n != len )
  {
    printf( "short write\n" );
    return false;
  }
  return true;
}

bool SocketHandler::Lock( )
{
  pthread_mutex_lock( &mutex );
}

bool SocketHandler::Unlock( )
{
  pthread_mutex_unlock( &mutex );
}

void SocketHandler::Dump( const char *buffer, int length ) const
{
  for( int i = 0; i < length; i++ )
    printf( "%02x ", buffer[i] );
  printf( "\n" );
}

