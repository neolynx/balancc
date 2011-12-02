/*
 * balancc - load balancer
 *
 * author     Andre Roth <neolynx@gmail.com>
 * copyright  GNU General Public License, see COPYING file
 */

#ifndef _Host_
#define _Host_

#include <string>

#include "Slot.h"

#define SLOTS_PER_CPU 2

class Host
{
  private:
    std::string name;
    int cpus;
    float load;
    int usage;

//    vector_Slot slots;

  public:
    Host( const char *name, int cpus );
    ~Host( );
    void SetLoad( float load );
    float GetLoad( ) { return load; }
    const std::string &GetName( ) { return name; }

    bool Assign( );
    bool Release( );
    bool IsFree( );
};

#endif

