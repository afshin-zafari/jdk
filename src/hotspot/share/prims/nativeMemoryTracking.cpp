#include "jni.h"
#include "nmt/mallocHeader.hpp"
#include "nmt/mallocHeader.inline.hpp"
#include "nmt/memTagFactory.hpp"
#include "nmt/memTracker.hpp"
#include "runtime/interfaceSupport.inline.hpp"
#include "runtime/jniHandles.hpp"
#include "runtime/jniHandles.inline.hpp"

#include <limits>

#define CC (char*)  /*cast a literal from (const char*)*/
#define FN_PTR(f) CAST_FROM_FN_PTR(void*, &f)

JVM_ENTRY(jlong, NMT_makeTag(JNIEnv *env, jobject ignored_this, jobject tag_name_string)) {
  constexpr static int max_tagname_len = 1024;
  if (!MemTracker::enabled()) {
    return (jlong)mtNone;
  }
  Handle tnh(Thread::current(), JNIHandles::resolve(tag_name_string));
  oop oop = tnh();

  // The tag name must be a non-null instance of java.lang.String
  // whose length doesn't exceed max_tagname_len.
  if (oop == nullptr) {
    return -1;
  }
  if (!oop->is_a(vmClasses::String_klass())) {
    return -1;
  }
  typeArrayOop value = java_lang_String::value(oop);
  int length = java_lang_String::length(oop, value);
  if (length > max_tagname_len) {
    return -1;
  }

  // OK, fine.
  ResourceMark rm;
  const char* tag_name = java_lang_String::as_utf8_string(oop);
  MemTag tag = MemTagFactory::tag(tag_name);
  return (jlong)tag;
}
JVM_END

JVM_ENTRY(long, NMT_allocate0(JNIEnv *env, jobject ignored_this, jlong size, jlong mem_tag)) {
  if (size < 0) {
    return 0;
  }
  bool enabled = MemTracker::enabled();
  if (enabled && (mem_tag < 0 || MemTagFactory::number_of_tags() <= mem_tag)) {
    return 0;
  }

#ifndef _LP64
  assert(sizeof(jlong) > sizeof(size_t), "must be");
  if (size > static_cast<jlong>(std::numeric_limits<size_t>::max())) {
    return 0;
  }
#endif

  MemTag tag = enabled ? (MemTag)mem_tag : mtOther;
  size_t sz = (size_t)size;
  return (jlong)os::malloc(sz, tag);
}
JVM_END

JVM_ENTRY(bool, NMT_isNMTEnabled(JNIEnv *env, jobject ignored_this)) {
  return MemTracker::enabled();
}
JVM_END

static JNINativeMethod NMT_methods[] = {
  {CC "makeTag", CC "(Ljava/lang/String;)J", FN_PTR(NMT_makeTag)},
  {CC "allocate0", CC "(JJ)J", FN_PTR(NMT_allocate0)},
  {CC "isNMTEnabled", CC "()Z", FN_PTR(NMT_isNMTEnabled)}
};

JVM_ENTRY(void, JVM_RegisterNativeMemoryTrackingMethods(JNIEnv *env, jclass NMT_class)) {
  ThreadToNativeFromVM ttnfv(thread);

  int status = env->RegisterNatives(NMT_class, NMT_methods, sizeof(NMT_methods) / sizeof(JNINativeMethod));
  guarantee(status == JNI_OK && !env->ExceptionCheck(),
            "register java.lang.invoke.MethodHandleNative natives");
}
JVM_END
