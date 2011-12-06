/*
 * balancc - load balancer
 *
 * author     Andre Roth <neolynx@gmail.com>
 * copyright  GNU General Public License, see COPYING file
 */

#ifndef _Slot_
#define _Slot_

#include <stdint.h>
#include <map>

union Slot
{
  struct
  {
    uint16_t client;
    uint16_t id;
  };
  uint32_t key;
  Slot( int client, int id ) : client(client), id(id) { }
};

struct ltslot
{
  bool operator()(const Slot s1, const Slot s2) const
  {
    return ( s1.key < s2.key );
  }
};

class Host;

typedef std::map<Slot, Host *, ltslot> map_Slot;
typedef std::map<Slot, Host *>::iterator iterator_Slot;

#endif

