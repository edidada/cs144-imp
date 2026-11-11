#include <iostream>

#include "arp_message.hh"
#include "ethernet_frame.hh"
#include "exception.hh"
#include "helpers.hh"
#include "network_interface.hh"

using namespace std;

//! \param[in] ethernet_address Ethernet (what ARP calls "hardware") address of the interface
//! \param[in] ip_address IP (what ARP calls "protocol") address of the interface
NetworkInterface::NetworkInterface( string_view name,
                                    shared_ptr<OutputPort> port,
                                    const EthernetAddress& ethernet_address,
                                    const Address& ip_address )
  : name_( name )
  , port_( notnull( "OutputPort", move( port ) ) )
  , ethernet_address_( ethernet_address )
  , ip_address_( ip_address )
{
  cerr << "DEBUG: Network interface has Ethernet address " << to_string( ethernet_address_ ) << " and IP address "
       << ip_address.ip() << "\n";
}

//! \param[in] dgram the IPv4 datagram to be sent
//! \param[in] next_hop the IP address of the interface to send it to (typically a router or default gateway, but
//! may also be another host if directly connected to the same network as the destination) Note: the Address type
//! can be converted to a uint32_t (raw 32-bit IP address) by using the Address::ipv4_numeric() method.
void NetworkInterface::send_datagram( InternetDatagram dgram, const Address& next_hop )
{
  const uint32_t ip = next_hop.ipv4_numeric();
  if ( const auto mapping = mappings_.find( ip ); mapping != mappings_.end() ) {
    send_ipv4( dgram, mapping->second.address );
    return;
  }
  auto [it, fresh] = pending_.try_emplace( ip );
  it->second.datagrams.push( move( dgram ) );
  if ( fresh ) {
    ARPMessage request;
    request.opcode = ARPMessage::OPCODE_REQUEST;
    request.sender_ethernet_address = ethernet_address_;
    request.sender_ip_address = ip_address_.ipv4_numeric();
    request.target_ip_address = ip;
    transmit( EthernetFrame { { ETHERNET_BROADCAST, ethernet_address_, EthernetHeader::TYPE_ARP },
                              serialize( request ) } );
  }
}

void NetworkInterface::send_ipv4( const InternetDatagram& datagram, const EthernetAddress& destination )
{
  transmit( EthernetFrame { { destination, ethernet_address_, EthernetHeader::TYPE_IPv4 }, serialize( datagram ) } );
}

//! \param[in] frame the incoming Ethernet frame
void NetworkInterface::recv_frame( EthernetFrame frame )
{
  if ( frame.header.dst != ethernet_address_ && frame.header.dst != ETHERNET_BROADCAST ) {
    return;
  }
  if ( frame.header.type == EthernetHeader::TYPE_IPv4 ) {
    InternetDatagram datagram;
    if ( parse( datagram, move( frame.payload ) ) ) {
      datagrams_received_.push( move( datagram ) );
    }
    return;
  }
  if ( frame.header.type != EthernetHeader::TYPE_ARP ) {
    return;
  }
  ARPMessage message;
  if ( not parse( message, move( frame.payload ) ) ) {
    return;
  }
  mappings_.insert_or_assign( message.sender_ip_address, Mapping { message.sender_ethernet_address, 30000 } );
  // A request also teaches the sender mapping; it can unblock queued data.
  if ( auto pending = pending_.find( message.sender_ip_address ); pending != pending_.end() ) {
    auto datagrams = move( pending->second.datagrams );
    pending_.erase( pending );
    while ( not datagrams.empty() ) {
      send_ipv4( datagrams.front(), message.sender_ethernet_address );
      datagrams.pop();
    }
  }
  if ( message.opcode == ARPMessage::OPCODE_REQUEST && message.target_ip_address == ip_address_.ipv4_numeric() ) {
    ARPMessage reply;
    reply.opcode = ARPMessage::OPCODE_REPLY;
    reply.sender_ethernet_address = ethernet_address_;
    reply.sender_ip_address = ip_address_.ipv4_numeric();
    reply.target_ethernet_address = message.sender_ethernet_address;
    reply.target_ip_address = message.sender_ip_address;
    transmit( EthernetFrame { { message.sender_ethernet_address, ethernet_address_, EthernetHeader::TYPE_ARP },
                              serialize( reply ) } );
  }
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void NetworkInterface::tick( const size_t ms_since_last_tick )
{
  // Countdown arithmetic handles arbitrarily large tick values without overflow.
  for ( auto it = mappings_.begin(); it != mappings_.end(); ) {
    if ( ms_since_last_tick >= it->second.remaining_ms ) {
      it = mappings_.erase( it );
    } else {
      it->second.remaining_ms -= ms_since_last_tick;
      ++it;
    }
  }
  for ( auto it = pending_.begin(); it != pending_.end(); ) {
    if ( ms_since_last_tick >= it->second.remaining_ms ) {
      it = pending_.erase( it ); // Unresolved datagrams must not outlive the request window.
    } else {
      it->second.remaining_ms -= ms_since_last_tick;
      ++it;
    }
  }
}
