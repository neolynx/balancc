/*
 * balancc - load balancer
 *
 * author     Andre Roth <neolynx@gmail.com>
 * copyright  GNU General Public License, see COPYING file
 */

#ifndef _SocketHandler_
#define _SocketHandler_

#include <pthread.h>

class SocketHandler
{
  private:
    bool connected;
    bool autoreconnect;
    int sd;

    char *host;
    int port;
    char *socket;

    pthread_t handler;
    pthread_mutex_t mutex;

    enum SocketType
    {
      UNIX,
      TCP
    } sockettype;

    enum Role
    {
      SERVER,
      CLIENT
    } role;

    bool StartServer( SocketType sockettype, int port, const char *sock );
    bool StartClient( );

    bool CreateSocket( );
    bool Bind( );
    bool Connect( );

    static void *run( void *ptr );

  public:
    virtual ~SocketHandler( );

    bool StartTCP   ( int port )     { return StartServer  ( TCP, port, NULL ); }
    bool StartUnix  ( const char *socket ) { return StartServer  ( UNIX, 0, socket ); }
    bool ConnectTCP ( const char *host, int port, bool autoreconnect = false );
    bool ConnectUnix( const char *socket, bool autoreconnect = false);
    void Stop( );

    virtual bool Send( const char *buffer, int len );
    virtual bool Send( int client, const char *buffer, int len );

    bool isUp( ) { return up; }
    bool isConnected( ) { return connected; }

  protected:
    bool up;

    SocketHandler( );
    virtual void Run( );
    bool Lock( );
    bool Unlock( );

    // Callbacks
    virtual void Connected( int client ) = 0;
    virtual void Disconnected( int client, bool error ) = 0;
    virtual void DataReceived( int client, const char *buf, int len ) = 0;
};

#endif

