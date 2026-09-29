.. _wlan-sta-config:

How to configure and use WLAN STA operations
============================================


The following steps illustrate how to configure and use WLAN STA operation.

1. Create the IWlanListener, IStaListener class.

.. code-block::

    class NotificationListener: public telux::wlan::IWlanListener,
                                public telux::wlan::IStaListener,
                                public std::enable_shared_from_this<NotificationListener> {
    public:
    void onEnableChanged(bool enable) {
        promise_.set_value(enable);
    }

    bool getEnableStatus() {
        return promise_.get_future().get();
    }

    bool isScanCompleted() {
        return scanCompletePromise_.get_future().get();
    }

    void resetPromise() {
        promise_ = std::promise<bool>();
    }

    void resetScanCompletePromise() {
        scanCompletePromise_ = std::promise<bool>();
    }

    void onScanResultUpdated(const telux::wlan::StaScanResult &staScanResult) {
        // Use staScanResult constituents as needed
        if(staScanResult.isScanComplete) {
            scanCompletePromise_.set_value(true);
        }
    }

    void onStationStatusChanged(std::vector<telux::wlan::StaStatus> status) {
         std::cout << "length of Active Stations:" << status.size() << std::endl;
    }

    private:
    std::promise<bool> promise_;
    std::promise<bool> scanCompletePromise_;
    };


2. Create the initialization callback object to be notified when initialization is complete and the object is ready to be used.

.. code-block::

   std::promise<telux::common::ServiceStatus> initPromise{};

    auto initCb = [&](telux::common::ServiceStatus status) {
        initPromise.set_value(status);
    };


3. Get the WlanFactory, WlanDeviceManager and StaInterfaceManager instances.

.. code-block::

    auto &wlanFactory = telux::wlan::WlanFactory::getInstance();
    wlanDevMgr_  = wlanFactory.getWlanDeviceManager(initCb);
    wlanStaMgr_  = wlanFactory.getStaInterfaceManager();


4. Wait for the object to complete initialization.

.. code-block::

    telux::common::ServiceStatus subSystemStatus = initPromise.get_future().get();
    if (subSystemStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        std::cout << " *** Wlan SubSystem is Ready *** " << std::endl;
    }
    else {
        std::cerr << " *** Unable to initialize Wlan subsystem *** " << std::endl;
    }


5. If WLAN is enabled, disable it (optional).

.. code-block::

    bool enableStat = false;
    std::vector<telux::wlan::InterfaceStatus> ifStatus;
    if(telux::common::ErrorCode::SUCCESS != wlanDevMgr_->getStatus(enableStat, ifStatus)) {
        std::cerr << "Failed to retrieve Wlan status" <<std::endl;
    }
    if(enableStat) {
        //Ensure callback returns SUCCESS and indication for Wlan disablement is received
        if((telux::common::ErrorCode::SUCCESS != wlanDevMgr_->enable(false)) ||
            (true == listener->getEnableStatus())) {
            std::cerr << "Failed to disable Wlan " <<std::endl;
        }
    }


6. Register listeners to receive notifications.

.. code-block::

    telux::common::ErrorCode ec;
    ec = wlanDevMgr_->registerListener(listener);
    if (ec != telux::common::ErrorCode::SUCCESS) {
        std::cerr << "Can't register to IWlanListener" << std::endl;
    }

    ec = wlanStaMgr_->registerListener(listener);
    if (ec != telux::common::ErrorCode::SUCCESS) {
        std::cerr << "Can't register to IStaListener" << std::endl;
    }


7. Set the desired WLAN configurations.

.. code-block::

   //Ensure that atleast one station is configured, numSta > 1
    if(telux::common::ErrorCode::SUCCESS != wlanDevMgr_->setMode(numAp, numSta)) {
        std::cerr << "Failed to set Wlan configuration" <<std::endl;
    }


8. Enable WLAN for new configurations to take effect.

.. code-block::

    listener->resetPromise();
    //Ensure callback returns SUCCESS and indication for Wlan enablement is received
    if((telux::common::ErrorCode::SUCCESS != wlanDevMgr_->enable(true)) ||
       (false == listener->getEnableStatus())) {
        std::cerr << "Failed to enable Wlan " << std::endl;
    }


9. Initiate STA scan process.

.. code-block::

    listener->resetScanCompletePromise();

    if(telux::common::ErrorCode::SUCCESS != wlanStaMgr_->startScan(telux::wlan::Id::PRIMARY)) {
        std::cerr << "Failed to enable Wlan STA Scan " << std::endl;
    }


10. Add the network configuration once the scan is complete.

.. code-block::

    telux::common::ErrorCode ec;

    if(listener->isScanCompleted()) {
        std::cout << "Adding Network Config" << std::endl;
        ec = wlanStaMgr_->addNetworkConfig(telux::wlan::Id::PRIMARY, staNetworkConfigEntry);
        if (ec != telux::common::ErrorCode::SUCCESS) {
            std::cerr << "Can't add NetworkConfig entry, err " << static_cast<int>(ec)
                        << std::endl;
        }
        // expect scan connection status details via onStationStatusChanged() notification
    }

