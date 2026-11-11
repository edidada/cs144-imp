#include "wrapping_integers.hh"
#include <limits>

using namespace std;

Wrap32 Wrap32::wrap( uint64_t n, Wrap32 zero_point )
{
  return zero_point + static_cast<uint32_t>( n );
}

uint64_t Wrap32::unwrap( Wrap32 zero_point, uint64_t checkpoint ) const
{
  constexpr uint64_t period = uint64_t { 1 } << 32;
  const uint32_t offset = raw_value_ - zero_point.raw_value_;
  const uint64_t candidate = ( checkpoint & ~( period - 1 ) ) | offset;
  const auto distance = [checkpoint]( uint64_t n ) { return n > checkpoint ? n - checkpoint : checkpoint - n; };
  uint64_t best = candidate;
  if ( candidate >= period && distance( candidate - period ) < distance( best ) ) {
    best = candidate - period;
  }
  if ( candidate <= numeric_limits<uint64_t>::max() - period
       && distance( candidate + period ) < distance( best ) ) {
    best = candidate + period;
  }
  return best;
}
