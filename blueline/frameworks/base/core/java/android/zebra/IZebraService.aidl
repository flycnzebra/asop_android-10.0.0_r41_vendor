// IZebraService.aidl
package android.zebra;

import android.zebra.ZebraListener;
import android.os.Bundle;

// Declare any non-default types here with import statements
interface IZebraService {

    void registerListener(ZebraListener zebraListener);

    void unRegisterListener(ZebraListener zebraListener);

    void upSensorData(inout Bundle bundle);

    Bundle getSensorData();

    void upGpsData(inout Bundle bundle);

    Bundle getGpsData();

    void upCellData(inout Bundle bundle);

    Bundle getCellData();

    void upWifiData(inout Bundle bundle);

    Bundle getWifiData();

    //OcProcessService
    List<String> getWhiteList();

    boolean addWhiteProcess(String packName);

    boolean delWhiteProcess(String packName);

}
