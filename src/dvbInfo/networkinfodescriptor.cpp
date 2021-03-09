/*
* This is a personal academic project. Dear PVS-Studio, please check it.
* PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
*/
#include <networkinfodescriptor.h>
#include <helpers.h>

#include <cstdio>

void NetworkInfoDescriptor::print(const int indent) const {
  printIndent(indent);
  printf("%s basic info: \n", name.c_str());
  
  printIndent(indent + 1);
  printf("Centre frequency: " DEC_HEX "\n", DEC_HEX_V(centre_frequency));
  printIndent(indent + 1);
  printf("Bandwidth: " DEC_HEX "\n", DEC_HEX_V(bandwidth));
  printIndent(indent + 1);
  printf("Priority: " DEC_HEX "\n", DEC_HEX_V(priority));
  printIndent(indent + 1);
  printf("Time slicingi ndicator: " DEC_HEX "\n", DEC_HEX_V(time_slicing_indicator));
  printIndent(indent + 1);
  printf("Mpe fec indicator: " DEC_HEX "\n", DEC_HEX_V(mpe_fec_indicator));
  printIndent(indent + 1);
  printf("Constellation: " DEC_HEX "\n", DEC_HEX_V(constellation));
  printIndent(indent + 1);
  printf("Hierarchy information: " DEC_HEX "\n", DEC_HEX_V(hierarchy_information));
  printIndent(indent + 1);
  printf("Code rate hp stream: " DEC_HEX "\n", DEC_HEX_V(code_rate_hp_stream));
  printIndent(indent + 1);
  printf("Code rate lp stream: " DEC_HEX "\n", DEC_HEX_V(code_rate_lp_stream));
  printIndent(indent + 1);
  printf("Guard interval: " DEC_HEX "\n", DEC_HEX_V(guard_interval));
  printIndent(indent + 1);
  printf("Transmission mode: " DEC_HEX "\n", DEC_HEX_V(transmission_mode));
  printIndent(indent + 1);
  printf("Other frequency flag: " DEC_HEX "\n", DEC_HEX_V(other_frequency_flag));
  
  printIndent(indent);
  printf("\nFrequencies: \n");
  
  for (const auto &freq : frequencies) {
    printIndent(indent + 1);
    printf("%d kHz\n", freq);
  }
}