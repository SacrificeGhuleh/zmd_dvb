/*
* This is a personal academic project. Dear PVS-Studio, please check it.
* PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
*/
#include <ac3audiodescriptor.h>
#include <helpers.h>
#include <cstdio>

void Ac3AudioDescriptor::print(const int indent) const {
  printIndent(indent);
  printf("AC3 Audio\n");
  
  printIndent(indent + 1);
  printf("sample_rate_code %d\n", sample_rate_code);
  printIndent(indent + 1);
  printf("bsid %d\n", bsid);
  printIndent(indent + 1);
  printf("bit_rate_code %d\n", bit_rate_code);
  printIndent(indent + 1);
  printf("surround_mode %d\n", surround_mode);
  printIndent(indent + 1);
  printf("bsmod %d\n", bsmod);
  printIndent(indent + 1);
  printf("num_channels %d\n", num_channels);
  printIndent(indent + 1);
  printf("full_svc %d\n", full_svc);
}