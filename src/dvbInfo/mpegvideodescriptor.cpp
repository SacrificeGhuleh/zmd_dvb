/*
* This is a personal academic project. Dear PVS-Studio, please check it.
* PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
*/
#include <cstdio>
#include <mpegvideodescriptor.h>
#include <helpers.h>

void MpegVideoDescriptor::print(const int indent) const {
  printIndent(indent);
  printf("MPEG video\n");
  printIndent(indent + 1);
  printf("multiple_frame_rate_flag %d\n", multiple_frame_rate_flag);
  printIndent(indent + 1);
  printf("frame_rate_code %d\n", frame_rate_code);
  printIndent(indent + 1);
  printf("mpeg_1_only_flag %d\n", mpeg_1_only_flag);
  printIndent(indent + 1);
  printf("constrained_parameter_flag %d\n", constrained_parameter_flag);
  printIndent(indent + 1);
  printf("still_picture_flag %d\n", still_picture_flag);
  if (hasExtra) {
    printIndent(indent + 1);
    printf("profile_and_level_indication %d\n", profile_and_level_indication);
    printIndent(indent + 1);
    printf("chroma_format %d\n", chroma_format);
    printIndent(indent + 1);
    printf("frame_rate_extension %d\n", frame_rate_extension);
  }
}