/*
* This is a personal academic project. Dear PVS-Studio, please check it.
* PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
*/
#include <program.h>
#include <helpers.h>

void Program::print(const int indent) const {
  printIndent(indent);
  printf("Program number:"
         DEC_HEX
         "\n", DEC_HEX_V(programNumber));
  printIndent(indent + 1);
  printf("pid:"
         DEC_HEX
         "\n", DEC_HEX_V(pid));
  sdtDescriptor.print(indent + 1);
  
  printIndent(indent + 1);
  printf("Services: \n");
  if (!mpegVideoStreams.empty()) {
    printIndent(indent + 2);
    printf("Video: \n");
    for (const auto &videoStream : mpegVideoStreams) {
      videoStream.print(indent + 3);
    }
  }
  
  if (!(mpegAudioSTreams.empty() || ac3AudioStreams.empty())) {
    printIndent(indent + 2);
    printf("Audio: \n");
    for (const auto &audioStream : mpegAudioSTreams) {
      audioStream.print(indent + 3);
    }
    for (const auto &audioStream : ac3AudioStreams) {
      audioStream.print(indent + 3);
    }
  }
  
  if (!services.empty()) {
    printIndent(indent + 1);
    printf("Other Services:\n");
    for (const auto &service : services) {
      printIndent(indent + 2);
      printf("%s\n", service.c_str());
    }
  }
}