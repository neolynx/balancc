/*
 * balancc - load balancer
 *
 * author     Andre Roth <neolynx@gmail.com>
 * copyright  GNU General Public License, see COPYING file
 */

#include "Host.h"

#include <stdio.h> // printf

Host::Host( const char *name, int cpus ) : name(name), cpus(cpus), usage(0)
{
  load = 1000.0;
  printf( "registered %s\n", name );
}

Host::~Host( )
{
  printf( "unregistered %s\n", name.c_str( ));
}

void Host::SetLoad( float load )
{
  //printf( "%s load: %f\n", name.c_str( ), load );
  this->load = load;
}

bool Host::Assign( )
{
  if( usage == cpus * SLOTS_PER_CPU + SLOTS_ADDITIONAL )
    return false;
  usage++;
  //printf( "%s: usage %d\n", name.c_str( ), usage );
  return true;
}

bool Host::Release( )
{
  if( usage == 0 )
    return false;
  usage--;
  //printf( "%s: usage %d\n", name.c_str( ), usage );
  return true;
}

bool Host::IsFree( )
{
  return usage < cpus * SLOTS_PER_CPU + SLOTS_ADDITIONAL;
}

