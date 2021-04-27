package org.a11y.brltty.android;

import android.test.ActivityInstrumentationTestCase2;

/**
 * This is a simple framework for a test of an Application.  See
 * {@link android.test.ApplicationTestCase ApplicationTestCase} for more information on
 * how to write and extend Application tests.
 * <p/>
 * To run this test, you can type:
 * adb shell am instrument -w \
 * -e class org.a11y.brltty.android.MainTest \
 * org.a11y.brltty.android.tests/android.test.InstrumentationTestRunner
 */
public class MainTest extends ActivityInstrumentationTestCase2<Main> {

    public MainTest() {
        super("org.a11y.brltty.android", Main.class);
    }

}
