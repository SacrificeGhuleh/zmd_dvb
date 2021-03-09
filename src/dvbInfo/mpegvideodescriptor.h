/*
* This is a personal academic project. Dear PVS-Studio, please check it.
* PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
*/
#ifndef MPEGVIDEODESCRIPTOR_H
#define MPEGVIDEODESCRIPTOR_H

#include <cstdint>

struct MpegVideoDescriptor {
  uint8_t multiple_frame_rate_flag;
  uint8_t frame_rate_code;
  uint8_t mpeg_1_only_flag;
  uint8_t constrained_parameter_flag;
  uint8_t still_picture_flag;
  bool hasExtra = false;
  uint8_t profile_and_level_indication;
  uint8_t chroma_format;
  uint8_t frame_rate_extension;
  
  void print(const int indent = 0) const;
};


#endif //MPEGVIDEODESCRIPTOR_H
