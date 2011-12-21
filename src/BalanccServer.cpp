/*
 * balancc - load balancer
 *
 * author     Andre Roth <neolynx@gmail.com>
 * copyright  GNU General Public License, see COPYING file
 */

#include "BalanccServer.h"

#include <stdio.h>  // sscanf
#include <string.h> // strncmp

#include "Host.h"

BalanccServer::~BalanccServer( )
{
}

void BalanccServer::Connected( int client )
{
  //Log( "%d: got new client", client );
}

void BalanccServer::Disconnected( int client, bool error )
{
  //Log( "%d: client disconnected, error=%d", client, error );
  Lock( );
  iterator_hosts h = hosts.find( client );
  if( h == hosts.end( ))
    LogWarn( "%d: unknown client disconnected", client );
  else
  {
    for( iterator_Slot i = assignment.begin( ); i != assignment.end( ); i++ )
      if( i->first.client == client )
      {
        Log( "%d:%d slot destroyed: disconnect", i->first.client, i->first.id );
        assignment.erase( i );
      }
    Log( "%d: unregistered %s", client, h->second->GetName( ).c_str( ));
    delete h->second;
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
      LogError( "%d:%d slot already assigned", slot.client, slot.id );
    else
    {
      if( !h->Assign( ))
        LogError( "%d:%d host assignmenet failed", slot.client, slot.id );
      else
      {
        assignment[slot] = h;
        host = h->GetName( );
        Log( "%d:%d assigned to %s", slot.client, slot.id, host.c_str( ));
      }
    }
  }
  else
    LogNotice( "%d:%d all hosts busy", slot.client, slot.id );
  Unlock( );
  return host;
}

void BalanccServer::Housekeeping( )
{
  time_t now = time( NULL );
  Lock( );
  for( iterator_hosts i = hosts.begin( ); i != hosts.end( ); i++ )
  {
    if( now - (*i).second->LastUpdate( ) > 5 )
    {
      Log( "%d: overdue, disconnecting...", (*i).first );
      Unlock( );
      DisconnectClient( (*i).first );
      Lock( );
    }
  }
  Unlock( );
}

void BalanccServer::HandleMessage( const int client, const SocketHandler::Message &msg )
{
  //Log( "BalanccServer got message from %d: %s", client, msg.getLine( ).c_str( ));
  const char *buffer = msg.getLine( ).c_str( );
  int length = msg.getLine( ).length( );
  //Dump( buffer, length );
  int id;
  int r;
  r = sscanf( buffer, "get %d", &id );
  if( r == 1 )
  {
    //Log( "host request with id %d:%d", client, id );
    Slot slot( client, id );
    std::string host = GetHost( slot );
    char buf[64];
    snprintf( buf, sizeof( buf ), "%s %d\n", host.c_str( ), id );
    SendToClient( client, buf, strlen( buf ));
    return;
  }

  r = sscanf( buffer, "+get %d", &id ); // host request including self host
  if( r == 1 )
  {
    //Log( "host request with id %d:%d", client, id );
    Slot slot( client, id );
    std::string host = GetHost( slot, true );
    char buf[64];
    snprintf( buf, sizeof( buf ), "%s %d\n", host.c_str( ), id );
    SendToClient( client, buf, strlen( buf ));
    return;
  }
  r = sscanf( buffer, "free %d", &id );
  if( r == 1 )
  {
    Slot slot( client, id );
    Lock( );
    iterator_Slot i = assignment.find( slot );
    if( i == assignment.end( ))
      LogError( "%d:%d cannot free: host is not assigned", slot.client, slot.id );
    else
    {
      Log( "%d:%d freed %s", slot.client, slot.id, (*i).second->GetName( ).c_str( ));
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
    //Log( "%d: got host %s, %d CPUs", client, host, cpus );
    Lock( );
    if( hosts.find( client ) != hosts.end( ))
      LogError( "%d: host already registered", client );
    else
    {
      Log( "%d: registered %s", client, host );
      hosts[client] = new Host( host, cpus );
    }
    Unlock( );
    return;
  }
  r = sscanf( buffer, "load %f", &load );
  if( r == 1 )
  {
    Lock( );
    if( hosts.find( client ) == hosts.end( ))
      LogError( "%d: client unknown", client );
    else
      hosts[client]->SetLoad( load );
    Unlock( );
    //Log( "%d: got load %f", client, load );
    return;
  }
  LogError( "%d: unknown message: %s", client, buffer );
}

