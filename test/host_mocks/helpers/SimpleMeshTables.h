#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>

namespace mesh {

static constexpr uint8_t ROUTE_TYPE_FLOOD = 0x01;
static constexpr uint8_t ROUTE_TYPE_DIRECT = 0x02;
static constexpr uint8_t PAYLOAD_TYPE_TXT_MSG = 0x02;
static constexpr uint8_t PH_TYPE_SHIFT = 2;
static constexpr size_t MAX_HASH_SIZE = 8;
static constexpr size_t MAX_PACKET_PAYLOAD = 184;

struct Packet {
  uint8_t header = 0;
  uint8_t path_len = 0;
  uint8_t payload_len = 0;
  uint8_t payload[MAX_PACKET_PAYLOAD]{};

  bool isRouteDirect() const { return (header & 0x03U) == ROUTE_TYPE_DIRECT; }
  uint8_t getPayloadType() const { return static_cast<uint8_t>((header >> PH_TYPE_SHIFT) & 0x0FU); }

  void calculatePacketHash(uint8_t hash[MAX_HASH_SIZE]) const {
    uint8_t state[32]{};
    const uint8_t type = getPayloadType();
    for (size_t j = 0; j < 32; j++) {
      state[j] = static_cast<uint8_t>((state[j] * 33U) ^ (type + j));
    }
    for (size_t i = 0; i < payload_len; i++) {
      const uint8_t b = payload[i];
      for (size_t j = 0; j < 32; j++) {
        state[j] = static_cast<uint8_t>((state[j] * 33U) ^ (b + j + i + 1U));
      }
    }
    std::memcpy(hash, state, MAX_HASH_SIZE);
  }
};

class MeshTables {
public:
  virtual ~MeshTables() = default;
  virtual bool wasSeen(const Packet* packet) = 0;
  virtual void markSeen(const Packet* packet) = 0;
  virtual void clear(const Packet* packet) = 0;
};

}  // namespace mesh

#ifndef MAX_PACKET_HASHES
  #if defined(_SEEED_SENSECAP_SOLAR_H_)
    #define MAX_PACKET_HASHES 512
  #else
    #define MAX_PACKET_HASHES (128 + 32)
  #endif
#endif

class SimpleMeshTables : public mesh::MeshTables {
  uint8_t _hashes[MAX_PACKET_HASHES * mesh::MAX_HASH_SIZE]{};
  int _next_idx = 0;
  uint32_t _direct_dups = 0;
  uint32_t _flood_dups = 0;

public:
  SimpleMeshTables() = default;

  bool wasSeen(const mesh::Packet* packet) override {
    uint8_t hash[mesh::MAX_HASH_SIZE];
    packet->calculatePacketHash(hash);

    const uint8_t* sp = _hashes;
    for (int i = 0; i < MAX_PACKET_HASHES; i++, sp += mesh::MAX_HASH_SIZE) {
      if (std::memcmp(hash, sp, mesh::MAX_HASH_SIZE) == 0) {
        if (packet->isRouteDirect()) {
          _direct_dups++;
        } else {
          _flood_dups++;
        }
        return true;
      }
    }
    return false;
  }

  void markSeen(const mesh::Packet* packet) override {
    uint8_t hash[mesh::MAX_HASH_SIZE];
    packet->calculatePacketHash(hash);

    const uint8_t* sp = _hashes;
    for (int i = 0; i < MAX_PACKET_HASHES; i++, sp += mesh::MAX_HASH_SIZE) {
      if (std::memcmp(hash, sp, mesh::MAX_HASH_SIZE) == 0) {
        return;
      }
    }

    std::memcpy(&_hashes[_next_idx * mesh::MAX_HASH_SIZE], hash, mesh::MAX_HASH_SIZE);
    _next_idx = (_next_idx + 1) % MAX_PACKET_HASHES;
  }

  void clear(const mesh::Packet* packet) override {
    uint8_t hash[mesh::MAX_HASH_SIZE];
    packet->calculatePacketHash(hash);

    uint8_t* sp = _hashes;
    for (int i = 0; i < MAX_PACKET_HASHES; i++, sp += mesh::MAX_HASH_SIZE) {
      if (std::memcmp(hash, sp, mesh::MAX_HASH_SIZE) == 0) {
        std::memset(sp, 0, mesh::MAX_HASH_SIZE);
        break;
      }
    }
  }
};
