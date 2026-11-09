#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>

struct TransportKey {
  uint8_t key[16]{};

  bool isNull() const {
    for (size_t i = 0; i < sizeof(key); i++) {
      if (key[i] != 0) {
        return false;
      }
    }
    return true;
  }
};

#ifndef MAX_TKS_ENTRIES
  #define MAX_TKS_ENTRIES 64
#endif

class TransportKeyStore {
  uint16_t cache_ids[MAX_TKS_ENTRIES]{};
  TransportKey cache_keys[MAX_TKS_ENTRIES]{};
  int num_cache = 0;
  int next_evict_idx = 0;

  static void deriveKey(const char* name, uint8_t out[16]) {
    uint8_t state[32]{};
    const size_t len = std::strlen(name);
    for (size_t i = 0; i < len; i++) {
      const auto b = static_cast<uint8_t>(name[i]);
      for (size_t j = 0; j < 32; j++) {
        state[j] = static_cast<uint8_t>((state[j] * 33U) ^ (b + j + i));
      }
    }
    std::memcpy(out, state, 16);
  }

  void putCache(uint16_t id, const TransportKey& key) {
    for (int i = 0; i < num_cache; i++) {
      if (cache_ids[i] == id) {
        cache_keys[i] = key;
        return;
      }
    }
    if (num_cache < MAX_TKS_ENTRIES) {
      cache_ids[num_cache] = id;
      cache_keys[num_cache] = key;
      num_cache++;
    } else {
      cache_ids[next_evict_idx] = id;
      cache_keys[next_evict_idx] = key;
      next_evict_idx = (next_evict_idx + 1) % MAX_TKS_ENTRIES;
    }
  }

  void invalidateCache() {
    num_cache = 0;
    next_evict_idx = 0;
  }

public:
  TransportKeyStore() = default;

  void getAutoKeyFor(uint16_t id, const char* name, TransportKey& dest) {
    for (int i = 0; i < num_cache; i++) {
      if (cache_ids[i] == id) {
        dest = cache_keys[i];
        return;
      }
    }
    deriveKey(name, dest.key);
    putCache(id, dest);
  }

  int loadKeysFor(uint16_t id, TransportKey keys[], int max_num) {
    int n = 0;
    for (int i = 0; i < num_cache && n < max_num; i++) {
      if (cache_ids[i] == id) {
        keys[n++] = cache_keys[i];
      }
    }
    return n;
  }

  bool saveKeysFor(uint16_t, const TransportKey[], int) {
    invalidateCache();
    return false;
  }

  bool removeKeys(uint16_t) {
    invalidateCache();
    return false;
  }

  bool clear() {
    invalidateCache();
    return false;
  }
};
