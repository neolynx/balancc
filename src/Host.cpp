/*
 * balancc - load balancer
 *
 * author     Andre Roth <neolynx@gmail.com>
 * copyright  GNU General Public License, see COPYING file
 */

#include "Host.h"

Host::Host( const char *name, int cpus ) : name(name), cpus(cpus), usage(0)
{
  load = 1000.0;
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
  if( usage == cpus * SLOTS_PER_CPU + SLOTS_ADDITIONAL )
    return false;
  usage++;
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
  return usage < cpus * SLOTS_PER_CPU + SLOTS_ADDITIONAL;
}

