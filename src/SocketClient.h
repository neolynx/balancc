/*
 * balancc - load balancer
 *
 * author     Andre Roth <neolynx@gmail.com>
 * copyright  GNU General Public License, see COPYING file
 */

#ifndef _SocketClient_
#define _SocketClient_

#include "SocketHandler.h"

#include <string>

class SocketClient : public SocketHandler
{
  public:
    SocketClient( bool self = false );
    virtual ~SocketClient( );

    const std::string &GetHost( );

  protected:
    virtual void Connected   ( int client );
    virtual void Disconnected( int client, bool error );
    virtual void HandleMessage( const int client, const Message &msg );

  private:
    std::string host;
    bool self;
};

#endif

