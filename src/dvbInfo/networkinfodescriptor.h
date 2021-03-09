/*
* This is a personal academic project. Dear PVS-Studio, please check it.
* PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
*/
#ifndef NETWORKINFODESCRIPTOR_H
#define NETWORKINFODESCRIPTOR_H

#include <cstdint>
#include <vector>
#include <string>

struct NetworkInfoDescriptor {
  std::vector<uint32_t> frequencies;
  std::string name;
  uint32_t centre_frequency;
  uint8_t bandwidth;
  uint8_t priority;
  uint8_t time_slicing_indicator;
  uint8_t mpe_fec_indicator;
  uint8_t constellation;
  uint8_t hierarchy_information;
  uint8_t code_rate_hp_stream;
  uint8_t code_rate_lp_stream;
  uint8_t guard_interval;
  uint8_t transmission_mode;
  uint8_t other_frequency_flag;
  
  void print(const int indent = 0) const;
};


#endif //NETWORKINFODESCRIPTOR_H
