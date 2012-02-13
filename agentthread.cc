/*
 * agentthread.cc
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


#include <jvmti.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "agentthread.h"
#include "base.h"


static jthread allocateThread(JNIEnv *env) {
  jclass thrClass = env->FindClass("java/lang/Thread");
  jmethodID cid = env->GetMethodID(thrClass, "<init>", "()V");
  return env->NewObject(thrClass, cid);
}

void createAgentThread(jvmtiEnv* jvmti, JNIEnv* env, jvmtiStartFunction proc, void *pArg) {
  CHECK(jvmti->RunAgentThread(allocateThread(env), proc, pArg, JVMTI_THREAD_NORM_PRIORITY));
}
