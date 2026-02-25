/*
 * Copyright (c) 2025, Oracle and/or its affiliates. All rights reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 *
 * This code is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 only, as
 * published by the Free Software Foundation.
 *
 * This code is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * version 2 for more details (a copy is included in the LICENSE file that
 * accompanied this code).
 *
 * You should have received a copy of the GNU General Public License version
 * 2 along with this work; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Please contact Oracle, 500 Oracle Parkway, Redwood Shores, CA 94065 USA
 * or visit www.oracle.com if you need additional information or have any
 * questions.
 *
 */

#include "utilities/macros.hpp"

#if INCLUDE_JFR
#include "jfr/recorder/checkpoint/types/jfrType.hpp"
#endif

#include <nmt/memTagFactory.hpp>

DeferredStatic<MemTagFactory::Instance> MemTagFactory::_instance;
DeferredStatic<MemTagFactory::Iterator> MemTagFactory::_iterator;

uint32_t MemTagFactory::Instance::string_hash(const char* t) const {
  return AltHashing::halfsiphash_32(_seed, t, (int)strlen(t));
}

void MemTagFactory::Instance::put_if_absent(MemTag tag, const char* name) {
  assert(strlen(name) < 4096, "mustn't be ridiculously long");
  int bucket = string_hash(name) % _table_size;
  EntryRef link = _table[bucket];
  while (link != Nil) {
    Entry e = _entries.at(link);
    if (strcmp(e.name, name) == 0) {
      return;
    }
    link = e.next;
  }
  const char* name_copy = os::strdup(name, mtNMT);
  Entry nentry(tag, _table[bucket], name_copy);
  _entries.push(nentry);
  _table[bucket] = _entries.length() - 1;
  AtomicAccess::inc(&_number_of_tags);
}

MemTag MemTagFactory::Instance::tag_or_absent(const char* name) const {
  int bucket = string_hash(name) % _table_size;
  EntryRef link = _table[bucket];
  while (link != Nil) {
    Entry e = _entries.at(link);
    if (strcmp(e.name, name) == 0) {
      return e.tag;
    }
    link = e.next;
  }
  return AbsentTag;
}

MemTag MemTagFactory::Instance::tag(const char* name, const char* human_name) {
  MemTag found = tag_or_absent(name);
  if (found != AbsentTag) {
    return found;
  }
  if (std::numeric_limits<MemTagI>::max() == static_cast<MemTagI>(_number_of_tags)) {
    // Out of MemTags, revert to mtOther
    return mtOther;
  }

  // No tag found, have to create a new one
  MemTag i = static_cast<MemTag>(_number_of_tags);
  put_if_absent(i, name);
  if (human_name != nullptr) {
    set_human_readable_name_of(i, human_name);
  }
  // Register type with JFR
  JFR_ONLY(NMTTypeConstant::register_single_type(i, name);)
  return i;
}

const char* MemTagFactory::Instance::name_of(MemTag tag) const {
  if (index(tag) >= AtomicAccess::load(&_number_of_tags)) {
    return nullptr;
  }
  return _entries.at(index(tag)).name;
}

void MemTagFactory::Instance::set_human_readable_name_of(MemTag tag, const char* hrn) {
  MemTagI i = index(tag);
  const char* copy = os::strdup(hrn);
  const char*& ref = _human_readable_names.at_grow(i, nullptr);
  assert(ref == nullptr, "do not reassign human-readable names");
  ref = copy;
}

const char* MemTagFactory::Instance::human_readable_name_of(MemTag tag) const {
  MemTagI i = index(tag);
  if (i < _human_readable_names.length()) {
    return _human_readable_names.at(index(tag));
  }
  return nullptr;
}

MemTagFactory::Instance::Instance()
    : _entries(), _table_size(nr_of_buckets), _table(nullptr),
      _human_readable_names(), _seed(5000002429), _number_of_tags(0),
      _iterator(this) {
  _table = NEW_C_HEAP_ARRAY(EntryRef, _table_size, mtNMT);
  for (int i = 0; i < _table_size; i++) {
    _table[i] = Nil;
  }
#define MEMORY_TAG_ADD_TO_TABLE(mem_tag, human_readable) this->tag(#mem_tag, human_readable);
MEMORY_TAG_DO(MEMORY_TAG_ADD_TO_TABLE)
#undef MEMORY_TAG_ADD_TO_TABLE
}

int MemTagFactory::Instance::number_of_tags() const {
  return AtomicAccess::load(&_number_of_tags);
}

void MemTagFactory::Iterator::iterate_tags_impl(
    void (*callback)(void *ctx, const MemTag, const char *, const char *),
    void *ctx) {
  using MemTagI = std::underlying_type_t<MemTag>;
  NmtMemTagLocker ntml;
  for (MemTagI i = 0; i < (MemTagI)_instance->number_of_tags(); i++) {
    MemTag memtag = static_cast<MemTag>(i);
    callback(ctx, memtag, _instance->name_of(memtag),
             human_readable_name_of(memtag));
  }
}

void MemTagFactory::Instance::Iterator::iterate_tags_impl(
    void (*callback)(void *ctx, const MemTag, const char *, const char *),
                                                          void *ctx) {
  for (MemTagI i = 0; i < (MemTagI)_instance->number_of_tags(); i++) {
    MemTag memtag = static_cast<MemTag>(i);
    callback(ctx, memtag, _instance->name_of(memtag),
             _instance->human_readable_name_of(memtag));
  }
};
