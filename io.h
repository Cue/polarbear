/*
 * io.h
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

#ifndef POLARBEAR_IO_H
#define POLARBEAR_IO_H

#include <stdlib.h>


class Output {
  public:
    virtual int printf(const char *msg, ...) = 0;
    virtual void flush() = 0;
};


class FileOutput : public Output {
  private:
    FILE *f;

  public:
    FileOutput(FILE *_f) : f(_f) {}

    virtual int printf(const char *msg, ...);
    virtual void flush();
};


class SocketOutput : public Output {
  private:
    int socket;

  public:
    SocketOutput(int _socket) : socket(_socket) {}

    virtual int printf(const char *msg, ...);
    virtual void flush();
};


#endif