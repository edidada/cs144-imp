#include "tcp_sender.hh"
#include "tcp_config.hh"
#include <algorithm>
#include <limits>

using namespace std;

// How many sequence numbers are outstanding?
uint64_t TCPSender::sequence_numbers_in_flight() const
{
  return next_ - acknowledged_;
}

// How many consecutive retransmissions have happened?
uint64_t TCPSender::consecutive_retransmissions() const
{
  return retransmissions_;
}

void TCPSender::push( const TransmitFunction& transmit )
{
  const uint64_t effective_window = max<uint64_t>( window_, 1 );
  while ( not fin_sent_ && sequence_numbers_in_flight() < effective_window ) {
    auto message = make_empty_message();
    message.SYN = next_ == 0;
    const uint64_t room = effective_window - sequence_numbers_in_flight();
    read( reader(), min<uint64_t>( TCPConfig::MAX_PAYLOAD_SIZE, room - message.SYN ), message.payload );
    message.FIN = reader().is_finished() && message.sequence_length() < room;
    if ( message.sequence_length() == 0 ) {
      break;
    }
    if ( outstanding_.empty() ) {
      elapsed_ms_ = 0;
    }
    next_ += message.sequence_length();
    fin_sent_ = message.FIN;
    outstanding_.push_back( Outstanding { message, next_ } );
    transmit( message );
  }
}

TCPSenderMessage TCPSender::make_empty_message() const
{
  return TCPSenderMessage { .seqno = Wrap32::wrap( next_, isn_ ), .RST = reader().has_error() };
}

void TCPSender::receive( const TCPReceiverMessage& msg )
{
  if ( msg.RST ) {
    writer().set_error();
  }
  window_ = msg.window_size;
  if ( not msg.ackno.has_value() ) {
    return;
  }
  const uint64_t ack = msg.ackno->unwrap( isn_, next_ );
  if ( ack > next_ || ack <= acknowledged_ ) {
    return;
  }
  acknowledged_ = ack;
  while ( not outstanding_.empty() && outstanding_.front().end <= ack ) {
    outstanding_.pop_front();
  }
  rto_ms_ = initial_RTO_ms_;
  elapsed_ms_ = 0;
  retransmissions_ = 0;
}

void TCPSender::tick( uint64_t ms_since_last_tick, const TransmitFunction& transmit )
{
  if ( outstanding_.empty() ) {
    return;
  }
  elapsed_ms_ += min( ms_since_last_tick, numeric_limits<uint64_t>::max() - elapsed_ms_ );
  if ( elapsed_ms_ < rto_ms_ ) {
    return;
  }
  auto message = outstanding_.front().message;
  message.RST = reader().has_error();
  transmit( message );
  if ( window_ != 0 ) {
    ++retransmissions_;
    rto_ms_ += min( rto_ms_, numeric_limits<uint64_t>::max() - rto_ms_ );
  }
  elapsed_ms_ = 0;
}
