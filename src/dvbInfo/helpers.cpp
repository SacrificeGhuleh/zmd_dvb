/*
* This is a personal academic project. Dear PVS-Studio, please check it.
* PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
*/

#include <helpers.h>
#include <cstdint>

void hexDump(const void *buf, const int size) {
  const uint8_t *bufBytes = reinterpret_cast<const uint8_t *>(buf);
  int i;
  uint8_t byte;
  char sascii[17];
  
  sascii[16] = 0x0;
  
  for (i = 0; i < size; i++) {
    byte = bufBytes[i];
    if (i % 16 == 0) {
      printf("%04x ", i);
    }
    printf("%02x ", byte);
    if (byte >= ' ' && byte <= '}')
      sascii[i % 16] = byte;
    else
      sascii[i % 16] = '.';
    
    if (i % 16 == 15)
      printf("   %s\n", sascii);
  }
  
  // i++ after loop
  if (i % 16 != 0) {
    
    for (; i % 16 != 0; i++) {
      printf("   ");
      sascii[i % 16] = ' ';
    }
    
    printf("   %s\n", sascii);
  }
}

