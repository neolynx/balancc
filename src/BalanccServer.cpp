/*
 * balancc - load balancer
 *
 * author     Andre Roth <neolynx@gmail.com>
 * copyright  GNU General Public License, see COPYING file
 */

#include "BalanccServer.h"

#include <stdio.h> // printf
#include <string.h> // strncmp

#include "Host.h"

BalanccServer::~BalanccServer( )
{
}

void BalanccServer::Connected( int client )
{
  //printf( "%d: got new client\n", client );
}

void BalanccServer::Disconnected( int client, bool error )
{
  //printf( "%d: client disconnected, error=%d\n", client, error );
  if( hosts.find( client ) == hosts.end( ))
  {
    printf( "unknown client %d disconnected\n", client );
    return;
  }
  Lock( );
  delete hosts[client];
  hosts.erase( client );
  Unlock( );
}

int BalanccServer::DataReceived( int client, const char *buffer, int length )
{
  printf( "BalanccServer %d: got %d bytes: %s\n", client, length, buffer );
  Dump( buffer, length );
  int len = 0;
  int id;
  int r;
  r = sscanf( buffer, "get %d", &id );
  if( r == 1 )
  {
    len = strlen( buffer ) + 1;
    printf( "host request with id %d:%d\n", client, id );
    Slot slot( client, id );
    std::string host = GetHost( slot );
    Send( client, host.c_str( ), host.length( ) + 1 ); // include \0
    printf( "eating: %d of %s\n", len, buffer );
    return len;
  }

  r = sscanf( buffer, "+get %d", &id ); // host request including self host
  if( r == 1 )
  {
    len = strlen( buffer ) + 1;
    printf( "host request with id %d:%d\n", client, id );
    Slot slot( client, id );
    std::string host = GetHost( slot, true );
    Send( client, host.c_str( ), host.length( ) + 1 ); // include \0
    printf( "eating: %d of %s\n", len, buffer );
    return len;
  }
  r = sscanf( buffer, "done %d", &id );
  if( r == 1 )
  {
    len = strlen( buffer ) + 1;
    printf( "host done, id %d:%d\n", client, id );
    Slot slot( client, id );
    Lock( );
    if( assignment.find( slot ) == assignment.end( ))
      printf( "host is not assigned\n" );
    else
    {
      assignment[slot]->Release( );
      assignment.erase( slot );
    }
    Unlock( );
    printf( "eating: %d of %s\n", len, buffer );
    return len;
  }

  int cpus;
  char host[256];
  float load;
  r = sscanf( buffer, "host %s %d", host, &cpus ); // FIXME: oberflow
  if( r == 2 )
  {
    len = strlen( buffer ) + 1;
    //printf( "%d: got host %s, %d CPUs\n", client, host, cpus );
    if( hosts.find( client ) != hosts.end( ))
    {
      printf( "%d: host already registered\n", client );
      printf( "eating: %d of %s\n", len, buffer );
      return len;
    }
    Lock( );
    hosts[client] = new Host( host, cpus );
    Unlock( );
    printf( "eating: %d", len, buffer );
    return len;
  }
  r = sscanf( buffer, "load %f", &load );
  if( r == 1 )
  {
    len = strlen( buffer ) + 1;
    if( hosts.find( client ) == hosts.end( ))
    {
      printf( "%d: client unknown\n", client );
      printf( "eating: %d of %s\n", len, buffer );
      return len;
    }
    hosts[client]->SetLoad( load );
    //printf( "%d: got load %f\n", client, load );
    printf( "eating: %d of %s\n", len, buffer );
    return len;
  }

  printf( "unknown message: %s\n", buffer );
  printf( "eating: %d of %s\n", len, buffer );
  up = false;
  return len;
}

std::string BalanccServer::GetHost( Slot slot, bool self )
{
  std::string host = "";
  Lock( );
  float t, load = 1000.0;
  Host *h = NULL;
  for( iterator_hosts i = hosts.begin( ); i != hosts.end( ); i++ )
  {
    if( !self && (*i).first == slot.client )
      continue;
    if( !(*i).second->IsFree( ))
      continue;
    t = (*i).second->GetLoad( );
    if( t < load )
    {
      load = t;
      h = (*i).second;
    }
  }
  if( h )
  {
    if( assignment.find( slot ) != assignment.end( ))
      printf( "slot already assigned\n" );
    else
    {
      if( !h->Assign( ))
        printf( "host assignmenet failed\n" );
      else
      {
        assignment[slot] = h;
        host = h->GetName( );
      }
    }
  }
  Unlock( );
  return host;
}

void BalanccServer::Polling( )
{
  Lock( );
  for( iterator_hosts i = hosts.begin( ); i != hosts.end( ); i++ )
  {
    Send( (*i).first, "?", 2 );
  }
  Unlock( );
}

