/*
* This is a personal academic project. Dear PVS-Studio, please check it.
* PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
*/
#ifndef MPEGAUDIODESCRIPTOR_H
#define MPEGAUDIODESCRIPTOR_H

#include <cstdint>

struct MpegAudioDescriptor {
  uint8_t free_format_flag;
  uint8_t id;
  uint8_t layer;
  uint8_t variable_rate_audio_indicator;
  
  void print(const int indent = 0) const;
};


#endif //MPEGAUDIODESCRIPTOR_H
