/*
 * Copyright (c) 2026, Oracle and/or its affiliates. All rights reserved.
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
 */

/*
 * @test
 * @library /test/lib
 * @modules java.base/jdk.internal.foreign
 * @run testng/othervm
 *     --enable-native-access=ALL-UNNAMED
 *     -XX:+UnlockDiagnosticVMOptions
 *     -XX:NativeMemoryTracking=summary
 *     -XX:+PrintNMTStatistics
 *     TestNativeMemoryTracking
 */

import jdk.internal.foreign.NativeMemoryTracking;
import jdk.internal.foreign.NativeMemoryTracking.Tag;
import java.lang.foreign.*;

import jdk.test.lib.process.ProcessTools;
import jdk.test.lib.process.OutputAnalyzer;


public class TestNativeMemoryTracking {
    // Unlikely to see this particular allocation size to be produced by any other
    // tag.
    static int lowLikelihoodAllocationSize = 971 * 563;

    private static void testMakingTags() throws Exception {
        var tag = NativeMemoryTracking.resolveName("TestNativeMemoryTrackingTagCreation");
        tag = NativeMemoryTracking.getAndSet(tag);
        try (Arena arena = Arena.ofConfined()) {
            MemorySegment segment = arena.allocate(lowLikelihoodAllocationSize);
        } finally {
            NativeMemoryTracking.getAndSet(tag);
        }
    }

    public static void main(String[] args) throws Exception {
        if (args.length == 1 && args[0].equals("testIt")) {
            testMakingTags();
            return;
        }
        ProcessBuilder pb = ProcessTools.createTestJavaProcessBuilder(
            "-XX:+UnlockDiagnosticVMOptions",
            "-XX:NativeMemoryTracking=summary",
            "-XX:+PrintNMTStatistics",
            "TestNativeMemoryTracking",
            "testIt"
        );
        OutputAnalyzer output = new OutputAnalyzer(pb.start());
        output.shouldHaveExitValue(0);
        output.shouldContain("TestNativeMemoryTrackingTagCreation");
        output.shouldContain(String.valueOf(lowLikelihoodAllocationSize));
    }
}
