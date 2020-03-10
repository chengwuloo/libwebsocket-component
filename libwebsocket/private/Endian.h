
#ifndef MUDUO_NET_ENDIAN_H
#define MUDUO_NET_ENDIAN_H

#include <stdint.h>
#include <endian.h>

namespace muduo
{
namespace net
{
namespace Endian
{

// the inline assembler code makes type blur,
// so we disable warnings for a while.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wold-style-cast"
inline uint64_t hostToNetwork64(uint64_t host64)
{
  return htobe64(host64);
}

inline uint32_t hostToNetwork32(uint32_t host32)
{
  return htobe32(host32);
}

inline uint16_t hostToNetwork16(uint16_t host16)
{
  return htobe16(host16);
}

inline uint64_t networkToHost64(uint64_t net64)
{
  return be64toh(net64);
}

inline uint32_t networkToHost32(uint32_t net32)
{
  return be32toh(net32);
}

inline uint16_t networkToHost16(uint16_t net16)
{
  return be16toh(net16);
}

//be16BigEndian
inline uint16_t be16BigEndian(uint8_t const ch[2]) {
	uint8_t const c0 = ch[0];
	uint8_t const c1 = ch[1];
	uint8_t const c2 = ch[2];
	uint8_t const c3 = ch[3];
	return (uint16_t)((c0 << 8) | c1);
}

//be16LittleEndian
inline uint16_t be16LittleEndian(uint8_t const ch[2]) {
	uint8_t const c0 = ch[0];
	uint8_t const c1 = ch[1];
	return (uint16_t)((c1 << 8) | c0);
}

//be32BigEndian
inline uint32_t be32BigEndian(uint8_t const ch[4]) {
	uint8_t const c0 = ch[0];
	uint8_t const c1 = ch[1];
	uint8_t const c2 = ch[2];
	uint8_t const c3 = ch[3];
	return (uint32_t)((c0 << 24) | (c1 << 16) | (c2 << 8) | c3);
}

//be32LittleEndian
inline uint32_t be32LittleEndian(uint8_t const ch[4]) {
	uint8_t const c0 = ch[0];
	uint8_t const c1 = ch[1];
	uint8_t const c2 = ch[2];
	uint8_t const c3 = ch[3];
	return (uint32_t)((c3 << 24) | (c2 << 16) | (c1 << 8) | c0);
}

//be64BigEndian
inline uint64_t be64BigEndian(uint8_t const ch[8]) {
	uint8_t const c0 = ch[0];
	uint8_t const c1 = ch[1];
	uint8_t const c2 = ch[2];
	uint8_t const c3 = ch[3];
	uint8_t const c4 = ch[4];
	uint8_t const c5 = ch[5];
	uint8_t const c6 = ch[6];
	uint8_t const c7 = ch[7];
	return (uint64_t)((c0 << 56) | (c1 << 48) | (c2 << 40) | (c3 << 32) | (c4 << 24) | (c5 << 16) | (c6 << 8) | c7);
}

//be64LittleEndian
inline uint64_t be64LittleEndian(uint8_t const ch[8]) {
	uint8_t const c0 = ch[0];
	uint8_t const c1 = ch[1];
	uint8_t const c2 = ch[2];
	uint8_t const c3 = ch[3];
	uint8_t const c4 = ch[4];
	uint8_t const c5 = ch[5];
	uint8_t const c6 = ch[6];
	uint8_t const c7 = ch[7];
	return (uint64_t)((c7 << 56) | (c6 << 48) | (c5 << 40) | (c4 << 32) | (c3 << 24) | (c2 << 16) | (c1 << 8) | c0);
}

#pragma GCC diagnostic pop

}  // namespace sockets
}  // namespace net
}  // namespace muduo

#endif  // MUDUO_NET_ENDIAN_H
