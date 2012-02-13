/*
 * io.cc
 *
 * Original source is Copyright (c) 2011 The PolarBear Authors. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "io.h"


int FileOutput::printf(const char *msg, ...) {
  va_list argList;
  va_start(argList, msg);

  int result = vfprintf(this->f, msg, argList);

  va_end(argList);

  return result;
}


void FileOutput::flush() {
  fflush(this->f);
}


int writestring(int sockd, const char *vptr, int n) {
  int nwritten;
  const char *buffer = vptr;
  int nleft = n;

  while (nleft > 0) {
    if ((nwritten = write(sockd, buffer, nleft)) <= 0) {
      if (errno == EINTR) {
        nwritten = 0;
      } else {
        return -1;
      }
    }
    nleft  -= nwritten;
    buffer += nwritten;
  }

  return n;
}


int SocketOutput::printf(const char *msg, ...) {
  char buffer[1000];

  va_list argList;
  va_start(argList, msg);

  int len = vsprintf(buffer, msg, argList);

  va_end(argList);

  return writestring(this->socket, buffer, len);
}


void SocketOutput::flush() {
  // Do nothing for now.
}
