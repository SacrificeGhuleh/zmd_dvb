/*
* This is a personal academic project. Dear PVS-Studio, please check it.
* PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
*/
#include <dvbinfo.h>
#include <helpers.h>

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

DVBInfo::DVBInfo(const std::filesystem::path &path) : devicePath_(path), sec_(nullptr) {
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

int DVBInfo::readPid(const int pid) {
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

void DVBInfo::dumpLastData() {
  if (lastDataSize_ > 0) {
    hexDump(readBinBuffer.data(), lastDataSize_);
  } else {
    printf("Dump failed, data missing\n");
  }
}

struct section *DVBInfo::getLastSection() {
  sec_ = nullptr;
  if ((sec_ = section_codec(readBinBuffer.data(), lastDataSize_)) == nullptr) {
    close(handle_);
    throw std::runtime_error("Unable to get section");
  }
  return sec_;
}

void DVBInfo::parseLastSection() {
  if (sec_ == nullptr) {
    getLastSection();
  }
  parseSection(sec_, lastPid);
}

void DVBInfo::parsePat(struct section *sec, const int pid) {
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
    printf("  SCT program_number:"
           DEC_HEX
           " pid:0x%04x\n", DEC_HEX_V(cur->program_number), cur->pid);
    if (isReservedPid(cur->pid)) continue;
    
    Program currentProgram;
    currentProgram.pid = cur->pid;
    currentProgram.programNumber = cur->program_number;
    programInfo_[cur->program_number] = (currentProgram);
  }
}

void DVBInfo::parseNit(struct section *sec, const int pid) {
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

void DVBInfo::parseSdt(struct section *sec, const int pid) {
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

void DVBInfo::parseProgramTable(struct section *sec, const int pid) {
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
  
  
  uint16_t programPid = mpeg_pmt_section_program_number(mpegPmtSection);
  printf("SCT program_number:0x%04x pcr_pid:0x%02x\n", programPid, mpegPmtSection->pcr_pid);
  mpeg_pmt_section_descriptors_for_each(mpegPmtSection, desc) {
    std::string str;
    parseDescriptor(desc, &str);
  }
  
  mpeg_pmt_section_streams_for_each(mpegPmtSection, mpegPmtStream) {
    printf("\tSCT stream_type:0x%02x pid:0x%04x\n", mpegPmtStream->stream_type, mpegPmtStream->pid);
    mpeg_pmt_stream_descriptors_for_each(mpegPmtStream, desc) {
      switch (desc->tag) {
        case dtag_mpeg_video_stream: {
          MpegVideoDescriptor mpegVideoDescriptor;
          parseDescriptor(desc, &mpegVideoDescriptor);
          programInfo_[programPid].mpegVideoStreams.emplace_back(mpegVideoDescriptor);
          break;
        }
        case dtag_mpeg_audio_stream: {
          MpegAudioDescriptor mpegAudioDescriptor;
          parseDescriptor(desc, &mpegAudioDescriptor);
          programInfo_[programPid].mpegAudioSTreams.emplace_back(mpegAudioDescriptor);
          break;
        }
        case dtag_atsc_ac3_audio: {
          Ac3AudioDescriptor ac3AudioDescriptor;
          parseDescriptor(desc, &ac3AudioDescriptor);
          programInfo_[programPid].ac3AudioStreams.emplace_back(ac3AudioDescriptor);
          break;
        }
        default: {
          std::string str;
          parseDescriptor(desc, &str);
          if (!str.empty()) {
            programInfo_[programPid].services.emplace_back(str);
          }
        }
      }
      
    }
  }
}

void DVBInfo::parseSection(struct section *sec, const int pid) {
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

void DVBInfo::parseDescriptor(struct descriptor *desc, std::any anyDescriptor) {
  switch (desc->tag) {
    case dtag_dvb_service: {
      auto *d = std::any_cast<SdtDescriptor *>(anyDescriptor);
      struct dvb_service_descriptor *serviceDescriptor = dvb_service_descriptor_codec(desc);
      
      struct dvb_service_descriptor_part2 *serviceDescriptorPart2;
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
      struct dvb_terrestrial_delivery_descriptor *tdDescriptor = dvb_terrestrial_delivery_descriptor_codec(desc);
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
      struct dvb_frequency_list_descriptor *frequencyListDescriptor = dvb_frequency_list_descriptor_codec(desc);
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
      struct dvb_network_name_descriptor *dvbNetworkNameDescriptor = dvb_network_name_descriptor_codec(desc);
      if (dvbNetworkNameDescriptor != nullptr) {
        d->name.resize(dvb_network_name_descriptor_name_length(dvbNetworkNameDescriptor));
        sprintf(d->name.data(), "%.*s", dvb_network_name_descriptor_name_length(dvbNetworkNameDescriptor),
                dvb_network_name_descriptor_name(dvbNetworkNameDescriptor));
      }
      return;
    }
    case dtag_dvb_teletext: {
      auto *d = std::any_cast<std::string *>(anyDescriptor);
      struct dvb_teletext_entry *teletextEntry;
      struct dvb_teletext_descriptor *teletextDescriptor = dvb_teletext_descriptor_codec(desc);
      
      if (teletextDescriptor != nullptr) {
        d->append("Teletext");
        printf("Teletext:\n");
        
        dvb_teletext_descriptor_entries_for_each(teletextDescriptor, teletextEntry) {
          printf("  Lang code: %.3s\n", teletextEntry->language_code);
          printf("  Type: %d\n", teletextEntry->type);
          printf("  Magazine nr: %d\n", teletextEntry->magazine_number);
          printf("  Page nr: %d\n", teletextEntry->page_number);
          printf(" ---\n");
        }
      }
      break;
    }
    case dtag_dvb_subtitling: {
      auto *d = std::any_cast<std::string *>(anyDescriptor);
      struct dvb_subtitling_entry *subtitlingEntry;
      struct dvb_subtitling_descriptor *subtitlingDescriptor = dvb_subtitling_descriptor_codec(desc);
      if (subtitlingDescriptor != nullptr) {
        d->append("SUbtitles:");
        printf("Subtitles:\n");
        dvb_subtitling_descriptor_subtitles_for_each(subtitlingDescriptor, subtitlingEntry) {
          d->append(" ");
          d->append(reinterpret_cast<char *>(&subtitlingEntry->language_code[0]), 3);
          
          printf("  Language code: %.3s\n", subtitlingEntry->language_code);
          printf("  type: %d\n", subtitlingEntry->subtitling_type);
          printf("  composition_page_id: %d\n", subtitlingEntry->composition_page_id);
          printf("  ancillary_page_id: %d\n", subtitlingEntry->ancillary_page_id);
          printf(" ---\n");
        }
      }
      break;
    }
    case dtag_mpeg_video_stream: {
      auto *d = std::any_cast<MpegVideoDescriptor *>(anyDescriptor);
      struct mpeg_video_stream_descriptor *videoStreamDescriptor = mpeg_video_stream_descriptor_codec(desc);
      if (videoStreamDescriptor != nullptr) {
        d->multiple_frame_rate_flag = videoStreamDescriptor->multiple_frame_rate_flag;
        d->frame_rate_code = videoStreamDescriptor->frame_rate_code;
        d->mpeg_1_only_flag = videoStreamDescriptor->mpeg_1_only_flag;
        d->constrained_parameter_flag = videoStreamDescriptor->constrained_parameter_flag;
        d->still_picture_flag = videoStreamDescriptor->still_picture_flag;
        d->hasExtra = !videoStreamDescriptor->mpeg_1_only_flag;
        if (d->hasExtra) {
          struct mpeg_video_stream_extra *videoSTreamExtra = mpeg_video_stream_descriptor_extra(videoStreamDescriptor);
          d->profile_and_level_indication = videoSTreamExtra->profile_and_level_indication;
          d->chroma_format = videoSTreamExtra->chroma_format;
          d->frame_rate_extension = videoSTreamExtra->frame_rate_extension;
        }
      }
      break;
    }
    case dtag_mpeg_audio_stream: {
      auto *d = std::any_cast<MpegAudioDescriptor *>(anyDescriptor);
      struct mpeg_audio_stream_descriptor *mpegAudioStreamDescriptor = mpeg_audio_stream_descriptor_codec(desc);
      if (mpegAudioStreamDescriptor != nullptr) {
        d->free_format_flag = mpegAudioStreamDescriptor->free_format_flag;
        d->id = mpegAudioStreamDescriptor->id;
        d->layer = mpegAudioStreamDescriptor->layer;
        d->variable_rate_audio_indicator = mpegAudioStreamDescriptor->variable_rate_audio_indicator;
      }
      break;
    }
    case dtag_atsc_ac3_audio: {
      auto *d = std::any_cast<Ac3AudioDescriptor *>(anyDescriptor);
      struct atsc_ac3_descriptor *ac3Descriptor = atsc_ac3_descriptor_codec(desc);
      if (ac3Descriptor != nullptr) {
        d->sample_rate_code = ac3Descriptor->sample_rate_code;
        d->bsid = ac3Descriptor->bsid;
        d->bit_rate_code = ac3Descriptor->bit_rate_code;
        d->surround_mode = ac3Descriptor->surround_mode;
        d->bsmod = ac3Descriptor->bsmod;
        d->num_channels = ac3Descriptor->num_channels;
        d->full_svc = ac3Descriptor->full_svc;
      }
      break;
    }
    default: {
//        printf("WARNING: Printing of descriptor with type " DEC_HEX " not implemented\n", DEC_HEX_V(desc->tag));
      return;
    }
  }
}

void DVBInfo::printInfo() {
  printf("\n\n----------------------------------\n\n");
  printf("Path do device: %s\n", devicePath_.c_str());
  
  infoDescriptor_.print();
  
  printf("\nPrograms:\n");
  printPrograms(1);
}

void DVBInfo::printPrograms(const int indent) {
  for (const auto &[key, program] : programInfo_) {
    program.print(indent);
    printf("\n");
  }
}

DVBInfo::~DVBInfo() {
  printf("Destructor, closing handle\n");
  close(handle_);
}

const std::map<uint16_t, Program> &DVBInfo::getProgramInfo() const {
  return programInfo_;
}

bool DVBInfo::isReservedPid(uint16_t pid) const {
  return
      pid == patTablePid ||
      pid == nitTablePid ||
      pid == sdtTablePid;
}