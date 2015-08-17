package com.rethinkdb.model;

import junit.framework.TestCase;

public class PathSpecTest extends TestCase {

    public void testMakeSimple() throws Exception {
        PathSpec obtained = PathSpec.make("hi");
    }
}