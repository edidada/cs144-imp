#include "router.hh"

#include <iostream>
#include <stdexcept>

using namespace std;

// route_prefix: The "up-to-32-bit" IPv4 address prefix to match the datagram's destination address against
// prefix_length: For this route to be applicable, how many high-order (most-significant) bits of
//    the route_prefix will need to match the corresponding bits of the datagram's destination address?
// next_hop: The IP address of the next hop. Will be empty if the network is directly attached to the router (in
//    which case, the next hop address should be the datagram's final destination).
// interface_num: The index of the interface to send the datagram out on.
void Router::add_route( const uint32_t route_prefix,
                        const uint8_t prefix_length,
                        const optional<Address> next_hop,
                        const size_t interface_num )
{
  cerr << "DEBUG: adding route " << Address::from_ipv4_numeric( route_prefix ).ip() << "/"
       << static_cast<int>( prefix_length ) << " => " << ( next_hop.has_value() ? next_hop->ip() : "(direct)" )
       << " on interface " << interface_num << "\n";

  if ( prefix_length > 32 ) {
    throw invalid_argument( "IPv4 route prefix length must be at most 32" );
  }
  // /0 is special: shifting a uint32_t by 32 would be undefined.
  const uint32_t mask = prefix_length == 0 ? 0 : UINT32_MAX << ( 32 - prefix_length );
  routes_.push_back( Route { route_prefix & mask, mask, prefix_length, next_hop, interface_num } );
}

// Go through all the interfaces, and route every incoming datagram to its proper outgoing interface.
void Router::route()
{
  for ( const auto& incoming : interfaces_ ) {
    auto& queue = incoming->datagrams_received();
    while ( not queue.empty() ) {
      auto datagram = move( queue.front() );
      queue.pop();
      if ( datagram.header.ttl <= 1 ) {
        continue;
      }
      const Route* best = nullptr;
      for ( const auto& candidate : routes_ ) {
        if ( ( datagram.header.dst & candidate.mask ) == candidate.prefix
             && ( best == nullptr || candidate.length > best->length ) ) {
          best = &candidate;
        }
      }
      if ( best == nullptr ) {
        continue;
      }
      --datagram.header.ttl;
      datagram.header.compute_checksum();
      const Address next_hop = best->next_hop.value_or( Address::from_ipv4_numeric( datagram.header.dst ) );
      interface( best->interface_num )->send_datagram( move( datagram ), next_hop );
    }
  }
}
