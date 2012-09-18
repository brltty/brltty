package org.a11y.BRLTTY.Android;

import android.app.Activity;
import android.os.Bundle;

public class BRLTTY extends Activity
{
    /** Called when the activity is first created. */
    @Override
    public void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.main);
    }
}
