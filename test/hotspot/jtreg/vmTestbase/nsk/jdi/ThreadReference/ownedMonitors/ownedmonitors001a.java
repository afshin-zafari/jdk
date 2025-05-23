/*
 * Copyright (c) 2001, 2025, Oracle and/or its affiliates. All rights reserved.
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

package nsk.jdi.ThreadReference.ownedMonitors;

import nsk.share.*;
import nsk.share.jpda.*;
import nsk.share.jdi.*;


/**
 * This class is used as debuggee application for the ownedmonitors001 JDI test.
 */

public class ownedmonitors001a {

    //----------------------------------------------------- templete section

    static final int PASSED = 0;
    static final int FAILED = 2;
    static final int PASS_BASE = 95;

    //--------------------------------------------------   log procedures

    static boolean verbMode = false;

    public static void log1(String message) {
        if (verbMode)
            System.err.println(Thread.currentThread().getName() + " : " + message);
    }
    public static void log2(String message) {
        if (verbMode)
            System.err.println("**> " + message);
    }

    private static void logErr(String message) {
        if (verbMode)
            System.err.println("!!**> mainThread: " + message);
    }

    //====================================================== test program

    public static Object waitnotifyObj = new Object();
    public static Object lockingObject = new Object();
    static Thread mainThread = null;

    //----------------------------------------------------   main method

    public static void main (String argv[]) {

        mainThread = Thread.currentThread();

        for (int i=0; i<argv.length; i++) {
            if ( argv[i].equals("-vbs") || argv[i].equals("-verbose") ) {
                verbMode = true;
                break;
            }
        }
        log1("debuggee started!");

        ArgumentHandler argHandler = new ArgumentHandler(argv);

        // informing a debugger of readyness
        IOPipe pipe = argHandler.createDebugeeIOPipe();
        pipe.println("ready");


        int exitCode = PASSED;
        for (int i = 0; ; i++) {

            String instruction;

            log1("waiting for an instruction from the debugger ...");
            instruction = pipe.readln();
            if (instruction.equals("quit")) {
                log1("'quit' recieved");
                break ;

            } else if (instruction.equals("newcheck")) {
                log1("'newcheck' recieved");

                switch (i) {

    //------------------------------------------------------  section tested

                case 0:
                     pipe.println("checkready");
                     instruction = pipe.readln();
                     if (!instruction.equals("continue")) {
                         logErr("ERROR: unexpected instruction: " + instruction);
                         exitCode = FAILED;
                     }
                     pipe.println("docontinue");
                     break;

                case 1:
                     label: {
                         synchronized (lockingObject) {
                             log1("entered: synchronized (lockingObject) {}");
                             synchronized (waitnotifyObj) {
                                 log1("entered: synchronized (waitnotifyObj) {}");

                                 pipe.println("checkready");
                                 instruction = pipe.readln();
                                 if (!instruction.equals("continue")) {
                                     logErr("ERROR: unexpected instruction: " + instruction);
                                     exitCode = FAILED;
                                     break label;
                                 }
                                 pipe.println("docontinue");
                             }
                             log1("exited: synchronized (waitnotifyObj) {}");
                         }
                         log1("exited: synchronized (lockingObject) {}");
                     }  // closing to 'label:'

    //-------------------------------------------------    standard end section

                default:
                     pipe.println("checkend");
                     break ;
                }

            } else {
                logErr("ERRROR: unexpected instruction: " + instruction);
                exitCode = FAILED;
                break ;
            }
        }

        System.exit(exitCode + PASS_BASE);
    }
}
