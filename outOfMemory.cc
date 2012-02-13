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

#include "base.h"
#include "threads.h"


/* Global static data */
typedef struct {
  jboolean vmDeathCalled;
  jboolean dumpInProgress;
  jrawMonitorID lock;
  int totalCount;

  char *optionsCopy;
  int retainedSizeClassCount;
  char **retainedSizeClasses;

} GlobalData;
static GlobalData globalData, *gdata = &globalData;


/* Typedef to hold class details */
typedef struct {
  jclass klass;
  char *signature;
  int count;
  int referLevelCount[4];
  int space;
} ClassDetails;

#define REFER_DEPTH 3


/* Test if the given haystack ends with the given needle + skipHaystackChars ignored characters. */
static bool endswith(char *haystack, char *needle, int skipHaystackChars) {
  int hLen = strlen(haystack);
  int nLen = strlen(needle);

  int offset = hLen - nLen - skipHaystackChars;
  if (offset < 0) {
    return false;
  }

  for (int i = nLen - 1; i >= 0; i--) {
    if (haystack[i + offset] != needle[i]) {
      return false;
    }
  }

  return true;
}


/* Enter agent monitor protected section */
static void enterAgentMonitor(jvmtiEnv *jvmti) {
  CHECK(jvmti->RawMonitorEnter(gdata->lock));
}


/* Exit agent monitor protected section */
static void exitAgentMonitor(jvmtiEnv *jvmti) {
  CHECK(jvmti->RawMonitorExit(gdata->lock));
}


/* IterateThroughHeap callback that finds referrers to a given class. */
static jint JNICALL referenceFinder(
    jvmtiHeapReferenceKind reference_kind,
    const jvmtiHeapReferenceInfo* reference_info,
    jlong class_tag,
    jlong referrer_class_tag,
    jlong size,
    jlong* tag_ptr,
    jlong* referrer_tag_ptr,
    jint length,
    void* user_data) {
  if (referrer_tag_ptr) {
    if (class_tag == (jlong) user_data) {
      *referrer_tag_ptr = 1;
    }
  }
  return JVMTI_VISIT_OBJECTS;
}


/* IterateThroughHeap callback that finds shortest path from an object to an object of the given class. */
static jint JNICALL referenceDepthCounter(
    jvmtiHeapReferenceKind reference_kind,
    const jvmtiHeapReferenceInfo* reference_info,
    jlong class_tag,
    jlong referrer_class_tag,
    jlong size,
    jlong* tag_ptr,
    jlong* referrer_tag_ptr,
    jint length,
    void* user_data) {
  if (referrer_tag_ptr) {
    if (*tag_ptr) {
      if (*referrer_tag_ptr == 0 || (*tag_ptr + 1) < *referrer_tag_ptr) {
        *referrer_tag_ptr = *tag_ptr + 1;
      }
    }
  }
  return JVMTI_VISIT_OBJECTS;
}


/* Aggregates reference depth tags by class. */
static jint JNICALL referenceDepthAggregator(
    jlong class_tag,
    jlong size,
    jlong* tag_ptr,
    jint length,
    void* user_data) {
  if (tag_ptr && *tag_ptr && *tag_ptr <= REFER_DEPTH) {
    ClassDetails *d = (ClassDetails*)(void*)(ptrdiff_t)class_tag;
    d->referLevelCount[*tag_ptr - 1] += 1;
  }

  return JVMTI_VISIT_OBJECTS;
}


/* Set up pointers / tag relationships for the given ClassDetails objects. */
static void setTagPointers(jvmtiEnv *jvmti, ClassDetails *details, int classCount) {
  /* Ensure classes are tagged correctly. */
  for (int i = 0 ; i < classCount; i++) {
    /* Tag this jclass */
    CHECK(jvmti->SetTag(details[i].klass, (jlong)(ptrdiff_t)(void *)(&details[i])));
  }
}


/* IterateThroughHeap callback that clears tags. */
static jint JNICALL clearTag(jlong class_tag, jlong size, jlong* tag_ptr, jint length, void* user_data) {
  *tag_ptr = 0;
  return JVMTI_VISIT_OBJECTS;
}


/* Clear all tags. */
static void clearTags(jvmtiEnv *jvmti) {
  jvmtiHeapCallbacks callbacks;
  memset(&callbacks, 0, sizeof(callbacks));

  callbacks.heap_iteration_callback = clearTag;
  CHECK(jvmti->IterateThroughHeap((jint) 0, (jclass) 0, &callbacks, (void *) NULL));
}


/* IterateThroughHeap callback that sets a tag to 1. */
static jint JNICALL setTag(jlong class_tag, jlong size, jlong* tag_ptr, jint length, void* user_data) {
  *tag_ptr = 1;
  return JVMTI_VISIT_OBJECTS;
}


/* IterateThroughHeap callback that marks all elements of the given class. */
static void mark(jvmtiEnv *jvmti, jclass klass) {
  jvmtiHeapCallbacks callbacks;
  memset(&callbacks, 0, sizeof(callbacks));
  callbacks.heap_iteration_callback = setTag;
  CHECK(jvmti->IterateThroughHeap((jint) 0, klass, &callbacks, (void *) NULL));
}


/* IterateThroughHeap callback that propogates marking from one set of objects to all referenced objects. */
static jint JNICALL markReferences(
    jvmtiHeapReferenceKind reference_kind,
    const jvmtiHeapReferenceInfo* reference_info,
    jlong class_tag,
    jlong referrer_class_tag,
    jlong size,
    jlong* tag_ptr,
    jlong* referrer_tag_ptr,
    jint length,
    void* user_data) {
  if (referrer_tag_ptr && *referrer_tag_ptr && *tag_ptr == 0) {
    *tag_ptr = 1;
  }
  return JVMTI_VISIT_OBJECTS;
}


/* IterateThroughHeap callback that accumulates sizes of marked objects. */
static jint JNICALL addSizes(jlong class_tag, jlong size, jlong* tag_ptr, jint length, void* user_data) {
  *((long *) user_data) += size;
  return JVMTI_VISIT_OBJECTS;
}


/* Gets the retained size across all instances of a given class and all objects referenced by those objects. */
static long getRetainedSize(jvmtiEnv *jvmti, jclass klass) {
  clearTags(jvmti);

  mark(jvmti, klass);

  jvmtiHeapCallbacks callbacks;
  memset(&callbacks, 0, sizeof(callbacks));
  callbacks.heap_reference_callback = markReferences;

  CHECK(jvmti->FollowReferences(0, NULL, NULL, &callbacks, 0));

  long result = 0;
  callbacks.heap_iteration_callback = addSizes;
  CHECK(jvmti->IterateThroughHeap(JVMTI_HEAP_FILTER_UNTAGGED, 0, &callbacks, (void *)(&result)));

  return result;
}


/* Prints a referrer summary. */
static void printRefererSummary(jvmtiEnv *jvmti, FILE *out, ClassDetails* details, int classCount, int offset) {
  jvmtiHeapCallbacks callbacks;
  memset(&callbacks, 0, sizeof(callbacks));

  clearTags(jvmti);
  setTagPointers(jvmti, details, classCount);

  fprintf(out, "Collecting references... ");
  callbacks.heap_reference_callback = referenceFinder;
  CHECK(jvmti->FollowReferences(0, NULL, NULL, &callbacks, (void *)(&details[offset])));

  fprintf(out, "Finding references to references... ");
  callbacks.heap_reference_callback = referenceDepthCounter;
  for (int i = 0; i < REFER_DEPTH - 1; i++) {
    CHECK(jvmti->FollowReferences(0, NULL, NULL, &callbacks, 0));
  }

  fprintf(out, "Aggregating references... ");
  setTagPointers(jvmti, details, classCount);
  callbacks.heap_iteration_callback = referenceDepthAggregator;
  CHECK(jvmti->IterateThroughHeap((jint) 0, (jclass) 0, &callbacks, (void *) NULL));

  fprintf(out, "\n");
  for (int level = 0; level < REFER_DEPTH; level++) {
    fprintf(out, "\t\tLevel %d referrers:\n", level + 1);
    for (int j = 0 ; j < classCount; j++) {
      int count = details[j].referLevelCount[level];
      if (count) {
        fprintf(out, "\t\t%10d %s\n", count, details[j].signature);
        details[j].referLevelCount[level] = 0;
      }
    }
    fprintf(out, "\n");
  }

  clearTags(jvmti);
}


/* IterateThroughHeap callback that aggregates counts and sizes by class. */
static jvmtiIterationControl JNICALL heapObject(jlong class_tag, jlong size, jlong* tag_ptr, void* user_data) {
  if (class_tag != (jlong)0) {
    ClassDetails *d = (ClassDetails*)(void*)(ptrdiff_t)class_tag;
    gdata->totalCount++;
    d->count++;
    d->space += size;
  }
  return JVMTI_ITERATION_CONTINUE;
}


/* Comparison function for two ClassDetails - used to sort largest size first. */
static int compareDetails(const void *p1, const void *p2) {
  return ((ClassDetails*)p2)->space - ((ClassDetails*)p1)->space;
}


/* Prints a heap histogram. */
static void JNICALL printHistogram(jvmtiEnv *jvmti, FILE *out) {
  if (!gdata->vmDeathCalled && !gdata->dumpInProgress) {
    jvmtiError    err;
    void         *user_data;
    jclass       *classes;
    jint          count;
    jint          i;
    ClassDetails *details;

    gdata->dumpInProgress = JNI_TRUE;
    gdata->totalCount = 0;

    /* Get all the loaded classes */
    CHECK(jvmti->GetLoadedClasses(&count, &classes));

    /* Setup an area to hold details about these classes */
    details = (ClassDetails*)calloc(sizeof(ClassDetails), count);
    CHECK_FOR_NULL(details);
    for (i = 0 ; i < count ; i++) {
      char *sig;

      /* Get and save the class signature */
      CHECK(jvmti->GetClassSignature(classes[i], &sig, NULL));
      CHECK_FOR_NULL(sig);
      details[i].signature = strdup(sig);
      deallocate(jvmti, sig);

      details[i].klass = classes[i];

      /* Tag this jclass */
      CHECK(jvmti->SetTag(classes[i], (jlong)(ptrdiff_t)(void*)(&details[i])));
    }

    /* Iterate over the heap and count up uses of jclass */
    CHECK(jvmti->IterateOverHeap(JVMTI_HEAP_OBJECT_EITHER, &heapObject, NULL));

    /* Remove tags */
    for (i = 0 ; i < count ; i++) {
      /* Un-Tag this jclass */
      CHECK(jvmti->SetTag(classes[i], (jlong)0));
    }

    /* Sort details by space used */
    qsort(details, count, sizeof(ClassDetails), &compareDetails);

    /* Print out sorted table */
    fprintf(out, "Heap View, Total of %d objects found.\n\n", gdata->totalCount);

    fprintf(out, "Space      Count      Retained   Class Signature\n");
    fprintf(out, "---------- ---------- ---------- ----------------------\n");

    for (i = 0 ; i < count ; i++) {
      if (details[i].space == 0) {
        break;
      }
      long retainedSize = 0;
      for (int j = 0; j < gdata->retainedSizeClassCount; j++) {
        if (endswith(details[i].signature, gdata->retainedSizeClasses[j], 1)) {
          retainedSize = getRetainedSize(jvmti, details[i].klass);
          break;
        }
      }
      fprintf(out, "%10d %10d %10ld %s\n", details[i].space, details[i].count, retainedSize, details[i].signature);
      if (i == 0) {
        printRefererSummary(jvmti, out, details, count, i);
      }
      fflush(out);
    }
    fprintf(out, "---------- ---------- ----------------------\n\n");
    fflush(out);

    /* Free up all allocated space */
    deallocate(jvmti, classes);
    for (i = 0 ; i < count ; i++) {
      if (details[i].signature != NULL) {
        free(details[i].signature);
      }
    }
    free(details);

    gdata->dumpInProgress = JNI_FALSE;
  }
}


/* Called when memory is exhausted. */
static void JNICALL resourceExhausted(
    jvmtiEnv *jvmti, JNIEnv* jni, jint flags, const void* reserved, const char* description) {
  if (flags & 0x0003) {
    enterAgentMonitor(jvmti); {
      FILE * out = fopen("/tmp/oom.log", "a");
      fprintf(out, "About to throw an OutOfMemory error.\n");

      fprintf(out, "Suspending all threads except the current one.\n");

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

      fprintf(out, "Printing a heap histogram.\n");

      printHistogram(jvmti, out);

      fprintf(out, "Resuming threads.\n");

      for (int i = 0; i < j; i++) {
        if (!errors[i]) {
          CHECK(jvmti->ResumeThread(threads[i]));
        }
      }
      deallocate(jvmti, threads);
      free(errors);

      fprintf(out, "Printing thread dump.\n");

      if (!gdata->vmDeathCalled) {
        printThreadDump(jvmti, jni, out, current);
      }
      fprintf(out, "\n\n");
      fclose(out);
    } exitAgentMonitor(jvmti);
  }
}


/* Callback to init the module. */
static void JNICALL vmInit(jvmtiEnv *jvmti, JNIEnv *env, jthread thread)
{
  enterAgentMonitor(jvmti); {
    jvmtiError err;

    CHECK(jvmti->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_DATA_DUMP_REQUEST, NULL));
  } exitAgentMonitor(jvmti);
}


/* Callback for JVM death. */
static void JNICALL vmDeath(jvmtiEnv *jvmti, JNIEnv *env) {
  jvmtiError          err;

  /* Disable events */
  enterAgentMonitor(jvmti); {
    CHECK(jvmti->SetEventNotificationMode(JVMTI_DISABLE, JVMTI_EVENT_RESOURCE_EXHAUSTED, NULL));

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
  callbacks.VMInit                  = &vmInit;
  callbacks.VMDeath                 = &vmDeath;
  callbacks.ResourceExhausted       = resourceExhausted;
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
