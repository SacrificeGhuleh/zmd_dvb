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
  
  void printInfo() {
    printf("Path do device: %s\n", devicePath_.c_str());
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
      printf("  SCT program_number:0x%04x (%04d) pid:0x%04x\n", cur->program_number, cur->program_number, cur->pid);
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
    
    printf("  Network ID: %d  (0x%02x)\n", dvb_nit_section_network_id(nitSection), dvb_nit_section_network_id(nitSection));
    
    dvb_nit_section_descriptors_for_each(nitSection, desc) {
      printDescriptor(desc);
    }
    
    dvb_nit_section_transports_for_each(nitSection, nitSectionPart2, nitTransport) {
      printf("\tSCT transport_stream_id:0x%04x original_network_id:0x%04x\n", nitTransport->transport_stream_id, nitTransport->original_network_id);
      dvb_nit_transport_descriptors_for_each(nitTransport, desc) {
        printDescriptor(desc);
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
      printf("Service ID: %d\n", sdtService->service_id);
      dvb_sdt_service_descriptors_for_each(sdtService, desc) {
        printDescriptor(desc);
      }
    }
  }
  
  void parseSection(struct section *sec, const int pid) {
    if (sec == nullptr) {
      close(handle_);
      throw std::runtime_error("Section does not exist");
    }
    
    printf("Table ID: %d (0x%x)\n", sec->table_id, sec->table_id);
    
    // TODO probably switch
    
    if (sec->table_id == stag_mpeg_program_association) {
      parsePat(sec, pid);
      return;
    }
    
    if (sec->table_id == stag_dvb_service_description_actual || sec->table_id == stag_dvb_service_description_other) {
      parseSdt(sec, pid);
      return;
    }
    
    if (sec->table_id == stag_dvb_network_information_actual || sec->table_id == stag_dvb_network_information_other) {
//      printf("nit parsing not implemented yet\n");
      parseNit(sec, pid);
      return;
    }
    
  }
  
  void printDescriptor(struct descriptor *desc) {
    switch (desc->tag) {
      case dtag_dvb_service: {
        struct dvb_service_descriptor *serviceDescriptor;
        struct dvb_service_descriptor_part2 *serviceDescriptorPart2;
        
        serviceDescriptor = dvb_service_descriptor_codec(desc);
        if (serviceDescriptor != nullptr) {
          serviceDescriptorPart2 = dvb_service_descriptor_part2(serviceDescriptor);
          printf("  Service type: %d (0x%02x)\n", serviceDescriptor->service_type, serviceDescriptor->service_type);
          printf("  Provider name: %.*s\n",
                 serviceDescriptor->service_provider_name_length,
                 dvb_service_descriptor_service_provider_name(serviceDescriptor));
          printf("  Service name: %.*s\n",
                 serviceDescriptorPart2->service_name_length,
                 dvb_service_descriptor_service_name(serviceDescriptorPart2));
        }
        
        return;
      }
      default: {
        printf("WARNING: Printing of descriptor with type %d (0x%02x) not implemented\n", desc->tag, desc->tag);
        return;
      }
    }
  }
  
  ~DVBInfo() {
    printf("Destructor, closing handle\n");
    close(handle_);
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
};

int main(const int argc, const char **argv) {
  uint16_t adapterNr = 0;
  uint16_t demuxNr = 0;
  
  constexpr uint8_t patTablePid = 0x00U;
  constexpr uint8_t nitTablePid = 0x10U;
  constexpr uint8_t sdtTablePid = 0x11U;
  
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
  
  return 0;
}
