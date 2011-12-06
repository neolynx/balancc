/*
 * balancc - load balancer
 *
 * author     Andre Roth <neolynx@gmail.com>
 * copyright  GNU General Public License, see COPYING file
 */

#include "BalanccServer.h"

#include <stdio.h>  // printf
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
  Lock( );
  if( hosts.find( client ) == hosts.end( ))
    printf( "unknown client %d disconnected\n", client );
  else
  {
    for( iterator_Slot i = assignment.begin( ); i != assignment.end( ); i++ )
      if( i->first.client == client )
      {
        printf( "%d:%d slot destroyed: disconnect\n", i->first.client, i->first.id );
        assignment.erase( i );
      }
    delete hosts[client];
    hosts.erase( client );
  }
  Unlock( );
}

std::string BalanccServer::GetHost( Slot slot, bool self )
{
  std::string host = "!";
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
      printf( "%d:%d slot already assigned\n", slot.client, slot.id );
    else
    {
      if( !h->Assign( ))
        printf( "%d:%d host assignmenet failed\n", slot.client, slot.id );
      else
      {
        assignment[slot] = h;
        host = h->GetName( );
        printf( "%d:%d assigned to %s\n", slot.client, slot.id, host.c_str( ));
      }
    }
  }
  else
    printf( "%d:%d all hosts busy\n", slot.client, slot.id );
  Unlock( );
  return host;
}

void BalanccServer::Polling( )
{
  Lock( );
  for( iterator_hosts i = hosts.begin( ); i != hosts.end( ); i++ )
  {
    Send( (*i).first, "?\n", 2 );
  }
  Unlock( );
}

void BalanccServer::HandleMessage( const int client, const SocketHandler::Message &msg )
{
  //printf( "BalanccServer got message from %d: %s\n", client, msg.getLine( ).c_str( ));
  const char *buffer = msg.getLine( ).c_str( );
  int length = msg.getLine( ).length( );
  //Dump( buffer, length );
  int id;
  int r;
  r = sscanf( buffer, "get %d", &id );
  if( r == 1 )
  {
    //printf( "host request with id %d:%d\n", client, id );
    Slot slot( client, id );
    std::string host = GetHost( slot );
    char buf[64];
    snprintf( buf, sizeof( buf ), "%s %d\n", host.c_str( ), id );
    Send( client, buf, strlen( buf ));
    return;
  }

  r = sscanf( buffer, "+get %d", &id ); // host request including self host
  if( r == 1 )
  {
    //printf( "host request with id %d:%d\n", client, id );
    Slot slot( client, id );
    std::string host = GetHost( slot, true );
    char buf[64];
    snprintf( buf, sizeof( buf ), "%s %d\n", host.c_str( ), id );
    Send( client, buf, strlen( buf ));
    return;
  }
  r = sscanf( buffer, "free %d", &id );
  if( r == 1 )
  {
    Slot slot( client, id );
    Lock( );
    iterator_Slot i = assignment.find( slot );
    if( i == assignment.end( ))
      printf( "%d:%d cannot free: host is not assigned\n", slot.client, slot.id );
    else
    {
      printf( "%d:%d freed %s\n", slot.client, slot.id, (*i).second->GetName( ).c_str( ));
      (*i).second->Release( );
      assignment.erase( slot );
    }
    Unlock( );
    return;
  }

  int cpus;
  char host[256];
  float load;
  r = sscanf( buffer, "host %s %d", host, &cpus ); // FIXME: overflow
  if( r == 2 )
  {
    //printf( "%d: got host %s, %d CPUs\n", client, host, cpus );
    Lock( );
    if( hosts.find( client ) != hosts.end( ))
      printf( "%d: host already registered\n", client );
    else
      hosts[client] = new Host( host, cpus );
    Unlock( );
    return;
  }
  r = sscanf( buffer, "load %f", &load );
  if( r == 1 )
  {
    Lock( );
    if( hosts.find( client ) == hosts.end( ))
      printf( "%d: client unknown\n", client );
    else
      hosts[client]->SetLoad( load );
    Unlock( );
    //printf( "%d: got load %f\n", client, load );
    return;
  }

  printf( "unknown message: %s\n", buffer );
}

