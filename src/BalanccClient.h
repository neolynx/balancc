/*
 * balancc - load balancer
 *
 * author     Andre Roth <neolynx@gmail.com>
 * copyright  GNU General Public License, see COPYING file
 */

#ifndef _BalanccClient_
#define _BalanccClient_

#include "SocketHandler.h"

class SocketServer;

class BalanccClient : public SocketHandler
{
  public:
    BalanccClient( );
    virtual ~BalanccClient( );

    void SetSocketServer( SocketServer *socketserver );
    void SendLoad( );

  protected:
    virtual void Connected   ( int client );
    virtual void Disconnected( int client, bool error );
    virtual void HandleMessage( const int client, const SocketHandler::Message &msg );

  private:
    SocketServer *socketserver;
    char hostname[64];
    int ncpu;
};

#endif

