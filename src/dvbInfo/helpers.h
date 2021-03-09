/*
* This is a personal academic project. Dear PVS-Studio, please check it.
* PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
*/
#ifndef HELPERS_H
#define HELPERS_H

#include <cstdio>

#define DEC_HEX "%d (0x%02x)"
#define DEC_HEX_V(x) x,x

inline void printIndent(int indent) {
  if (indent > 0) {
    while (indent--) {
      printf("  ");
    }
  }
}

/// \brief Displays data in hex format
/// \param buf Pointer to input data
/// \param size Size in bytes of data
void hexDump(const void *buf, const int size);

template<typename T>
void hexDump(const T &buf) {
  hexDump(buf, sizeof(T));
}


#endif //HELPERS_H
