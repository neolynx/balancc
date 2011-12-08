/*
 * balancc - load balancer
 *
 * author     Andre Roth <neolynx@gmail.com>
 * copyright  GNU General Public License, see COPYING file
 */

#ifndef _SocketHandler_
#define _SocketHandler_

#include <pthread.h>
#include <string>

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

    bool CreateServer( SocketType sockettype, int port, const char *socket );
    bool CreateClient( SocketType sockettype, const char *host, int port, const char *socket, bool autoreconnect );
    bool StartServer( );
    bool StartClient( );

    bool CreateSocket( );
    bool Bind( );
    bool Connect( );

    static void *run( void *ptr );

  public:
    virtual ~SocketHandler( );

    bool CreateServerTCP ( int port )           { return CreateServer( TCP, port, NULL ); }
    bool CreateServerUnix( const char *socket ) { return CreateServer( UNIX, 0, socket ); }
    bool CreateClientTCP ( const char *host, int port, bool autoreconnect = false ) { return CreateClient( TCP, host, port, NULL, autoreconnect ); }
    bool CreateClientUnix( const char *socket,         bool autoreconnect = false ) { return CreateClient( UNIX, NULL, 0, socket, autoreconnect ); }
    bool Start( );
    void Stop ( );

    virtual bool Send( const char *buffer, int len );
    virtual bool Send( int client, const char *buffer, int len );

    bool isUp( ) { return up; }
    bool isConnected( ) { return connected; }

    static bool daemonize( const char *user = NULL );
    static void OpenLog( const char *prog );
    static void Log( const char *fmt, ... ) __attribute__ (( format( printf, 1, 2 )));

  protected:
    bool up;

    class Message
    {
      private:
        bool submitted;
        std::string line;
      public:
        Message( ) { line = ""; submitted = false; }
        virtual ~Message( ) { };
        virtual int AccumulateData( const char *buffer, int length );
        void Submit( ) { submitted = true; };
        bool isSubmitted( ) { return submitted; }
        const std::string getLine( ) const { return line; }
    };

    SocketHandler( );
    void Run( );
    bool Lock( );
    bool Unlock( );

    static void Dump( const char *buffer, int length );

    virtual Message *CreateMessage( ) const;

    // Callbacks
    virtual void Connected( int client ) = 0;
    virtual void Disconnected( int client, bool error ) = 0;
    virtual void HandleMessage( const int client, const Message &msg ) = 0;

  private:
    Message *message;
};

#endif

