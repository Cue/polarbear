/*
 * outOfMemory.cc
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

#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "jni.h"
#include "jvmti.h"

#include "agentthread.h"
#include "base.h"
#include "io.h"
#include "memory.h"
#include "shell.h"
#include "threads.h"


/* Called when memory is exhausted. */
static void JNICALL resourceExhausted(
    jvmtiEnv *jvmti, JNIEnv* jni, jint flags, const void* reserved, const char* description) {
  if (flags & 0x0003) {
    enterAgentMonitor(jvmti); {
      FILE * out = fopen("/tmp/oom.log", "a");
      FileOutput output(out);

      output.printf("About to throw an OutOfMemory error.\n");

      output.printf("Suspending all threads except the current one.\n");

      jthread current;
      CHECK(jvmti->GetCurrentThread(&current));

      jint threadCount;
      jthread *threads;
      CHECK(jvmti->GetAllThreads(&threadCount, &threads));

      int j = 0;
      for (int i = 0; i < threadCount; i++) {
        if (!jni->IsSameObject(threads[i], current)) {
          threads[j] = threads[i];
          j++;
        }
      }

      jvmtiError *errors = (jvmtiError *)calloc(sizeof(jvmtiError), j);
      CHECK(jvmti->SuspendThreadList(j, threads, errors));

      output.printf("Printing a heap histogram.\n");

      printHistogram(jvmti, &output, true);

      output.printf("Resuming threads.\n");

      for (int i = 0; i < j; i++) {
        if (!errors[i]) {
          CHECK(jvmti->ResumeThread(threads[i]));
        }
      }
      deallocate(jvmti, threads);
      free(errors);

      output.printf("Printing thread dump.\n");

      if (!gdata->vmDeathCalled) {
        printThreadDump(jvmti, jni, &output, current);
      }
      output.printf("\n\n");
      fclose(out);
    } exitAgentMonitor(jvmti);
  }
}


/* Callback to init the module. */
static void JNICALL vmInit(jvmtiEnv *jvmti, JNIEnv *env, jthread thread)
{
  enterAgentMonitor(jvmti); {
    jvmtiError err;

    createAgentThread(jvmti, env, shellServer, NULL);

    CHECK(jvmti->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_DATA_DUMP_REQUEST, NULL));
  } exitAgentMonitor(jvmti);
}


/* Callback for JVM death. */
static void JNICALL vmDeath(jvmtiEnv *jvmti, JNIEnv *env) {
  jvmtiError          err;

  /* Disable events */
  enterAgentMonitor(jvmti); {
    CHECK(jvmti->SetEventNotificationMode(JVMTI_DISABLE, JVMTI_EVENT_RESOURCE_EXHAUSTED, NULL));

    closeShellServer();

    gdata->vmDeathCalled = JNI_TRUE;
  } exitAgentMonitor(jvmti);
}


/* Called by the JVM to load the module. */
JNIEXPORT jint JNICALL Agent_OnLoad(JavaVM *vm, char *options, void *reserved) {
  jint rc;
  jvmtiError err;
  jvmtiCapabilities capabilities;
  jvmtiEventCallbacks callbacks;
  jvmtiEnv *jvmti;
  FILE *log;

  /* Build list of filter classes. */
  if (options && options[0]) {
    int len = strlen(options);
    int commaCount = 0;
    gdata->optionsCopy = strdup(options);
    for (int i = 0; i < len; i++) {
      if (options[i] == ',') {
        commaCount += 1;
      }
    }
    gdata->retainedSizeClassCount = commaCount + 1;
    if (commaCount == 0) {
      gdata->retainedSizeClasses = &gdata->optionsCopy;
    } else {
      gdata->retainedSizeClasses = (char **) calloc(sizeof(char *), gdata->retainedSizeClassCount);
      char *base = gdata->optionsCopy;
      int j = 0;
      for (int i = 0; i < len; i++) {
        if (gdata->optionsCopy[i] == ',') {
          gdata->optionsCopy[i] = 0;
          gdata->retainedSizeClasses[j] = base;
          base = gdata->optionsCopy + (i + 1);
          j++;
        }
      }
      gdata->retainedSizeClasses[j] = base;
    }
  } else {
    gdata->retainedSizeClassCount = 0;
  }

  log = fopen("/tmp/oom.log", "a");
  fprintf(log, "Initializing polarbear.\n\n");

  if (gdata->retainedSizeClassCount) {
    fprintf(log, "Performing retained size analysis for %d classes:\n", gdata->retainedSizeClassCount);
    for (int i = 0; i < gdata->retainedSizeClassCount; i++) {
      fprintf(log, "%s\n", gdata->retainedSizeClasses[i]);
    }
    fprintf(log, "\n");
  }

  /* Get JVMTI environment */
  jvmti = NULL;
  rc = vm->GetEnv((void **)&jvmti, JVMTI_VERSION);
  if (rc != JNI_OK) {
    fprintf(stderr, "ERROR: Unable to create jvmtiEnv, GetEnv failed, error=%d\n", rc);
    return -1;
  }
  CHECK_FOR_NULL(jvmti);

  /* Get/Add JVMTI capabilities */
  CHECK(jvmti->GetCapabilities(&capabilities));
  capabilities.can_tag_objects = 1;
  capabilities.can_generate_garbage_collection_events = 1;
  capabilities.can_get_source_file_name = 1;
  capabilities.can_get_line_numbers = 1;
  capabilities.can_suspend = 1;
  CHECK(jvmti->AddCapabilities(&capabilities));

  /* Create the raw monitor */
  CHECK(jvmti->CreateRawMonitor("agent lock", &(gdata->lock)));

  /* Set callbacks and enable event notifications */
  memset(&callbacks, 0, sizeof(callbacks));
  callbacks.VMInit = &vmInit;
  callbacks.VMDeath = &vmDeath;
  callbacks.ResourceExhausted = resourceExhausted;
  CHECK(jvmti->SetEventCallbacks(&callbacks, sizeof(callbacks)));
  CHECK(jvmti->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_VM_INIT, NULL));
  CHECK(jvmti->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_VM_DEATH, NULL));
  CHECK(jvmti->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_RESOURCE_EXHAUSTED, NULL));

  fclose(log);

  return 0;
}

/* Agent_OnUnload() is called last */
JNIEXPORT void JNICALL Agent_OnUnload(JavaVM *vm) {
}
