// IZebraProcessService.aidl
package android.zebra;

// Declare any non-default types here with import statements

interface IZebraProcessService {

    List<String> getWhiteList();

    boolean addWhiteProcess(String packName);

    boolean delWhiteProcess(String packName);

    void notifyProcessStatus(String packName, int status);
}
