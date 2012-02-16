/*
 * shell.cc
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

#include <arpa/inet.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "base.h"
#include "io.h"
#include "memory.h"
#include "shell.h"
#include "threads.h"


static void interact(jvmtiEnv* jvmti, JNIEnv* jni, int socket);


// Starts the shell server.
void JNICALL shellServer(jvmtiEnv* jvmti, JNIEnv* jni, void *pData) {
  struct sockaddr_in serverInfo;

  // TCP stream oriented socket.
  gdata->activeShellSocket = -1;
  gdata->shellSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

  if (gdata->shellSocket == -1) {
    fprintf(stderr, "Could not create a socket for listening");
    return;
  }

  int optval = 1;
  setsockopt(gdata->shellSocket, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof optval);

  serverInfo.sin_family = AF_INET;
  serverInfo.sin_addr.s_addr = INADDR_ANY;
  serverInfo.sin_port = htons((short) 8787);

  // Bind the socket to our local server address
  int nret = bind(gdata->shellSocket, (struct sockaddr *)&serverInfo, sizeof(serverInfo));

  if (nret == -1) {
    fprintf(stderr, "Could not bind the control socket on port 8787.");
    return;
  }

  // Make the socket listen
  nret = listen(gdata->shellSocket, 1);
  if (nret == -1) {
    fprintf(stderr, "Error listening on socket on port 8787.");
    return;
  }

  int sockd = 0;
  while ((sockd = accept(gdata->shellSocket, NULL, NULL)) != -1) {
    interact(jvmti, jni, sockd);
  }

  enterAgentMonitor(jvmti); {
    closeShellServer();
  } exitAgentMonitor(jvmti);
}


void closeShellSession() {
  if (gdata->activeShellSocket != -1) {
    if (close(gdata->activeShellSocket) == -1) {
    fprintf(stderr, "Error closing active session.");
    }
    gdata->activeShellSocket = -1;
  }
}


void closeShellServer() {
  closeShellSession();
  if (gdata->shellSocket != -1) {
    if (close(gdata->shellSocket) == -1) {
    fprintf(stderr, "Error closing socket.");
    }
    gdata->shellSocket = -1;
  }
}



#define MAX_LINE 1000


ssize_t readline(int sockd, char *vptr, int maxlen) {
  int rc;
  char c, *buffer;

  buffer = vptr;

  int n;
  for (n = 1; n < maxlen; n++) {
    if ((rc = read(sockd, &c, 1)) == 1) {
      if (c == '\r') {
        n -= 1;
        continue;
      }
      if (c == '\n') {
        break;
      }
      *buffer++ = c;

    } else if (rc == 0) {
      if (n == 1) {
        return 0;
      } else {
        break;
      }

    } else {
      if (errno == EINTR) {
        continue;
      }
      return -1;
    }
  }

  *buffer = 0;
  return n;
}


ssize_t writestring(int sockd, const char *vptr, size_t n) {
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


static void interact(jvmtiEnv* jvmti, JNIEnv* jni, int socket) {
  char buffer[1000];
  SocketOutput out(socket);
  out.printf("Type 'help' to see a list of commands.\n");

  gdata->activeShellSocket = socket;

  while (1) {
    out.printf("> ");

    int length = readline(socket, buffer, MAX_LINE - 1);
    if (length < 0) {
      break;
    }
    if (strcmp("quit", buffer) == 0) {
      out.printf("Goodbye\n");
      break;
    }
    if (strcmp("help", buffer) == 0) {
      out.printf("Try any of the following:\n\n");
      out.printf("threads");
      out.printf("histogram");
      out.printf("gc");
      out.printf("stats <cls-signature>");
      out.printf("count <cls-signature>");
      out.printf("referrers <cls-signature>");

    } else if (strcmp("threads", buffer) == 0) {
      enterAgentMonitor(jvmti); {

        printThreadDump(jvmti, jni, &out, (jthread) 0);

      } exitAgentMonitor(jvmti);

    } else if (strcmp("histogram", buffer) == 0) {
      enterAgentMonitor(jvmti); {

        printHistogram(jvmti, &out, false);

      } exitAgentMonitor(jvmti);

    } else if (strncmp("count ", buffer, 6) == 0) {
      enterAgentMonitor(jvmti); {
        ThreadSuspension threads(jvmti, jni);

        out.printf("Computing count of '%s'\n\n", buffer + 6);
        printClassStats(jvmti, buffer + 6, &out, false);

      } exitAgentMonitor(jvmti);

    } else if (strncmp("stats ", buffer, 6) == 0) {
      enterAgentMonitor(jvmti); {
        ThreadSuspension threads(jvmti, jni);

        out.printf("Computing stats for '%s'\n\n", buffer + 6);
        printClassStats(jvmti, buffer + 6, &out, true);

      } exitAgentMonitor(jvmti);

    } else if (strncmp("referrers ", buffer, 10) == 0) {
      enterAgentMonitor(jvmti); {
        ThreadSuspension threads(jvmti, jni);

        out.printf("Computing stats for '%s'\n\n", buffer + 10);
        printReferrers(jvmti, buffer + 10, &out);

      } exitAgentMonitor(jvmti);


    } else if (strcmp("gc", buffer) == 0) {
      enterAgentMonitor(jvmti); {
        out.printf("Forcing garbage collection.\n");
        CHECK(jvmti->ForceGarbageCollection());
      } exitAgentMonitor(jvmti);

    } else {
      out.printf("Unknown command: '%s'\n", buffer);
    }
  }

  enterAgentMonitor(jvmti); {
    closeShellSession();
  } exitAgentMonitor(jvmti);
}
