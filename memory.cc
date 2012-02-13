/*
 * memory.cc
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
#include "io.h"
#include "memory.h"


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
static void printRefererSummary(jvmtiEnv *jvmti, Output *out, ClassDetails* details, int classCount, int offset) {
  jvmtiHeapCallbacks callbacks;
  memset(&callbacks, 0, sizeof(callbacks));

  clearTags(jvmti);
  setTagPointers(jvmti, details, classCount);

  callbacks.heap_reference_callback = referenceFinder;
  CHECK(jvmti->FollowReferences(0, NULL, NULL, &callbacks, (void *)(&details[offset])));

  callbacks.heap_reference_callback = referenceDepthCounter;
  for (int i = 0; i < REFER_DEPTH - 1; i++) {
    CHECK(jvmti->FollowReferences(0, NULL, NULL, &callbacks, 0));
  }

  setTagPointers(jvmti, details, classCount);
  callbacks.heap_iteration_callback = referenceDepthAggregator;
  CHECK(jvmti->IterateThroughHeap((jint) 0, (jclass) 0, &callbacks, (void *) NULL));

  out->printf("\n");
  for (int level = 0; level < REFER_DEPTH; level++) {
    out->printf("\t\tLevel %d referrers:\n", level + 1);
    for (int j = 0 ; j < classCount; j++) {
      int count = details[j].referLevelCount[level];
      if (count) {
        out->printf("\t\t%10d %s\n", count, details[j].signature);
        details[j].referLevelCount[level] = 0;
      }
    }
    out->printf("\n");
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
void JNICALL printHistogram(jvmtiEnv *jvmti, Output *out, bool includeReferrers) {
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
    out->printf("Heap View, Total of %d objects found.\n\n", gdata->totalCount);

    out->printf("Space      Count      Retained   Class Signature\n");
    out->printf("---------- ---------- ---------- ----------------------\n");

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
      out->printf("%10d %10d %10ld %s\n", details[i].space, details[i].count, retainedSize, details[i].signature);
      if (i == 0 && includeReferrers) {
        printRefererSummary(jvmti, out, details, count, i);
      }
      out->flush();
    }
    out->printf("---------- ---------- ----------------------\n\n");
    out->flush();

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
