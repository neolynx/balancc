/*
 * balancc - load balancer
 *
 * author     Andre Roth <neolynx@gmail.com>
 * copyright  GNU General Public License, see COPYING file
 */

#ifndef _BalanccServer_
#define _BalanccServer_

#include "SocketHandler.h"
#include "Slot.h"

#include <map>
#include <string>

class Host;

class BalanccServer : public SocketHandler
{
  private:
    std::map<int, Host *> hosts;
    typedef std::map<int, Host *>::iterator iterator_hosts;
    map_Slot assignment;

    std::string GetHost( Slot slot, bool self = false );

  public:
    virtual ~BalanccServer( );

    void Housekeeping( );

  protected:
    // Callbacks
    virtual void Connected   ( int client );
    virtual void Disconnected( int client, bool error );
    virtual void HandleMessage( const int client, const SocketHandler::Message &msg );
};

#endif

