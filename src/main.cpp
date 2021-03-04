#include <cstdio>
#include <sstream>
#include <filesystem>
#include <cstdlib>
#include <cstring>
#include <cctype>
#include <getopt.h>
#include <ctime>
#include <cstdint>
#include <vector>
#include <map>

#include <linux/dvb/frontend.h>
#include <linux/dvb/dmx.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <libucsi/mpeg/descriptor.h>
#include <libucsi/mpeg/section.h>
#include <libucsi/dvb/descriptor.h>
#include <libucsi/dvb/section.h>
#include <libucsi/atsc/descriptor.h>
#include <libucsi/atsc/section.h>
#include <libucsi/transport_packet.h>
#include <libucsi/section_buf.h>
#include <libucsi/dvb/types.h>
#include <libdvbapi/dvbdemux.h>
#include <libdvbapi/dvbfe.h>
#include <libdvbcfg/dvbcfg_zapchannel.h>
#include <libdvbsec/dvbsec_api.h>
#include <libdvbsec/dvbsec_cfg.h>
#include <any>

#define DEC_HEX "%d (0x%02x)"
#define DEC_HEX_V(x) x,x

constexpr uint16_t patTablePid = 0x00U;
constexpr uint16_t nitTablePid = 0x10U;
constexpr uint16_t sdtTablePid = 0x11U;

bool isReservedPid(uint16_t pid) {
  return
      pid == patTablePid ||
      pid == nitTablePid ||
      pid == sdtTablePid;
}

/// \brief Displays data in hex format
/// \param buf Pointer to input data
/// \param size Size in bytes of data
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

template<typename T>
void hexDump(const T &buf) {
  printf("(dbg) Template instantitated hexdump\n");
  hexDump(buf, sizeof(T));
}

inline void printIndent(int indent) {
  if (indent > 0) {
    while (indent--) {
      printf("  ");
    }
  }
}

struct NetworkInfoDescriptor {
  std::vector<uint32_t> frequencies;
  std::string name;
  uint32_t centre_frequency;
  uint8_t bandwidth;
  uint8_t priority;
  uint8_t time_slicing_indicator;
  uint8_t mpe_fec_indicator;
  uint8_t constellation;
  uint8_t hierarchy_information;
  uint8_t code_rate_hp_stream;
  uint8_t code_rate_lp_stream;
  uint8_t guard_interval;
  uint8_t transmission_mode;
  uint8_t other_frequency_flag;
  
  void print(const int indent = 0) const {
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
};

struct SdtDescriptor {
  int serviceType = -1;
  std::string providerName;
  std::string serviceName;
  
  void print(const int indent = 0) const {
    printIndent(indent);
    printf("Service type: " DEC_HEX "\n", DEC_HEX_V(serviceType));
    
    printIndent(indent);
    printf("Provider name: %s\n", providerName.c_str());
    
    printIndent(indent);
    printf("Service name: %s\n", serviceName.c_str());
    
  }
};

struct Program {
  uint16_t pid;
  uint16_t programNumber;
  SdtDescriptor sdtDescriptor;
  
  void print(const int indent = 0) const {
    printIndent(indent);
    printf("Program number:" DEC_HEX "\n", DEC_HEX_V(programNumber));
    printIndent(indent + 1);
    printf("pid:" DEC_HEX "\n", DEC_HEX_V(pid));
    sdtDescriptor.print(indent + 1);
  }
};

class DVBInfo {
public:
  DVBInfo(const std::filesystem::path &path) : devicePath_(path), sec_(nullptr) {
    const bool exists = std::filesystem::exists(path);
    printf("Device \"%s\" exists: %s\n", path.c_str(), exists ? "true" : "fasle");
    
    if (!exists) {
      throw std::runtime_error("Device does not found, perhaps incorrect path set");
    }
    
    if ((handle_ = open(devicePath_.c_str(), O_RDWR | O_LARGEFILE)) < 0) {
      close(handle_);
      throw std::runtime_error("Opening demux failed");
    }
    
    if (ioctl(handle_, DMX_SET_BUFFER_SIZE, tsBufferSize) < 0) {
      close(handle_);
      throw std::runtime_error("Unable to set demux buffer size");
    }
    
    readBinBuffer.resize(tsPacketSize * 20);
  }
  
  int readPid(const int pid) {
    sec_ = nullptr;
    memset(readBinBuffer.data(), 0, readBinBuffer.size());
    printf("Requesting data from PID %d\n", pid);
    struct dmx_sct_filter_params sctFilterParams;
    memset(&sctFilterParams, 0, sizeof(struct dmx_sct_filter_params));
    sctFilterParams.pid = pid;
    sctFilterParams.timeout = 20000; //10s
    sctFilterParams.flags = DMX_IMMEDIATE_START | DMX_CHECK_CRC;
    
    if (ioctl(handle_, DMX_SET_FILTER, &sctFilterParams) < 0) {
//      close(handle_);
      printf("set demux filter failed\n");
//      std::runtime_error("set demux filter failed");
      return -1;
    }
    
    lastDataSize_ = read(handle_, readBinBuffer.data(), readBinBuffer.size());
    
    if (lastDataSize_ < 0) {
      if (errno == EOVERFLOW) {
        printf("Data overflow!\n");
        return -1;
      } else if (errno == EAGAIN) {
//        usleep(100);
        printf("Data should be readed again\n");
        return -1;
      } else {
//        close(handle_);
//        throw std::runtime_error("Read error\n");
        printf("Read error\n");
        return -1;
      }
    }
    
    printf("Read %d bytes of data from PID %d\n", lastDataSize_, pid);
    lastPid = pid;
    return lastDataSize_;
  }
  
  void dumpLastData() {
    if (lastDataSize_ > 0) {
      hexDump(readBinBuffer.data(), lastDataSize_);
    } else {
      printf("Dump failed, data missing\n");
    }
  }
  
  struct section *getLastSection() {
    sec_ = nullptr;
    if ((sec_ = section_codec(readBinBuffer.data(), lastDataSize_)) == nullptr) {
      close(handle_);
      throw std::runtime_error("Unable to get section");
    }
    return sec_;
  }
  
  
  void parseLastSection() {
    if (sec_ == nullptr) {
      getLastSection();
    }
    parseSection(sec_, lastPid);
  }
  
  void parsePat(struct section *sec, const int pid) {
    struct section_ext *secExtended = nullptr;
    struct mpeg_pat_section *pat;
    struct mpeg_pat_program *cur;
    
    //načíst hlavičku PAT tabulky
    if ((secExtended = section_ext_decode(sec, 1)) == nullptr) {
      close(handle_);
      throw std::runtime_error("cant get section_ext");
    }
    
    //vypsat hlavičku PAT tabulky
    printf("SCT Decode PAT (pid:0x%04x) (table:0x%02x)\n", pid, sec->table_id);
    
    //dekodovat PAT tabulku
    if ((pat = mpeg_pat_section_codec(secExtended)) == nullptr) {
      close(handle_);
      throw std::runtime_error("SCT XXXX PAT section decode error");
    }
    
    //vypsat ID PAT tabulky
    printf("SCT transport_stream_id:0x%04x\n", mpeg_pat_section_transport_stream_id(pat));
    
    //modifikovaný cyklus for
    //cyklus projde všechny sece PAT tabulky a do proměné cur zadá info a aktuální sekci
    mpeg_pat_section_programs_for_each(pat, cur) {
      printf("  SCT program_number:" DEC_HEX " pid:0x%04x\n", DEC_HEX_V(cur->program_number), cur->pid);
      if (isReservedPid(cur->pid)) continue;
      
      Program currentProgram;
      currentProgram.pid = cur->pid;
      currentProgram.programNumber = cur->program_number;
      programInfo_[cur->program_number] = (currentProgram);
    }
  }
  
  void parseNit(struct section *sec, const int pid) {
    struct section_ext *secExtended = nullptr;
    struct dvb_nit_section *nitSection = nullptr;
    struct dvb_nit_section_part2 *nitSectionPart2 = nullptr;
    struct dvb_nit_transport *nitTransport = nullptr;
    struct descriptor *desc = nullptr;
    
    if ((secExtended = section_ext_decode(sec, 1)) == nullptr) {
      close(handle_);
      throw std::runtime_error("cant get section_ext");
    }
    
    if ((nitSection = dvb_nit_section_codec(secExtended)) == nullptr) {
      close(handle_);
      throw std::runtime_error("Cant get nit section");
    }
    
    nitSectionPart2 = dvb_nit_section_part2(nitSection);

//    printf("  Network ID: %d  (0x%02x)\n", dvb_nit_section_network_id(nitSection), dvb_nit_section_network_id(nitSection));
    dvb_nit_section_descriptors_for_each(nitSection, desc) {
      parseDescriptor(desc, &infoDescriptor_);
    }
    
    dvb_nit_section_transports_for_each(nitSection, nitSectionPart2, nitTransport) {
//      printf("\tSCT transport_stream_id:0x%04x original_network_id:0x%04x\n", nitTransport->transport_stream_id, nitTransport->original_network_id);
      dvb_nit_transport_descriptors_for_each(nitTransport, desc) {
        parseDescriptor(desc, &infoDescriptor_);
      }
    }
    
  }
  
  void parseSdt(struct section *sec, const int pid) {
    struct section_ext *secExtended = nullptr;
    struct dvb_sdt_section *sdtSection = nullptr;
    struct dvb_sdt_service *sdtService = nullptr;
    struct descriptor *desc = nullptr;
    
    if ((secExtended = section_ext_decode(sec, 1)) == nullptr) {
      close(handle_);
      throw std::runtime_error("Cant get section_ext");
    }
    
    if ((sdtSection = dvb_sdt_section_codec(secExtended)) == nullptr) {
      close(handle_);
      throw std::runtime_error("Cant get sdt section");
    }
    dvb_sdt_section_services_for_each(sdtSection, sdtService) {
      dvb_sdt_service_descriptors_for_each(sdtService, desc) {
        if (desc->tag == dtag_dvb_service)
          parseDescriptor(desc, &programInfo_[sdtService->service_id].sdtDescriptor);
      }
    }
  }
  
  void parseProgramTable(struct section *sec, const int pid) {
    struct mpeg_pmt_section *mpegPmtSection;
    struct descriptor *desc;
    struct mpeg_pmt_stream *mpegPmtStream;
    struct section_ext *secExtended = nullptr;
    
    if ((secExtended = section_ext_decode(sec, 1)) == nullptr) {
      close(handle_);
      throw std::runtime_error("cant get section_ext");
    }
    
    printf("SCT Decode PMT (pid:0x%04x) (table:0x%02x)\n", pid, sec->table_id);
    
    if ((mpegPmtSection = mpeg_pmt_section_codec(secExtended)) == nullptr) {
      close(handle_);
      throw std::runtime_error("cant get mpeg pmt section");
    }
    
    uint8_t dummyNull = 0;
    printf("SCT program_number:0x%04x pcr_pid:0x%02x\n", mpeg_pmt_section_program_number(mpegPmtSection), mpegPmtSection->pcr_pid);
    mpeg_pmt_section_descriptors_for_each(mpegPmtSection, desc) {
      parseDescriptor(desc, &dummyNull);
    }
    
    mpeg_pmt_section_streams_for_each(mpegPmtSection, mpegPmtStream) {
      printf("\tSCT stream_type:0x%02x pid:0x%04x\n", mpegPmtStream->stream_type, mpegPmtStream->pid);
      mpeg_pmt_stream_descriptors_for_each(mpegPmtStream, desc) {
        parseDescriptor(desc, &dummyNull);
      }
    }
  }
  
  void parseSection(struct section *sec, const int pid) {
    if (sec == nullptr) {
      close(handle_);
      throw std::runtime_error("Section does not exist");
    }
    
    printf("Table ID: %d (0x%x)\n", sec->table_id, sec->table_id);
    if (sec->table_id == stag_mpeg_program_association) {
      parsePat(sec, pid);
      return;
    }
    
    if (sec->table_id == stag_dvb_service_description_actual || sec->table_id == stag_dvb_service_description_other) {
      parseSdt(sec, pid);
      return;
    }
    
    if (sec->table_id == stag_dvb_network_information_actual || sec->table_id == stag_dvb_network_information_other) {
      parseNit(sec, pid);
      return;
    }
    
    if (sec->table_id == stag_mpeg_program_map) {
      parseProgramTable(sec, pid);
      return;
    }
    
  }
  
  void parseDescriptor(struct descriptor *desc, std::any anyDescriptor) {
    switch (desc->tag) {
      case dtag_dvb_service: {
        auto *d = std::any_cast<SdtDescriptor *>(anyDescriptor);
        struct dvb_service_descriptor *serviceDescriptor;
        struct dvb_service_descriptor_part2 *serviceDescriptorPart2;
        
        serviceDescriptor = dvb_service_descriptor_codec(desc);
        if (serviceDescriptor != nullptr) {
          serviceDescriptorPart2 = dvb_service_descriptor_part2(serviceDescriptor);
          
          d->serviceName.resize(serviceDescriptorPart2->service_name_length);
          sprintf(d->serviceName.data(), "%.*s", serviceDescriptorPart2->service_name_length,
                  dvb_service_descriptor_service_name(serviceDescriptorPart2));
          
          d->providerName.resize(serviceDescriptor->service_provider_name_length);
          sprintf(d->providerName.data(), "%.*s", serviceDescriptor->service_provider_name_length,
                  dvb_service_descriptor_service_provider_name(serviceDescriptor));
          
          d->serviceType = serviceDescriptor->service_type;
        }
        
        return;
      }
      case dtag_dvb_terrestial_delivery_system: {
        auto *d = std::any_cast<NetworkInfoDescriptor *>(anyDescriptor);
        struct dvb_terrestrial_delivery_descriptor *tdDescriptor;
        tdDescriptor = dvb_terrestrial_delivery_descriptor_codec(desc);
        if (tdDescriptor != nullptr) {
          d->centre_frequency = tdDescriptor->centre_frequency;
          d->bandwidth = tdDescriptor->bandwidth;
          d->priority = tdDescriptor->priority;
          d->time_slicing_indicator = tdDescriptor->time_slicing_indicator;
          d->mpe_fec_indicator = tdDescriptor->mpe_fec_indicator;
          d->constellation = tdDescriptor->constellation;
          d->hierarchy_information = tdDescriptor->hierarchy_information;
          d->code_rate_hp_stream = tdDescriptor->code_rate_hp_stream;
          d->code_rate_lp_stream = tdDescriptor->code_rate_lp_stream;
          d->guard_interval = tdDescriptor->guard_interval;
          d->transmission_mode = tdDescriptor->transmission_mode;
          d->other_frequency_flag = tdDescriptor->other_frequency_flag;
        }
        return;
      }
      case dtag_dvb_frequency_list: {
        auto *d = std::any_cast<NetworkInfoDescriptor *>(anyDescriptor);
        struct dvb_frequency_list_descriptor *frequencyListDescriptor;
        frequencyListDescriptor = dvb_frequency_list_descriptor_codec(desc);
        if (frequencyListDescriptor != nullptr) {
          int count = dvb_frequency_list_descriptor_centre_frequencies_count(frequencyListDescriptor);
          uint32_t *freqs = dvb_frequency_list_descriptor_centre_frequencies(frequencyListDescriptor);
          d->frequencies.resize(count);
          for (int i = 0; i < count; i++) {
            d->frequencies.at(i) = freqs[i];
          }
        }
        return;
      }
      case dtag_dvb_network_name: {
        auto *d = std::any_cast<NetworkInfoDescriptor *>(anyDescriptor);
        struct dvb_network_name_descriptor *dvbNetworkNameDescriptor;
        dvbNetworkNameDescriptor = dvb_network_name_descriptor_codec(desc);
        if (dvbNetworkNameDescriptor != nullptr) {
          d->name.resize(dvb_network_name_descriptor_name_length(dvbNetworkNameDescriptor));
          sprintf(d->name.data(), "%.*s", dvb_network_name_descriptor_name_length(dvbNetworkNameDescriptor),
                  dvb_network_name_descriptor_name(dvbNetworkNameDescriptor));
        }
        return;
      }
      
      default: {
        printf("WARNING: Printing of descriptor with type " DEC_HEX " not implemented\n", DEC_HEX_V(desc->tag));
        return;
      }
    }
  }
  
  void printInfo() {
    printf("\n\n----------------------------------\n\n");
    printf("Path do device: %s\n", devicePath_.c_str());
    
    infoDescriptor_.print();
    
    printf("\nPrograms:\n");
    printPrograms(1);
  }
  
  void printPrograms(const int indent = 0) {
    for (const auto &[key, program] : programInfo_) {
      program.print(indent);
      printf("\n");
    }
  }
  
  ~DVBInfo() {
    printf("Destructor, closing handle\n");
    close(handle_);
  }
  
  const std::map<uint16_t, Program> &getProgramInfo() const {
    return programInfo_;
  }

private:
  struct section *sec_;
  static constexpr size_t tsPacketSize = TRANSPORT_PACKET_LENGTH;
  static constexpr size_t tsBufferSize = 256U * 1024U;
  std::vector<uint8_t> readBinBuffer;
  int lastDataSize_ = 0;
  int lastPid = -1;
  int handle_;
  const std::filesystem::path devicePath_;
  std::map<uint16_t, Program> programInfo_;
//  TdsDescriptor tsdDescriptor_;
  NetworkInfoDescriptor infoDescriptor_;
};

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
  
  dvbInfo.readPid(patTablePid);
  dvbInfo.dumpLastData();
  dvbInfo.parseLastSection();
  
  dvbInfo.readPid(nitTablePid);
  dvbInfo.dumpLastData();
  dvbInfo.parseLastSection();
  
  dvbInfo.readPid(sdtTablePid);
  dvbInfo.dumpLastData();
  dvbInfo.parseLastSection();
  
  const auto &programInfo = dvbInfo.getProgramInfo();
  for (const auto &[key, program] : programInfo) {
    dvbInfo.readPid(program.pid);
    dvbInfo.dumpLastData();
    dvbInfo.parseLastSection();
  }
  
  dvbInfo.printInfo();
  
  return 0;
}
