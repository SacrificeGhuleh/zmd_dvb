/*
* This is a personal academic project. Dear PVS-Studio, please check it.
* PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
*/
#ifndef DVBINFO_H
#define DVBINFO_H

#include <cstdint>
#include <any>
#include <map>
#include <filesystem>
#include <vector>

#include <libucsi/transport_packet.h>

#include <program.h>
#include <networkinfodescriptor.h>

class DVBInfo {
public:
  DVBInfo(const std::filesystem::path &path);
  
  ~DVBInfo();
  
  int readPid(const int pid);
  
  void dumpLastData();
  
  struct section *getLastSection();
  
  void parseLastSection();
  
  #pragma region "Parsing methods"
  
  void parsePat(struct section *sec, const int pid);
  
  void parseNit(struct section *sec, const int pid);
  
  void parseSdt(struct section *sec, const int pid);
  
  void parseProgramTable(struct section *sec, const int pid);
  
  void parseSection(struct section *sec, const int pid);
  
  void parseDescriptor(struct descriptor *desc, std::any anyDescriptor);
  
  #pragma endregion
  
  #pragma region "Printing methods"
  
  void printInfo();
  
  void printPrograms(const int indent = 0);
  
  #pragma endregion
  
  const std::map<uint16_t, Program> &getProgramInfo() const;
  
  bool isReservedPid(uint16_t pid) const;
  
  constexpr static uint16_t patTablePid = 0x00U;
  constexpr static uint16_t nitTablePid = 0x10U;
  constexpr static uint16_t sdtTablePid = 0x11U;

private:
  struct section *sec_;
  static constexpr size_t tsPacketSize = TRANSPORT_PACKET_LENGTH;
  static constexpr size_t tsBufferSize = 256U * 1024U;
  std::vector<uint8_t> readBinBuffer;
  int lastDataSize_ = 0;
  uint16_t lastPid = -1;
  int handle_;
  const std::filesystem::path devicePath_;
  std::map<uint16_t, Program> programInfo_;
  NetworkInfoDescriptor infoDescriptor_;
};


#endif //DVBINFO_H
