/*
* This is a personal academic project. Dear PVS-Studio, please check it.
* PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
*/
#ifndef SDTDESCRIPTOR_H
#define SDTDESCRIPTOR_H

#include <string>

struct SdtDescriptor {
  int serviceType = -1;
  std::string providerName;
  std::string serviceName;
  
  void print(const int indent = 0) const;
  
  const char *getServiceTypeName(uint8_t type) const;
  
};


#endif //SDTDESCRIPTOR_H
