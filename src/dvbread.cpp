//#VERZE 2017

#include <stdio.h>
#include <linux/dvb/frontend.h>
#include <linux/dvb/dmx.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <getopt.h>
#include <time.h>
#include <stdint.h>

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

//Funkce hexdump pro zobrazení binárnách dat v HEXa formátu
// buf - data
// size - velikost zobrazovaných dat
void hexDump(__u8 *buf, int size) {
  int i;
  unsigned char ch;
  char sascii[17];
  
  sascii[16] = 0x0;
  
  for (i = 0; i < size; i++) {
    ch = buf[i];
    if (i % 16 == 0) {
      printf("%04x ", i);
    }
    printf("%02x ", ch);
    if (ch >= ' ' && ch <= '}')
      sascii[i % 16] = ch;
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

//Funkce se připojí na demuxer a získá z něj data požadovaného PID
// dedev - demux device /dev/dvb/adapter0/demux0
// data  - pole jenž bude naplněno daty
// size_data - velikost pole
// pid - požadovaný PID 
int read_pid(char *dedev, __u8 *data, int size_data, int pid) {
  int defd;
  
  if ((defd = open(dedev, O_RDWR | O_LARGEFILE)) < 0) {
    perror("opening demux failed");
    return 0;
  }
  
  #define TS_BUF_SIZE   (256 * 1024)       /* default DMX_Buffer Size for TS */
  long dmx_buffer_size = TS_BUF_SIZE;
  
  if (ioctl(defd, DMX_SET_BUFFER_SIZE, dmx_buffer_size) < 0) {
    perror("set demux filter failed");
    return 0;
  }
  
  struct dmx_sct_filter_params sctFilterParams;
  memset(&sctFilterParams, 0, sizeof(struct dmx_sct_filter_params));
  sctFilterParams.pid = pid;
  sctFilterParams.timeout = 10000; //10s
  sctFilterParams.flags = DMX_IMMEDIATE_START | DMX_CHECK_CRC;
  
  if (ioctl(defd, DMX_SET_FILTER, &sctFilterParams) < 0) {
    perror("set demux filter failed");
    return 0;
  }
  
  int len = read(defd, data, size_data);
  
  close(defd);
  
  return len;
}

#define DEMUXDEVICE "/dev/dvb/adapter%d/demux%d"
#define TS_PACKET_SIZE 188

#define ADAPTER 0
#define DEMUX   0

#define PAT_PID 0

int main(int argc, char **argv) {
  
  //do dedev se uloží název demuxu
  char dedev[128];
  snprintf(dedev, sizeof(dedev), DEMUXDEVICE, ADAPTER, DEMUX);
  
  //přečtou se data s PAT tabulkou
  __u8 pat_data[TS_PACKET_SIZE * 10];
  int len = read_pid(dedev, pat_data, sizeof(pat_data), PAT_PID);
  
  //co se přečetlo
  dump_hex(pat_data, len);
  
  
  
  //Dále se využívá knihovna libucsi pro parsování DVB tabulek
  
  //definici základních struktur pro tabulky
  struct section *section;
  struct section_ext *section_ext = NULL;
  
  //dekodování sekce
  if ((section = section_codec(pat_data, len)) == NULL) {
    perror("cant get section");
    return 1;
  }
  
  //pokud je PAT tabulka vypiš info z tabulky
  if (section->table_id == stag_mpeg_program_association) {
    struct mpeg_pat_section *pat;
    struct mpeg_pat_program *cur;
    
    //načíst hlavičku PAT tabulky
    if ((section_ext = section_ext_decode(section, 1)) == NULL) {
      perror("cant get section_ext");
      return 1;
    }
    
    //vypsat hlavičku PAT tabulky
    printf("SCT Decode PAT (pid:0x%04x) (table:0x%02x)\n", PAT_PID, section->table_id);
    
    //dekodovat PAT tabulku
    if ((pat = mpeg_pat_section_codec(section_ext)) == NULL) {
      fprintf(stderr, "SCT XXXX PAT section decode error\n");
      return 1;
    }
    
    //vypsat ID PAT tabulky
    printf("SCT transport_stream_id:0x%04x\n", mpeg_pat_section_transport_stream_id(pat));
    
    //modifikovaný cyklus for
    //cyklus projde všechny sece PAT tabulky a do proměné cur zadá info a aktuální sekci
    mpeg_pat_section_programs_for_each(pat, cur) {
      printf("\tSCT program_number:0x%04x pid:0x%04x\n", cur->program_number, cur->pid);
    }
    
  }
  return 0;
}






