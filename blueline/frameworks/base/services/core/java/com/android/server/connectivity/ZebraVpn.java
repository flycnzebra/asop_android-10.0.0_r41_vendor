package com.android.server.connectivity;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.pm.UserInfo;
import android.net.ConnectivityManager;
import android.net.INetworkManagementEventObserver;
import android.net.IpPrefix;
import android.net.LinkProperties;
import android.net.NetworkAgent;
import android.net.NetworkCapabilities;
import android.net.NetworkInfo;
import android.net.NetworkInfo.DetailedState;
import android.net.RouteInfo;
import android.net.UidRange;
import android.os.Binder;
import android.os.INetworkManagementService;
import android.os.Looper;
import android.os.RemoteException;
import android.os.SystemProperties;
import android.os.UserManager;
import android.util.Log;
import android.zebra.FlyLog;

import com.android.internal.annotations.GuardedBy;
import com.android.server.net.BaseNetworkObserver;

import java.net.InetAddress;
import java.util.ArrayList;
import java.util.List;

/**
 * Author FlyZebra
 * 2021/10/22 0022 15:23
 * Describ:
 * @hide
 */
public class ZebraVpn {
    private static final String NETWORKTYPE = "ZEBRA-MP";
    private Context mContext;
    private NetworkInfo mNetworkInfo;
    private String mInterface = "tun0";
    private NetworkAgent mNetworkAgent;
    private final Looper mLooper;
    private final NetworkCapabilities mNetworkCapabilities;

    /* list of users using this VPN. */
    @GuardedBy("this")
    private List<UidRange> mVpnUsers = null;
    private BroadcastReceiver mUserIntentReceiver = null;

    public ZebraVpn(Looper looper, Context context, INetworkManagementService netService, int userHandle) {
        mContext = context;
        mLooper = looper;
        try {
            netService.registerObserver(mObserver);
        } catch (RemoteException e) {
            FlyLog.e("Problem registering observer %s", e.toString());
        }

        mNetworkInfo = new NetworkInfo(ConnectivityManager.TYPE_VPN, 0, NETWORKTYPE, "");
        // TODO: Copy metered attribute and bandwidths from physical transport, b/16207332
        mNetworkCapabilities = new NetworkCapabilities();
        mNetworkCapabilities.addCapability(NetworkCapabilities.NET_CAPABILITY_INTERNET);
//        mNetworkCapabilities.addTransportType(NetworkCapabilities.TRANSPORT_VPN);
//        mNetworkCapabilities.removeCapability(NetworkCapabilities.NET_CAPABILITY_NOT_VPN);
    }

    public synchronized void agentConnect() {
        if (mNetworkAgent != null) {
            FlyLog.d("tsylog zebravpn network is already create.");
            return;
        }
        mInterface = "tun0";
        mVpnUsers = new ArrayList<UidRange>();
        LinkProperties lp = new LinkProperties();
        lp.setInterfaceName(mInterface);
        try {
//            lp.addRoute(new RouteInfo(new IpPrefix(InetAddress.getByName("::"), Integer.parseInt("0")), null));
            lp.addRoute(new RouteInfo(new IpPrefix(InetAddress.getByName("0.0.0.0"), Integer.parseInt("0")), null));
            String dns = SystemProperties.get("persist.sys.mag.dns", "172.16.251.77");
            lp.addDnsServer(InetAddress.getByName(dns));
            lp.addDnsServer(InetAddress.getByName("8.8.8.8"));
        } catch (Exception e) {
            FlyLog.e("tsylog init route and dns server error" + e.toString());
        }
//        lp.setDomains(buffer.toString().trim());

        mNetworkInfo.setIsAvailable(true);
        mNetworkInfo.setDetailedState(DetailedState.CONNECTED, null, "zebramp");

        long token = Binder.clearCallingIdentity();
        try {
            FlyLog.d("tsylog create vpn NetworkAgent!");
            mNetworkAgent = new NetworkAgent(mLooper, mContext, NETWORKTYPE,
                    mNetworkInfo, mNetworkCapabilities, lp, 100, null) {
                @Override
                public void unwanted() {
                    // We are user controlled, not driven by NetworkRequest.
                }
            };
        } finally {
            Binder.restoreCallingIdentity(token);
        }
    }

    public synchronized void agentDisconnect() {
        mInterface = null;
        if (mNetworkInfo != null) {
            mNetworkInfo.setIsAvailable(false);
            mNetworkInfo.setDetailedState(DetailedState.DISCONNECTED, null, null);
        }
        if (mNetworkAgent != null) {
            mNetworkAgent.sendNetworkInfo(mNetworkInfo);
            mNetworkAgent = null;
            FlyLog.d("tsylog zebravpn network is destroy!");
        }
    }

    private INetworkManagementEventObserver mObserver = new BaseNetworkObserver() {

        @Override
        public void interfaceAdded(String interfaze) {
            FlyLog.d("tsylog interfaceAdded " + interfaze);
            synchronized (ZebraVpn.this) {
//                if(interfaze.startsWith("tun0")){
//                    mInterface = interfaze;
//                    agentConnect();
//                }
            }
        }

        @Override
        public void interfaceRemoved(String interfaze) {
            FlyLog.d("tsylog interfaceRemoved " + interfaze);
            synchronized (ZebraVpn.this) {
                if (interfaze != null && interfaze.equals(mInterface)) {
                    mInterface = null;
                    agentDisconnect();
                }
            }
        }
    };

}