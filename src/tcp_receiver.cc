#include "tcp_receiver.hh"
#include <algorithm>

using namespace std;

void TCPReceiver::receive( TCPSenderMessage message )
{
  if ( message.RST ) {
    reader().set_error();
  }
  if ( reader().has_error() ) {
    return;
  }
  if ( message.SYN && not isn_.has_value() ) {
    isn_ = message.seqno;
  }
  if ( not isn_.has_value() ) {
    return;
  }
  const uint64_t absolute = message.seqno.unwrap( isn_.value(), writer().bytes_pushed() + 1 );
  if ( absolute == 0 && not message.SYN ) {
    // A payload at absolute sequence zero overlaps the SYN's position.
    if ( message.payload.empty() ) {
      return;
    }
    message.payload.erase( 0, 1 );
    reassembler_.insert( 0, move( message.payload ), message.FIN );
    return;
  }
  reassembler_.insert( absolute - ( message.SYN ? 0 : 1 ), move( message.payload ), message.FIN );
}

TCPReceiverMessage TCPReceiver::send() const
{
  TCPReceiverMessage result;
  if ( isn_.has_value() ) {
    result.ackno = Wrap32::wrap( writer().bytes_pushed() + 1 + writer().is_closed(), isn_.value() );
  }
  result.window_size = static_cast<uint16_t>( min<uint64_t>( writer().available_capacity(), UINT16_MAX ) );
  result.RST = reader().has_error();
  return result;
}
