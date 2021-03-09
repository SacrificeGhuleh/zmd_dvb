/*
* This is a personal academic project. Dear PVS-Studio, please check it.
* PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
*/
#include <sdtdescriptor.h>
#include <helpers.h>

void SdtDescriptor::print(const int indent) const {
  printIndent(indent);
  printf("Service type: " DEC_HEX "- %s\n", DEC_HEX_V(serviceType), getServiceTypeName(serviceType));
  
  printIndent(indent);
  printf("Provider name: %s\n", providerName.c_str());
  
  printIndent(indent);
  printf("Service name: %s\n", serviceName.c_str());
  
}

//https://www.etsi.org/deliver/etsi_en/300400_300499/300468/01.10.01_40/en_300468v011001o.pdf
const char *SdtDescriptor::getServiceTypeName(uint8_t type) const {
  switch (type) {
    case 0x01: return "Digital television service";
    case 0x02: return "Digital radio sound service";
    case 0x03: return "Teletext service";
    case 0x04: return "NVOD reference service";
    case 0x05: return "NVOD time-shifted service";
    case 0x06: return "Mosaic service";
    case 0x07: return "FM radio service";
    case 0x08: return "DVB SRM service";
    case 0x0A: return "Advanced codec digital radio service";
    case 0x0B: return "Advanced codec mosaic service";
    case 0x0C: return "Data broadcasr service";
    case 0x0E: return "RCS MAP";
    case 0x0F: return "RCS FLS";
    case 0x10: return "DVB MHP service";
    case 0x11: return "MPEG-2 HD digital television service";
    case 0x16: return "Advanced codec SD digital television service";
    case 0x17: return "Advanced codec SD NVOD time-shifted service";
    case 0x18: return "Advanced codec SD NVOD reference service";
    case 0x19: return "Advanced codec HD digital television service";
    case 0x1A: return "Advanced codec HD NVOD time-shifted service";
    case 0x1B: return "Advanced codec HD NVOD reference service";
  }
  if (type >= 0x80 && type <= 0xfe) return "User defined";
  
  return "Reserved";
}