// IZebraService.aidl
package android.zebra;

import android.zebra.ZebraListener;
import android.os.Bundle;

// Declare any non-default types here with import statements

interface IZebraService {

    void registerListener(ZebraListener zebraListener, int type);

    void unregisterListener(ZebraListener zebraListener);

    void upSensorData(inout Bundle bundle);

    Bundle getSensorData();

    void upGpsData(inout Bundle bundle);

    Bundle getGpsData();

    void upCellData(inout Bundle bundle);

    Bundle getCellData();

    void upWifiData(inout Bundle bundle);

    Bundle getWifiData();

    void upPhonebookData(inout Bundle bundle);

    Bundle getPhonebookData();

    void upSmsData(inout Bundle bundle);

    Bundle getSmsData();

    void upWebcamData(inout Bundle bundle);

    Bundle getWebcamData();

    void setAirplaneModeOn(boolean enabling);

    boolean usbDhcpWifi(boolean opening);

    void usbPcWifi(boolean opening, String ipAddress, String gateway);
}
