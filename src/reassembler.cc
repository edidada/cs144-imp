#include "reassembler.hh"
#include <algorithm>
#include <limits>

using namespace std;

void Reassembler::insert( uint64_t first_index, string data, bool is_last_substring )
{
  if ( writer().is_closed() ) {
    return;
  }
  // Remember EOF even when the payload falls outside the receive window.
  if ( is_last_substring && data.size() <= numeric_limits<uint64_t>::max() - first_index ) {
    eof_ = first_index + data.size();
  }
  const uint64_t next = writer().bytes_pushed();
  const uint64_t available = writer().available_capacity();
  const uint64_t limit = next + min( available, numeric_limits<uint64_t>::max() - next );
  const uint64_t begin = max( next, first_index );
  if ( not bytes_.empty() && begin < limit && begin - first_index < data.size() ) {
    const uint64_t offset = begin - first_index;
    const uint64_t length = min( limit - begin, data.size() - offset );
    for ( uint64_t i = 0; i < length; ++i ) {
      const uint64_t slot = ( begin + i ) % bytes_.size();
      // Retransmissions must not count or store the same byte twice.
      if ( not present_[slot] ) {
        bytes_[slot] = data[offset + i];
        present_[slot] = true;
      }
    }
    string contiguous;
    for ( uint64_t index = next; index < limit && present_[index % bytes_.size()]; ++index ) {
      const uint64_t slot = index % bytes_.size();
      contiguous.push_back( bytes_[slot] );
      present_[slot] = false;
    }
    output_.writer().push( move( contiguous ) );
  }
  if ( eof_.has_value() && writer().bytes_pushed() == eof_.value() ) {
    output_.writer().close();
  }
}

// How many bytes are stored in the Reassembler itself?
// This function is for testing only; don't add extra state to support it.
uint64_t Reassembler::count_bytes_pending() const
{
  return count( present_.begin(), present_.end(), true );
}
