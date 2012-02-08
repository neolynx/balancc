/*
 * balancc - load balancer
 *
 * author     Andre Roth <neolynx@gmail.com>
 * copyright  GNU General Public License, see COPYING file
 */

#include "SocketHandler.h"

#include <netinet/in.h> // sockaddr_in
#include <linux/un.h>   // sockaddr_un, UNIX_PATH_MAX
#include <netdb.h>      // getaddrinfo
#include <sys/time.h>   // timeval
#include <unistd.h>     // sleep, exit
#include <stdlib.h>     // calloc
#include <string.h>     // memset
#include <stdio.h>      // printf, freopen
#include <sys/stat.h>   // umask
#include <pwd.h>        // getpwnam
#include <syslog.h>     // openlog, vsyslog, closelog
#include <stdarg.h>     // va_start, va_end
#include <fcntl.h>      // creat

bool SocketHandler::log2syslog = false;

SocketHandler::SocketHandler() : up(false), connected(false), autoreconnect(false), sd(0), host(NULL), port(0), socket(0)
{
  pthread_mutex_init( &mutex, 0 );
}

SocketHandler::~SocketHandler()
{
  Stop( );
  closelog( );
  if( pidfile != "" )
    unlink( pidfile.c_str( ));
}

bool SocketHandler::CreateClient( SocketType sockettype, const char *host, int port, const char *socket, bool autoreconnect )
{
  this->host = (char *) host;
  this->port = port;
  this->socket = (char *) socket;
  this->autoreconnect = autoreconnect;
  this->sockettype = sockettype;
  this->role = CLIENT;

  if( sd )
    return false;

  if( !CreateSocket( ))
    return false;

  return true;
}

bool SocketHandler::CreateServer( SocketType sockettype, int port, const char *socket )
{
  if( sd )
    return false;

  this->role = SERVER;
  this->sockettype = sockettype;
  this->port = port;
  this->socket = (char *) socket;

  if( !CreateSocket( ))
  {
    LogError( "socket creation failed" );
    return false;
  }

  int yes = 1;
  if( setsockopt( sd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(  int )) != 0 )
  {
    LogError( "setsockopt failed" );
    goto errorexit;
  }

  if( !Bind( ))
  {
    goto errorexit;
  }

  return true;

errorexit:
  close( sd );
  sd = 0;
  return false;
}

bool SocketHandler::Start( )
{
  switch( role )
  {
    case SERVER:
      return StartServer( );
    case CLIENT:
      return StartClient( );
  }
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
    if( this->socket )
    {
      // FIXME: verify it's a socket
      unlink( this->socket );
    }
  }
}

bool SocketHandler::StartClient( )
{
  if( !sd )
    return false;

  if( Connect( ))
  {
    connected = true;
    Connected( sd );
  }
  else if( !autoreconnect )
  {
    LogError( "Connection refused" );
    goto errorexit;
  }

  up = true;
  if( pthread_create( &handler, NULL, run, (void*) this ) != 0 )
  {
    LogError( "thread creation failed" );
    goto errorexit;
  }
  return true;

errorexit:
  close( sd );
  sd = 0;
  return false;
}

bool SocketHandler::StartServer( )
{
  if( !sd )
    return false;

  int backlog = 15;
  if( listen( sd, backlog ) != 0 )
  {
    LogError( "listen failed" );
    goto errorexit;
  }

  up = true;
  if( pthread_create( &handler, NULL, run, (void *) this ) != 0 )
  {
    LogError( "thread creation failed" );
    goto errorexit;
  }
  return true;

errorexit:
  close( sd );
  sd = 0;
  return false;
}

bool SocketHandler::Bind( )
{
  bool ok = false;
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
        ok = bind( sd, (struct sockaddr*) &addr, sizeof( addr )) == 0;
        if( !ok )
          LogError( "binding socket %s failed", this->socket );
      }
    case TCP:
      {
        struct sockaddr_in addr;
        memset( &addr, 0, sizeof( struct sockaddr_in ));
        addr.sin_family      = AF_INET;
        addr.sin_port        = htons( port );
        addr.sin_addr.s_addr = INADDR_ANY;
        ok = bind( sd, (struct sockaddr*) &addr, sizeof( addr )) == 0;
        if( !ok )
          LogError( "binding port %d failed", port );
      }
  }
  return ok;
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
    LogError( "socket creation failed" );
    return false;
  }
  return true;
}

void *SocketHandler::run( void *ptr )
{
  SocketHandler *sh = (SocketHandler *) ptr;
  sh->Run( );
  pthread_exit( 0 );
  return NULL;
}

void SocketHandler::Run( )
{
  fd_set tmp_fds;
  char buf[2048];
  int readpos = 0;
  int writepos = 0;

  FD_ZERO( &fds );
  fdmax = sd;
  FD_SET ( sd, &fds );

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
          LogError( "Reconnect refused" );
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
      LogError( "select error" );
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
                LogError( "accept error" );
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
                  LogError( "Error receiving data..." );
                DisconnectClient( i, len != 0 );
              }
              else
              {
                writepos += len;
                if( writepos == sizeof( buf ))
                  writepos = 0;
                int already_read;
                while( readpos != writepos && up ) // handle all data
                {
                  if( !messages[i] )
                    messages[i] = CreateMessage( );

                  already_read = messages[i]->AccumulateData( buf + readpos, len );
                  if( already_read > len )
                  {
                    // FIXME: log
                    already_read = len;
                  }

                  if( messages[i]->isSubmitted( ))
                  {
                    HandleMessage( i, *messages[i] );
                    delete messages[i];
                    messages[i] = NULL;
                  }
                  len     -= already_read;
                  readpos += already_read;
                  if( readpos == sizeof( buf ))
                    readpos = 0;
                  //LogError( "writepos: %d, readpos: %d\n", writepos, readpos );
                }
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
              LogError( "Error receiving data..." );
            Disconnected( sd, len != 0 );
            connected = false;
            close( sd );
            sd = 0;
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
              up = false;
            }
          }
          else
          {
            writepos += len;
            if( writepos == sizeof( buf ))
              writepos = 0;
            int already_read;
            while( readpos != writepos && up ) // handle all data
            {
              if( !messages[sd] )
                messages[sd] = CreateMessage( );

              already_read = messages[sd]->AccumulateData( buf + readpos, len );
              if( already_read > len )
              {
                // FIXME: log
                already_read = len;
              }

              if( messages[sd]->isSubmitted( ))
              {
                HandleMessage( sd, *messages[sd] );
                delete messages[sd];
                messages[sd] = NULL;
              }
              len     -= already_read;
              readpos += already_read;
              if( readpos == sizeof( buf ))
                readpos = 0;
              //Log( "writepos: %d, readpos: %d\n", writepos, readpos );
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
    LogError( "error writing to socket" );
    Disconnected( sd, true );
    close( sd );
    sd = 0;
    connected = false;
    return false;
  }
  if( n != len )
  {
    LogError( "short write" );
    return false;
  }
  return true;
}

bool SocketHandler::SendToClient( int client, const char *buffer, int len )
{
  if( role != SERVER )
    return false;

  int n = write( client, buffer, len );
  if( n < 0 )
  {
    LogError( "error writing to socket" );
    DisconnectClient( client );
    return false;
  }
  if( n != len )
  {
    LogError( "short write" );
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

void SocketHandler::Dump( const char *buffer, int length )
{
  for( int i = 0; i < length; i++ )
    printf( "%02x ", buffer[i] );
  printf( "\n" );
}

SocketHandler::Message *SocketHandler::CreateMessage( ) const
{
  return new Message( );
}

int SocketHandler::Message::AccumulateData( const char *buffer, int length )
{
  bool end = false;
  int i;
  //  Log( "accumulating data: " );
  //  Dump( buffer, length );
  for( i = 0; i < length; i++ )
    if( buffer[i] == '\0' ||  buffer[i] == '\n' ||  buffer[i] == '\r' )
      end = true;
    else
    {
      if( end )
        break;
      line += buffer[i];
    }
  if( end )
  {
    Submit( );
  }
  return i;
}

void SocketHandler::DisconnectClient( int client, bool error )
{
  Disconnected( client, error );
  close( client );
  FD_CLR( client, &fds );
}

bool SocketHandler::Daemonize( const char *user, const char *pidfile )
{
  pid_t pid, sid;
  if( getppid() == 1 ) // already daemonized
  {
    LogError( "daemon cannot daemonize" );
    return false;
  }
  pid = fork();
  if( pid < 0 )
  {
    LogError( "fork error" );
    return false;
  }
  if( pid > 0 ) // parent
  {
    if( pidfile ) // FIXME: remove pidfile on exit
    {
      int fd = creat( pidfile, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH );
      if( fd > 0 )
      {
        char buf[16];
        snprintf( buf, sizeof( buf ), "%d\n", pid );
        write( fd, buf, strlen( buf ));
        close( fd );
      }
      else
      {
        LogError( "unable to create PID file: %s", pidfile );
      }
    }
    exit( 0 ); // exit the parent
  }

  // child
  if( pidfile )
    this->pidfile = pidfile;
  if( user )
  {
    struct passwd *pwd = getpwnam( user );
    if( setuid( pwd->pw_uid ) != 0 )
    {
      LogError( "cannot setuid( %d ) for user %s", pwd->pw_uid, user );
      return false;
    }
  }
  umask( 0 );
  sid = setsid( );
  if( sid < 0 )
    return false;
  if(( chdir( "/" )) < 0 )
    return false;
  freopen( "/dev/null", "r", stdin );
  freopen( "/dev/null", "w", stdout );
  freopen( "/dev/null", "w", stderr );
  return true;
}

void SocketHandler::OpenLog( const char *prog )
{
  openlog( prog, LOG_PID | LOG_NDELAY, LOG_DAEMON );
  log2syslog = true;
}

void SocketHandler::Log( const char *fmt, ... )
{
  va_list ap;
  va_start( ap, fmt );
  if( log2syslog )
    vsyslog( LOG_INFO, fmt, ap );
  else
  {
    printf( "LOG    " );
    vprintf( fmt, ap );
    printf( "\n" );
  }
  va_end( ap );
}

void SocketHandler::LogNotice( const char *fmt, ... )
{
  va_list ap;
  va_start( ap, fmt );
  if( log2syslog )
    vsyslog( LOG_NOTICE, fmt, ap );
  else
  {
    printf( "NOTICE " );
    vprintf( fmt, ap );
    printf( "\n" );
  }
  va_end( ap );
}

void SocketHandler::LogWarn( const char *fmt, ... )
{
  va_list ap;
  va_start( ap, fmt );
  if( log2syslog )
    vsyslog( LOG_WARNING, fmt, ap );
  else
  {
    printf( "WARN   " );
    vprintf( fmt, ap );
    printf( "\n" );
  }
  va_end( ap );
}

void SocketHandler::LogError( const char *fmt, ... )
{
  va_list ap;
  va_start( ap, fmt );
  if( log2syslog )
    vsyslog( LOG_ERR, fmt, ap );
  else
  {
    fprintf( stderr, "ERROR  " );
    vfprintf( stderr, fmt, ap );
    fprintf( stderr, "\n" );
  }
  va_end( ap );
}

