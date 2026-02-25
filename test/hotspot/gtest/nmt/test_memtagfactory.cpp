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

#include "nmt/memTagFactory.hpp"
#include "nmt/memTracker.hpp"
#include "nmt/nmtCommon.hpp"
#include "runtime/mutexLocker.hpp"
#include "unittest.hpp"
#include "utilities/rbTree.hpp"
#include "utilities/rbTree.inline.hpp"
#include "gtest/gtest.h"


TEST(NMTMemTagFactoryTest, CreatingSameTagTwiceGivesSameTag) {
  MemTagFactory::Instance mtf;
  MemTag t = mtf.tag("a");
  MemTag t2 = mtf.tag("a");
  EXPECT_EQ(t, t2);
}

TEST(NMTMemTagFactoryTest, CanStoreMaxNumberOfTags) {
  MemTagFactory::Instance mtf;
  int mnot = NMTUtil::max_number_of_tags() - mtf.number_of_tags();

  struct Cmp {
    static RBTreeOrdering cmp(MemTag a, MemTag b) {
      if (a < b) return RBTreeOrdering::LT;
      if (a > b) return RBTreeOrdering::GT;
      return RBTreeOrdering::EQ;
    }
  };
  RBTreeCHeap<MemTag, int, Cmp,  mtTest> set;
  char name[128];
  for (int i = 0; i < mnot; i++) {
    int _ignore = os::snprintf(name, sizeof(name), "tagnr%d", i);
    MemTag t = mtf.tag(name);
    int* p = set.find(t);
    EXPECT_EQ(nullptr, p);
    set.upsert(t , 1);
  }
  EXPECT_EQ((size_t)mnot, set.size());
}

TEST(NMTMemTagFactoryTest, TagOverflowReturnsMtOther) {
  MemTagFactory::Instance mtf;
  int mnot = NMTUtil::max_number_of_tags();
  char name[128];
  for (int i = 0; i < mnot; i++) {
    int _ignore = os::snprintf(name, sizeof(name), "tagnr%d", i);
    mtf.tag(name);
  }
  MemTag t =  mtf.tag("a random tag");
  MemTag t2 = mtf.tag("another random tag");
  EXPECT_EQ(t, t2);
  EXPECT_EQ(mtOther, t);
}

TEST(NMTMemTagFactoryTest, RegisterHRN) {
  MemTagFactory::Instance mtf;
  MemTag tag = mtf.tag("a");
  const char* name = "Hello World!";
  mtf.set_human_readable_name_of(tag, name);
  const char* ret_name = mtf.human_readable_name_of(tag);
  EXPECT_STREQ(name, ret_name);
}

TEST(NMTMemTagFactoryTest, TagNamesAreSaved) {
  const char* name = "abc";
  MemTagFactory::Instance mtf;
  MemTag tag = mtf.tag(name);
  EXPECT_STREQ(name, mtf.name_of(tag));
}

TEST(NMTMemTagFactoryTest, MissingTagsAreTaggedAsAbsent) {
  MemTagFactory::Instance mtf;
  MemTag mt = mtf.tag_or_absent("some random name");
  EXPECT_EQ(mt, MemTagFactory::AbsentTag);
}

TEST(NMTMemTagFactoryTest, EnumTagsHaveAHumanReadableName) {
  MemTagFactory::Instance mtf;
  auto test_it = [&](MemTag mem_tag, const char* hrn) {
    EXPECT_STREQ(hrn, mtf.human_readable_name_of(mem_tag));
  };
#define TEST_HRN(mem_tag, hrn) test_it(mem_tag, hrn);
  MEMORY_TAG_DO(TEST_HRN);
#undef TEST_HRN
}
