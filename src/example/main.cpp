/*
* This is a personal academic project. Dear PVS-Studio, please check it.
* PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
*/
#include <cstdint>
#include <filesystem>
#include <ostream>
#include <dvbinfo.h>

int main(const int argc, const char **argv) {
  uint16_t adapterNr = 0;
  uint16_t demuxNr = 0;
  
  const char *defaultPath = "/dev/dvb";
  std::stringstream dvbDeviceName;
  
  dvbDeviceName << defaultPath
                << "/adapter"
                << adapterNr
                << "/demux"
                << demuxNr;
  
  std::filesystem::path dvbDevicePath(dvbDeviceName.str());
  
  DVBInfo dvbInfo(dvbDevicePath);
  
  dvbInfo.readPid(DVBInfo::patTablePid);
  dvbInfo.dumpLastData();
  dvbInfo.parseLastSection();
  
  dvbInfo.readPid(DVBInfo::nitTablePid);
  dvbInfo.dumpLastData();
  dvbInfo.parseLastSection();
  
  dvbInfo.readPid(DVBInfo::sdtTablePid);
  dvbInfo.dumpLastData();
  dvbInfo.parseLastSection();
  
  const auto &programInfo = dvbInfo.getProgramInfo();
  for (const auto &[key, program] : programInfo) {
    dvbInfo.readPid(program.pid);
//    dvbInfo.dumpLastData();
    dvbInfo.parseLastSection();
  }
  
  dvbInfo.printInfo();
  
  return 0;
}

