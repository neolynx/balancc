/*
 * balancc - load balancer
 *
 * author     Andre Roth <neolynx@gmail.com>
 * copyright  GNU General Public License, see COPYING file
 */

#include "Host.h"

#include <math.h> // NAN

Host::Host( const char *name, int cpus, float loadlimit, int slots ) :
  name(name),
  cpus(cpus),
  loadlimit(loadlimit),
  slots(slots),
  load(NAN),
  usage(0),
  assigned(0)
{
  if( slots == 0 )
    this->slots = cpus * SLOTS_PER_CPU + SLOTS_ADDITIONAL;
}

Host::~Host( )
{
}

void Host::SetLoad( float load )
{
  this->lastupdate = time( NULL );
  this->load = load;
}

bool Host::Assign( )
{
  if( usage == slots )
    return false;
  usage++;
  assigned++;
  return true;
}

bool Host::Release( )
{
  if( usage == 0 )
    return false;
  usage--;
  return true;
}

bool Host::IsFree( )
{
  return ( usage < slots ) && ( load < loadlimit );
}

