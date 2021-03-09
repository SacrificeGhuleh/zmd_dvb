/*
* This is a personal academic project. Dear PVS-Studio, please check it.
* PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
*/
#include <mpegaudiodescriptor.h>
#include <helpers.h>
#include <cstdio>

void MpegAudioDescriptor::print(const int indent) const {
  printIndent(indent);
  printf("MPEG Audio\n");
  printIndent(indent + 1);
  printf("free_format_flag %d\n", free_format_flag);
  printIndent(indent + 1);
  printf("id %d\n", id);
  printIndent(indent + 1);
  printf("layer %d\n", layer);
  printIndent(indent + 1);
  printf("variable_rate_audio_indicator %d\n", variable_rate_audio_indicator);
}