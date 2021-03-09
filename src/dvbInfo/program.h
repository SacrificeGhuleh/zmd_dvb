/*
* This is a personal academic project. Dear PVS-Studio, please check it.
* PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
*/
#ifndef PROGRAM_H
#define PROGRAM_H

#include <cstdint>
#include <string>
#include <vector>

#include <sdtdescriptor.h>
#include <mpegvideodescriptor.h>
#include <mpegaudiodescriptor.h>
#include <ac3audiodescriptor.h>

struct Program {
  uint16_t pid;
  uint16_t programNumber;
  SdtDescriptor sdtDescriptor;
  std::vector<std::string> services;
  std::vector<MpegVideoDescriptor> mpegVideoStreams;
  std::vector<MpegAudioDescriptor> mpegAudioSTreams;
  std::vector<Ac3AudioDescriptor> ac3AudioStreams;
  
  void print(const int indent = 0) const;
};


#endif //PROGRAM_H
