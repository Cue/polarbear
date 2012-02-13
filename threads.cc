/*
 * threads.cc
 *
 * Originally based on http://hope.nyc.ny.us/~lprimak/java/demo/jvmti/heapViewer/
 *
 * Original source is Copyright (c) 2004 Sun Microsystems, Inc. All Rights Reserved.
 *
 * Modifications are Copyright (c) 2011 The PolarBear Authors. All Rights Reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * -Redistribution of source code must retain the above copyright notice, this
 *  list of conditions and the following disclaimer.
 *
 * -Redistribution in binary form must reproduce the above copyright notice,
 *  this list of conditions and the following disclaimer in the documentation
 *  and/or other materials provided with the distribution.
 *
 * Neither the name of Sun Microsystems, Inc. or the names of contributors may
 * be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * This software is provided "AS IS," without a warranty of any kind. ALL
 * EXPRESS OR IMPLIED CONDITIONS, REPRESENTATIONS AND WARRANTIES, INCLUDING
 * ANY IMPLIED WARRANTY OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE
 * OR NON-INFRINGEMENT, ARE HEREBY EXCLUDED. SUN MIDROSYSTEMS, INC. ("SUN")
 * AND ITS LICENSORS SHALL NOT BE LIABLE FOR ANY DAMAGES SUFFERED BY LICENSEE
 * AS A RESULT OF USING, MODIFYING OR DISTRIBUTING THIS SOFTWARE OR ITS
 * DERIVATIVES. IN NO EVENT WILL SUN OR ITS LICENSORS BE LIABLE FOR ANY LOST
 * REVENUE, PROFIT OR DATA, OR FOR DIRECT, INDIRECT, SPECIAL, CONSEQUENTIAL,
 * INCIDENTAL OR PUNITIVE DAMAGES, HOWEVER CAUSED AND REGARDLESS OF THE THEORY
 * OF LIABILITY, ARISING OUT OF THE USE OF OR INABILITY TO USE THIS SOFTWARE,
 * EVEN IF SUN HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.
 *
 * You acknowledge that this software is not designed, licensed or intended
 * for use in the design, construction, operation or maintenance of any
 * nuclear facility.
 */

#include <stdlib.h>
#include <string.h>

#include "base.h"
#include "threads.h"


/* Prints a thread frame. */
static void JNICALL printFrame(jvmtiEnv* jvmti, jvmtiFrameInfo frame, FILE *out) {
  jvmtiError err;
  char *methodName, *className, *cleanClassName;
  char *fileName;
  int si, li, lineNumber;
  jclass declaringClass;
  jint locationCount;
  jvmtiLineNumberEntry* locationTable;

  CHECK(jvmti->GetMethodName(frame.method, &methodName, NULL, NULL));
  CHECK(jvmti->GetMethodDeclaringClass(frame.method, &declaringClass));
  CHECK(jvmti->GetClassSignature(declaringClass, &className, NULL));
  err = jvmti->GetSourceFileName(declaringClass, &fileName);
  if (err == JVMTI_ERROR_NATIVE_METHOD || err == JVMTI_ERROR_ABSENT_INFORMATION) {
    fileName = strdup("Unknown");

  } else {
    char *temp;

    CHECK(err);
    temp = strdup(fileName);
    deallocate(jvmti, fileName);
    fileName = temp;
  }
  err = jvmti->GetLineNumberTable(frame.method, &locationCount, &locationTable);
  if (err == JVMTI_ERROR_NATIVE_METHOD || err == JVMTI_ERROR_ABSENT_INFORMATION) {
    lineNumber = 0;
  } else {
    CHECK(err);
    lineNumber = 0;
    for (li = 0; li < locationCount; li++) {
      if (locationTable[li].start_location > frame.location) {
        break;
      }
      lineNumber = locationTable[li].line_number;
    }
    deallocate(jvmti, locationTable);
  }

  cleanClassName = strdup(className + 1);
  si = 0;
  while (cleanClassName[si]) {
    if (cleanClassName[si] == '/') {
      cleanClassName[si] = '.';
    } else if (cleanClassName[si] == ';') {
      cleanClassName[si] = '.';
    }
    si++;
  }

  if (lineNumber) {
    fprintf(out, "\tat %s%s(%s:%d)\n", cleanClassName, methodName, fileName, lineNumber);
  } else {
    fprintf(out, "\tat %s%s(%s)\n", cleanClassName, methodName, fileName);
  }
  deallocate(jvmti, methodName);
  deallocate(jvmti, className);
  free(fileName);
  free(cleanClassName);
}


/* Prints a thread dump. */
void JNICALL printThreadDump(jvmtiEnv *jvmti, JNIEnv *jni, FILE *out, jthread current) {
  jvmtiStackInfo *stack_info;
  jint thread_count;
  int ti;
  jvmtiError err;
  jvmtiThreadInfo threadInfo;

  fprintf(out, "\n");
  CHECK(jvmti->GetAllStackTraces(150, &stack_info, &thread_count));
  fprintf(out, "Dumping thread state for %d threads\n\n", thread_count);
  for (ti = 0; ti < thread_count; ++ti) {
    jvmtiStackInfo *infop = &stack_info[ti];
    jthread thread = infop->thread;
    jint state = infop->state;
    jvmtiFrameInfo *frames = infop->frame_buffer;
    int fi;
    const char *threadState;

    if (state & JVMTI_THREAD_STATE_SUSPENDED) {
      threadState = "SUSPENDED";
    } else if (state & JVMTI_THREAD_STATE_INTERRUPTED) {
      threadState = "INTERRUPTED";
    } else if (state & JVMTI_THREAD_STATE_IN_NATIVE) {
      threadState = "NATIVE";
    } else if (state & JVMTI_THREAD_STATE_RUNNABLE) {
      threadState = "RUNNABLE";
    } else if (state & JVMTI_THREAD_STATE_BLOCKED_ON_MONITOR_ENTER) {
      threadState = "BLOCKED";
    } else if (state & JVMTI_THREAD_STATE_IN_OBJECT_WAIT) {
      threadState = "WAITING";
    } else if (state & JVMTI_THREAD_STATE_PARKED) {
      threadState = "PARKED";
    } else if (state & JVMTI_THREAD_STATE_SLEEPING) {
      threadState = "SLEEPING";
    } else {
      threadState = "UNKNOWN";
    }

    jvmti->GetThreadInfo(thread, &threadInfo);
    fprintf(out, "#%d - %s - %s", ti + 1, threadInfo.name, threadState);
    if (thread == current || jni->IsSameObject(thread, current)) {
      fprintf(out, " - [OOM thrower]");
    }
    fprintf(out, "\n");
    deallocate(jvmti, threadInfo.name);

    for (fi = 0; fi < infop->frame_count; fi++) {
      printFrame(jvmti, frames[fi], out);
    }
    fprintf(out, "\n");
  }
  fprintf(out, "\n\n");
  /* this one Deallocate call frees all data allocated by GetAllStackTraces */
  deallocate(jvmti, stack_info);
}
