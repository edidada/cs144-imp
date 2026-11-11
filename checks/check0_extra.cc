#include "byte_stream.hh"

#include <algorithm>
#include <deque>
#include <iostream>
#include <random>
#include <stdexcept>
#include <string>
#include <utility>

using namespace std;

namespace {
void require( bool condition )
{
  if ( not condition ) {
    throw runtime_error( "ByteStream oracle mismatch" );
  }
}

void check( uint64_t capacity )
{
  ByteStream stream { capacity };
  deque<char> oracle {};
  mt19937 random { 144 };
  uint64_t pushed = 0;
  uint64_t popped = 0;
  for ( unsigned int step = 0; step < 10000; ++step ) {
    if ( random() % 2 == 0 ) {
      string data( random() % 97, '\0' );
      for ( char& byte : data ) {
        byte = static_cast<char>( random() % 256 );
      }
      const auto accepted = min<uint64_t>( capacity - oracle.size(), data.size() );
      for ( uint64_t i = 0; i < accepted; ++i ) {
        oracle.push_back( data[i] );
      }
      stream.writer().push( move( data ) );
      pushed += accepted;
    } else {
      const auto count = min<uint64_t>( random() % 97, oracle.size() );
      stream.reader().pop( count );
      for ( uint64_t i = 0; i < count; ++i ) {
        oracle.pop_front();
      }
      popped += count;
    }
    const auto view = stream.reader().peek();
    require( view.size() <= oracle.size() );
    require( oracle.empty() == view.empty() );
    for ( size_t i = 0; i < view.size(); ++i ) {
      require( view[i] == oracle[i] );
    }
    require( stream.reader().bytes_buffered() == oracle.size() );
    require( stream.writer().available_capacity() == capacity - oracle.size() );
    require( stream.reader().bytes_popped() == popped );
    require( stream.writer().bytes_pushed() == pushed );
  }
  stream.writer().close();
  stream.writer().close();
  stream.writer().push( "ignored after close" );
  require( stream.writer().bytes_pushed() == pushed );
  string remaining;
  read( stream.reader(), capacity, remaining );
  require( remaining == string( oracle.begin(), oracle.end() ) );
  require( stream.reader().is_finished() );
  stream.set_error();
  require( stream.has_error() );
}
} // namespace

int main()
{
  try {
    for ( const uint64_t capacity : { 0, 1, 31, 4096 } ) {
      check( capacity );
    }
    cout << "4 capacities x 10000 oracle operations, binary bytes, close and error passed\n";
  } catch ( const exception& error ) {
    cerr << error.what() << '\n';
    return 1;
  }
}
