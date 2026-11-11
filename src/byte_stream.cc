#include "byte_stream.hh"

#include <algorithm>
#include <utility>

using namespace std;

ByteStream::ByteStream( uint64_t capacity ) : capacity_( capacity ) {}

void Writer::push( string data )
{
  if ( closed_ ) {
    return;
  }
  const auto accepted = min<uint64_t>( data.size(), available_capacity() );
  if ( accepted == 0 ) {
    return;
  }
  if ( accepted < data.size() ) {
    data = data.substr( 0, accepted );
  }
  chunks_.push_back( move( data ) );
  buffered_ += accepted;
  pushed_ += accepted;
}

void Writer::close() { closed_ = true; }
bool Writer::is_closed() const { return closed_; }
uint64_t Writer::available_capacity() const { return capacity_ - buffered_; }
uint64_t Writer::bytes_pushed() const { return pushed_; }

string_view Reader::peek() const
{
  if ( chunks_.empty() ) {
    return {};
  }
  return string_view { chunks_.front() }.substr( front_offset_ );
}

void Reader::pop( uint64_t len )
{
  len = min( len, buffered_ );
  buffered_ -= len;
  popped_ += len;
  while ( len > 0 ) {
    const auto count = min<uint64_t>( len, chunks_.front().size() - front_offset_ );
    len -= count;
    front_offset_ += count;
    if ( front_offset_ == chunks_.front().size() ) {
      chunks_.pop_front();
      front_offset_ = 0;
    }
  }
}

bool Reader::is_finished() const { return closed_ and buffered_ == 0; }
uint64_t Reader::bytes_buffered() const { return buffered_; }
uint64_t Reader::bytes_popped() const { return popped_; }
