/*
 * balancc - load balancer
 *
 * author     Andre Roth <neolynx@gmail.com>
 * copyright  GNU General Public License, see COPYING file
 */

#ifndef _SocketServer_
#define _SocketServer_

#include "SocketHandler.h"

#include <deque>

class BalanccClient;

class SocketServer : public SocketHandler
{
  public:
    SocketServer( BalanccClient &balancclient );
    virtual ~SocketServer( );

    bool Reply( const char *buffer, int length );

  protected:
    virtual void Connected   ( int client );
    virtual void Disconnected( int client, bool error );
    virtual int  DataReceived( int client, const char *buffer, int length );

  private:
    BalanccClient &balancclient;
    std::deque<int> requests;
};

#endif

