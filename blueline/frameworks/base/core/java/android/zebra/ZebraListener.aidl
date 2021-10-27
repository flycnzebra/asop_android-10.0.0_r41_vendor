// ZebraListener.aidl
package android.zebra;

import android.os.Bundle;

// Declare any non-default types here with import statements

interface ZebraListener {

    oneway void notifySensorChange(in Bundle bundle);

    oneway void notifyGpsChange(in Bundle bundle);

    oneway void notifyCellChange(in Bundle bundle);

    oneway void notifyWifiChange(in Bundle bundle);

}