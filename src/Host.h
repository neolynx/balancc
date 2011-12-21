/*
 * balancc - load balancer
 *
 * author     Andre Roth <neolynx@gmail.com>
 * copyright  GNU General Public License, see COPYING file
 */

#ifndef _Host_
#define _Host_

#include <string>
#include <time.h>

#include "Slot.h"

#define SLOTS_PER_CPU    2
#define SLOTS_ADDITIONAL 1

class Host
{
  private:
    std::string name;
    int cpus;
    float load;
    int usage;
    time_t lastupdate;

  public:
    Host( const char *name, int cpus );
    ~Host( );
    void SetLoad( float load );
    float GetLoad( ) { return load; }
    std::string &GetName( ) { return name; }
    time_t LastUpdate( ) { return lastupdate; }

    bool Assign( );
    bool Release( );
    bool IsFree( );
};

#endif

