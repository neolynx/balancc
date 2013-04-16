/*
 * balancc - load balancer
 *
 * author     Andre Roth <neolynx@gmail.com>
 * copyright  GNU General Public License, see COPYING file
 */

#include "BalanccServer.h"

#include <stdio.h>  // sscanf
#include <string.h> // strncmp
#include <math.h>   // INF

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
    for( iterator_Slot i = assignment.begin( ); i != assignment.end( ); )
      if( i->first.client == client )
      {
        Log( "%d:%d slot destroyed: disconnect", i->first.client, i->first.id );
        assignment.erase( i++ );
      }
      else
	i++;
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
  float t, load = INFINITY;
  Host *h = NULL;
  if( self )
    for( iterator_hosts i = hosts.begin( ); i != hosts.end( ); i++ )
      if( slot.client == (*i).first )
      {
        if( (*i).second->IsFree( ))
        {
          h = (*i).second;
          host = "localhost";
        }
        break;
      }
  if( !h )
  {
    for( iterator_hosts i = hosts.begin( ); i != hosts.end( ); i++ )
    {
      if( (*i).first == slot.client )
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
    if( h ) // found a host
      host = h->GetName( );
  }
  if( h )
  {
    iterator_Slot i = assignment.find( slot );
    if( assignment.find( slot ) != assignment.end( ))
    {
      LogError( "%d:%d slot already assigned", slot.client, slot.id );
      (*i).second->Release( );
      assignment.erase( slot );
    }
    else
    {
      if( !h->Assign( ))
        LogError( "%d:%d host assignmenet failed", slot.client, slot.id );
      else
      {
        assignment[slot] = h;
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
  for( iterator_hosts i = hosts.begin( ); i != hosts.end( ); )
  {
    if( difftime( now, (*i).second->LastUpdate( )) > HEARTBEAT * 2 )
    {
      Log( "%d: timeout, disconnecting...", (*i).first );
      Unlock( );
      DisconnectClient( (*i++).first );
    }
    else
      i++;
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

  r = sscanf( buffer, "-get %d", &id ); // host request excluding self host
  if( r == 1 )
  {
    //Log( "host request with id %d:%d", client, id );
    Slot slot( client, id );
    std::string host = GetHost( slot, false );
    char buf[64];
    snprintf( buf, sizeof( buf ), "%s %d\n", host.c_str( ), id );
    SendToClient( client, buf, strlen( buf ));
    return;
  }

  r = sscanf( buffer, "info %d", &id ); // info
  if( r == 1 )
  {
    char buf[128];
    snprintf( buf, sizeof( buf ), "%d: balancc - Load Balancer\n", id );
    SendToClient( client, buf, strlen( buf ));
    snprintf( buf, sizeof( buf ), "%d: °°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°\n", id );
    SendToClient( client, buf, strlen( buf ));
    snprintf( buf, sizeof( buf ), "%d: Hostname         Slots Load/Limit Jobs\n", id );
    SendToClient( client, buf, strlen( buf ));
    Host *h = NULL;
    int total_slots = 0;
    int total_jobs = 0;
    Lock( );
    for( iterator_hosts i = hosts.begin( ); i != hosts.end( ); i++ )
    {
      h = (*i).second;
      snprintf( buf, sizeof( buf ), "%d:  %-16.16s  %2d  %2.2f %2.2f  %d\n", id, h->GetName( ).c_str( ), h->GetSlots( ), h->GetLoad( ), h->GetLoadLimit( ), h->GetAssignCount( ));
      SendToClient( client, buf, strlen( buf ));
      total_slots += h->GetSlots( );
      total_jobs += h->GetAssignCount( );
    }
    int total_hosts = hosts.size( );
    Unlock( );
    snprintf( buf, sizeof( buf ), "%d: ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n", id );
    SendToClient( client, buf, strlen( buf ));
    snprintf( buf, sizeof( buf ), "%d: Total %3d hosts   %3d           %3d\n", id, total_hosts, total_slots, total_jobs );
    SendToClient( client, buf, strlen( buf ));
    snprintf( buf, sizeof( buf ), "%d: ^EOM\n", id );
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
  float loadlimit;
  int slots;
  r = sscanf( buffer, "host %s %d %f %d", host, &cpus, &loadlimit, &slots ); // FIXME: overflow
  if( r >= 2 )
  {
    if( r != 3 )
      loadlimit = LOADLIMIT;

    if( r != 4 )
      slots = 0;

    //Log( "%d: got host %s, %d CPUs", client, host, cpus );
    Lock( );
    if( hosts.find( client ) != hosts.end( ))
      LogError( "%d: host already registered", client );
    else
    {
      Log( "%d: registered %s", client, host );
      hosts[client] = new Host( host, cpus, loadlimit, slots );
    }
    Unlock( );
    return;
  }

  float load;
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

