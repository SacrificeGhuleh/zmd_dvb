/*
* This is a personal academic project. Dear PVS-Studio, please check it.
* PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
*/
#ifndef AC3AUDIODESCRIPTOR_H
#define AC3AUDIODESCRIPTOR_H

#include <cstdint>

struct Ac3AudioDescriptor {
  uint8_t sample_rate_code;
  uint8_t bsid;
  uint8_t bit_rate_code;
  uint8_t surround_mode;
  uint8_t bsmod;
  uint8_t num_channels;
  uint8_t full_svc;
  
  void print(const int indent = 0) const;
};


#endif //AC3AUDIODESCRIPTOR_H
