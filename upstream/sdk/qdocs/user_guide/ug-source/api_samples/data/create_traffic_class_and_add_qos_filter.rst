..
   *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
   *  SPDX-License-Identifier: BSD-3-Clause-Clear

.. _create-traffic-class-and-add-qos-filter:

Create traffic class and add QoS filter
==================================================================

Example use cases as per different data paths in system:

1. VLAN-based downlink traffic, tethered to apps software path
2. VLAN-based uplink traffic, tethered to the apps software path
3. IPv4-based downlink traffic, tethered to the WAN hardware accelerated path
4. IPv4-based uplink traffic, tethered to the WAN hardware accelerated path
5. IPv4-based uplink traffic, from apps to the WAN path

1. VLAN-based downlink traffic, tethered to apps software path (ETH <=> Apps):

1.1 Get VLAN and QoS Manager and wait for service availability

- Get the data factory, VLAN and QoS manager and wait until the service is available.

.. code-block::

    std::promise<telux::common::ServiceStatus> vlanProm;
    // VLAN Manager instance
    vlanManager_  = telux::data::DataFactory::getInstance().getVlanManager(
        telux::data::OperationType::DATA_LOCAL,
        [&vlanProm](telux::common::ServiceStatus status) {
        vlanProm.set_value(status);
        });

    if (!vlanManager_) {
        std::cout <<  " Failed to get VLAN Manager object" << std::endl;;
        return false;
    }

    telux::common::ServiceStatus subSystemStatus = vlanProm.get_future().get();
    if (subSystemStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        std::cout << " *** VLAN manager is Ready *** " << std::endl;
    } else {
        std::cout << " *** Unable to initialize VLAN subsystem *** " << std::endl;
        return false;
    }

    //Register for listener
    vlanManager_->registerListener(vlanListener);

    std::promise<telux::common::ServiceStatus> qosProm;

    // QoS Manager instance
    dataQoSManager_  = telux::data::DataFactory::getInstance().getQoSManager([&qosProm]
        (telux::common::ServiceStatus status) {
        qosProm.set_value(status);
        });

    if (!dataQoSManager_) {
        std::cout << " Failed to get DataQoSManager object" << std::endl;
    }

    telux::common::ServiceStatus subSystemStatus = qosProm.get_future().get();
    if (subSystemStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        std::cout << " *** QoS manager is Ready *** " << std::endl;
    } else {
        std::cout << " *** Unable to initialize QoS subsystem *** " << std::endl;
    }

    //Register for listener
    dataQoSManager_->registerListener(qosListener);

1.2. Create and wait for VLAN

- Create VLAN with below attributes
    - ID = 20
    - HW Acceleration = False
    - PCP = 7
- Note: Creation VLAN at first time might lead to device restart

.. code-block::

    std::promise<telux::common::ErrorCode> vlanProm;
    auto respCb = [&vlanProm, &vlanId, &pcp](bool isAccelerated, telux::common::ErrorCode error) {
        std::cout << std::endl << std::endl;
        std::cout << "VLAN ID: "  << vlanId << ", PCP: "<< pcp <<", IPA HW acceleration: "
                << (isAccelerated ? " enabled" : " not enabled") << std::endl;
        std::cout << "CALLBACK: "
                << "createVlan Response"
                << (error == telux::common::ErrorCode::SUCCESS ? " is successful" : " failed")
                << ". ErrorCode: " << static_cast<int>(error) << std::endl;
        vlanProm.set_value(error);
    };

    telux::data::VlanConfig config;
    config.iface = telux::data::InterfaceType::ETH;
    config.vlanId = vlanId;
    config.priority = pcp;
    config.isAccelerated = isAccelerated;

    telux::common::Status status = vlanManager_->createVlan(config, respCb);
    if ((status == telux::common::Status::SUCCESS) &&
        (vlanProm.get_future().get() == telux::common::ErrorCode::SUCCESS)) {
        return true;
    }

1.3. Create traffic class
- Create traffic class
    - TC ID = 0
    - Data path = TETHERED_TO_APPS_SW
    - BW Config {min = 5Mbps, max = 10Mbps}
    - Direction = DOWNLINK
- Note: The traffic class is uniquely identified using a combination of the traffic class ID and direction. i.e., it can be the same across different traffic classes.

.. code-block::

    telux::data::net::BandwidthConfig bandwidthConfig;
    bandwidthConfig.setDlBandwidthRange(minBandwidth, maxBandwidth);

    telux::data::net::TcConfigBuilder tcConfigBuilder;
    tcConfigBuilder.setTrafficClass(trafficClass).
        setDirection(telux::data::Direction::DOWNLINK).
        setDataPath(dataPath).
        setBandwidthConfig(bandwidthConfig);

    telux::data::net::TcConfigErrorCode tcConfigErrorCode;
    telux::common::ErrorCode errorCode
        = dataQoSManager_->createTrafficClass(tcConfigBuilder.build(), tcConfigErrorCode);
    if (errorCode == telux::common::ErrorCode::SUCCESS) {
        std::cout << " Create traffic class is successful." << std::endl;
        trafficClass_.push_back(trafficClass);
    } else {
        std::cout << " Create traffic class is failed. ErrorCode: " << static_cast<int>(errorCode)
                    << " " << static_cast<int>(tcConfigErrorCode) << std::endl;
        return false;
    }

1.4. Add VLAN based QoS filter

- Add VLAN based QoS filter with following parameter:
    - TC ID = 0
    - Data path = TETHERED_TO_APPS_SW
    - Direction = DOWNLINK
    - PCP = 7   Note: PCP is not a mandatory parameter to provide, but it is recommended to provide if available.
    - VLAN IDs = [20] Note: Instead of VLAN, a 5-tuple (i.e., L3, L4 parameters) can be provided.

- When a QoS filter is added successfully, a policy handle is provided as an output parameter.

.. code-block::

    telux::data::TrafficFilterBuilder tfBuilder;
    // In the downlink direction, the field type of VLAN is expected to be of the destination.
    tfBuilder.setDirection(direction).setVlanList({vlanId}, telux::data::FieldType::DESTINATION).
        setDataPath(dataPath).setPCP(pcp);

    // Configure QoS filter
    telux::data::net::QoSFilterConfig qosFilterConfig = {0};
    // traffic class
    qosFilterConfig.trafficClass = trafficClass;
    // traffic filter
    qosFilterConfig.trafficFilter = tfBuilder.build();

    // Add QoS filter
    uint32_t policyHandle;
    telux::data::net::QoSFilterErrorCode qosFilterErrorCode;
    telux::common::ErrorCode errorCode =
        dataQoSManager_->addQoSFilter(qosFilterConfig, policyHandle, qosFilterErrorCode);
    if (errorCode == telux::common::ErrorCode::SUCCESS) {
        std::cout << " Add QoS filter is successful. Handle of the QoS filter = " << policyHandle
                    << std::endl;
        qosFilterHandles_.push_back(policyHandle);
    } else {
        std::cout << " Add QoS filter is failed. ErrorCode: " << static_cast<int>(errorCode)
                    << " "  << static_cast<int>(qosFilterErrorCode) << std::endl;
        return 0;
    }

2. VLAN-based uplink traffic, tethered to the apps software path (ETH <=> Apps):

2.1 Get VLAN and QoS Manager and wait for service availability

- Get the data factory, VLAN and QoS manager and wait until the service is available.

- Step same as 1.1.

2.2. Create and wait for VLAN

- Create VLAN with below attributes
    - ID = 19
    - HW Acceleration = False
    - PCP = 7
- Parameters are different, but the step is similar to 1.2.

2.3. Create traffic class

- Create traffic class
    - TC ID = 0
    - Data path = TETHERED_TO_APPS_SW (ETH <=> Apps)
    - Direction = UPLINK
- Note: Bandwidth configuration/traffic shaping in the uplink direction is not supported.

.. code-block::

    telux::data::net::TcConfigBuilder tcConfigBuilder;
    tcConfigBuilder.setTrafficClass(trafficClass).
        setDirection(telux::data::Direction::UPLINK).
        setDataPath(dataPath);

    telux::data::net::TcConfigErrorCode tcConfigErrorCode;
    telux::common::ErrorCode errorCode
        = dataQoSManager_->createTrafficClass(tcConfigBuilder.build(), tcConfigErrorCode);
    if (errorCode == telux::common::ErrorCode::SUCCESS) {
        std::cout << " Create traffic class is successful." << std::endl;
        trafficClass_.push_back(trafficClass);
    } else {
        std::cout << " Create traffic class is failed. ErrorCode: " << static_cast<int>(errorCode)
                    << " " << static_cast<int>(tcConfigErrorCode) << std::endl;
        return false;
    }

2.4. Add VLAN based QoS filter

- Add VLAN based QoS filter with following parameter:
    - TC ID = 0
    - Data path = TETHERED_TO_APPS_SW (ETH <=> Apps)
    - Direction = UPLINK
    - PCP = 7   Note: It is not mandatory, but if available, providing PCP in the uplink direction for data paths involving the ETH module is beneficial.
    - VLAN IDs = [19] Note: Instead of VLAN, a 5-tuple (i.e., L3, L4 parameters) can be provided.

- When a QoS filter is added successfully, a policy handle is provided as an output parameter.

.. code-block::

    telux::data::TrafficFilterBuilder tfBuilder;
    // In the downlink direction, the field type of VLAN is expected to be of the destination.
    // For uplink, it is supposed to be the source.
    tfBuilder.setDirection(direction).setVlanList({vlanId}, telux::data::FieldType::SOURCE).
        setDataPath(dataPath).setPCP(pcp);

    // Configure QoS filter
    telux::data::net::QoSFilterConfig qosFilterConfig = {0};
    // traffic class
    qosFilterConfig.trafficClass = trafficClass;
    // traffic filter
    qosFilterConfig.trafficFilter = tfBuilder.build();

    // Add QoS filter
    uint32_t policyHandle;
    telux::data::net::QoSFilterErrorCode qosFilterErrorCode;
    telux::common::ErrorCode errorCode =
        dataQoSManager_->addQoSFilter(qosFilterConfig, policyHandle, qosFilterErrorCode);
    if (errorCode == telux::common::ErrorCode::SUCCESS) {
        std::cout << " Add QoS filter is successful. Handle of the QoS filter = " << policyHandle
                    << std::endl;
        qosFilterHandles_.push_back(policyHandle);
    } else {
        std::cout << " Add QoS filter is failed. ErrorCode: " << static_cast<int>(errorCode)
                    << " "  << static_cast<int>(qosFilterErrorCode) << std::endl;
        return 0;
    }

3. IPv4-based downlink traffic, tethered to the WAN hardware accelerated path (ETH <=> IPA <=> Modem):

3.1 Get VLAN, connection and QoS Manager and wait for service availability

- Get the data factory, VLAN, connection and QoS manager and wait until the service is available.
- In addition to 1.1, we will also need a data connection manager for some steps in this use case.
- The steps to get the data connection manager are as follows:

.. code-block::

    // data connection mananger
    dataConnectionManager_ =
        telux::data::DataFactory::getInstance().getDataConnectionManager(slotId,
                                            [&dcmProm](telux::common::ServiceStatus status)
                                            { dcmProm.set_value(status); });

    if (!dataConnectionManager_) {
        std::cout << " Failed to get DataConnectionManager object" << std::endl;
        return false;
    }

    //wait for connection manager to get ready
    std::cout << " Initializing Data connection manager subsystem Please wait" << std::endl;
    telux::common::ServiceStatus subSystemStatus = dcmProm.get_future().get();

    if (subSystemStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        std::shared_ptr<telux::data::IDataConnectionListener> dcmListener =  shared_from_this();
        std::cout << " *** Data Connection Manager is ready *** " << std::endl;
        telux::common::Status status =
        dataConnectionManager_->registerListener(dcmListener);

        if (status != telux::common::Status::SUCCESS) {
        std::cout << " Unable to register data connection manager listener" << std::endl;
        return false;
        }
    } else {
        std::cout << " Data Connection Manager is failed" << std::endl;
        dataConnectionManager_ = nullptr;
        return false;
    }

3.2. Create and wait for VLAN

- Create VLAN with below attributes
    - ID = 18
    - HW Acceleration = True
    - PCP = 6
- Parameters are different, but the step is similar to 1.2.

3.3. Bind VLAN with Backhaul
- Bind VLAN-18 to WWAN default Backhaul ( for example slotId = 1, profileId = 1)

.. code-block::

    std::promise<telux::common::ErrorCode> vlanProm;
    telux::data::net::VlanBindConfig vlanBindConfig = {};
    vlanBindConfig.bhInfo.backhaul = telux::data::BackhaulType::WWAN;
    vlanBindConfig.bhInfo.profileId = profileId_;
    vlanBindConfig.bhInfo.slotId = slotId_;
    vlanBindConfig.vlanId = vlanId;

    auto respCb = [&vlanProm](telux::common::ErrorCode error) {
        std::cout << std::endl << std::endl;
        std::cout << "CALLBACK: "
                << "bindToBackhaul Response"
                << (error == telux::common::ErrorCode::SUCCESS ? " is successful" : " failed")
                << ". ErrorCode: " << static_cast<int>(error) << std::endl;
        vlanProm.set_value(error);
    };

    telux::common::Status status = vlanManager_->bindToBackhaul(vlanBindConfig, respCb);
    if ((status == telux::common::Status::SUCCESS) &&
            (vlanProm.get_future().get() == telux::common::ErrorCode::SUCCESS)) {
        return true;
    }

3.4. Bring-up data call

- The data call is expected to be brought up on the same profile to successfully add a filter in the modem.

.. code-block::

    void logDataCallDetails(const std::shared_ptr<telux::data::IDataCall> &dataCall) {
        std::cout << " ** DataCall Details **\n";
        std::cout << " SlotID: " << dataCall->getSlotId() << std::endl;
        std::cout << " ProfileID: " << dataCall->getProfileId() << std::endl;
        std::cout << " interfaceName: " << dataCall->getInterfaceName() << std::endl;
        std::cout << " DataCallStatus: " << (int)dataCall->getDataCallStatus() << std::endl;
        std::cout
            << " DataCallEndReason: Type = " << static_cast<int>(dataCall->getDataCallEndReason().type)
            << std::endl;
        std::list<telux::data::IpAddrInfo> ipAddrList = dataCall->getIpAddressInfo();
        if(dataCall->getDataCallStatus() == telux::data::DataCallStatus::NET_CONNECTED) {

            // Check if the data call on the expected slot and profile is up.
            if((dataCall->getSlotId() == slotId_) && (dataCall->getProfileId() == profileId_)) {
                std::cout << " ** Active data call on default profile **\n";
                std::lock_guard<std::mutex> lock(dataCallMtx_);
                rmnetIp_ = ipAddrList.begin()->ifAddress;
                dataCallCv_.notify_all();
            }
            for(auto &it : ipAddrList) {
                std::cout << "\n ifAddress: " << it.ifAddress
                        << "\n primaryDnsAddress: " << it.primaryDnsAddress
                        << "\n secondaryDnsAddress: " << it.secondaryDnsAddress << '\n';
            }
            std::cout << "IpFamilyType: " << static_cast<int>(dataCall->getIpFamilyType()) << '\n';
            std::cout << "TechPreference: " << static_cast<int>(dataCall->getTechPreference()) << '\n';
        }
    }

    // This is a listener API in the data connection manager listener, which is invoked in changes to the information of a data call.
    void onDataCallInfoChanged(
        const std::shared_ptr<telux::data::IDataCall> &dataCall) {
        std::cout << "\n onDataCallInfoChanged";
        logDataCallDetails(dataCall);
    }

    // To avoid issue in start data call Before staring data call make sure APN is update signal strength and RAT band are selected as expected etc.
    // steps for this is not part of this sample app.
    telux::common::Status status =
        dataConnectionManager_->startDataCall(profileId_, ipFamilyType,
        [this](const std::shared_ptr<telux::data::IDataCall> &dataCall,
            telux::common::ErrorCode errorCode) {
                std::cout << "startCallResponse: errorCode: "
                << static_cast<int>(errorCode) << std::endl;

            // Check if the data call is already available, or else wait for the indication `onDataCallInfoChanged`.
            logDataCallDetails(dataCall);
        }, telux::data::OperationType::DATA_LOCAL);

        if(status == telux::common::Status::SUCCESS) {
            std::unique_lock<std::mutex> lck(dataCallMtx_);
            dataCallCv_.wait(lck, [&]{return !rmnetIp_.empty();});
            return true;
        }


3.5. Create traffic class

- Parameters are different, but the step is similar to 1.3.

- Create traffic class
    - TC ID = 1
    - BW Config {min = 5Mbps, max = 10Mbps}
    - Data path = TETHERED_TO_WAN_HW (ETH <=> IPA <=> Modem)
    - Direction = DOWNLINK
- Parameters are different, but the step is similar to 1.3.

3.6. Add IP based QoS filter

- Add IP based QoS filter

    - TC ID = 1
    - Data path = TETHERED_TO_WAN_HW (ETH <=> IPA <=> Modem) Note: Prioritization in the modem happens based on a 5-tuple (i.e., L3, L4 parameters).
    - Direction = DOWNLINK
    - Source IP = Remote server Note: An IP-based filter involving the modem needs the source IP, protocol, and (destination port or destination IP) as mandatory parameters.
    - Destination port = 30044
    - Protocol = TCP (6 as per IANA)
    - Source port = 8080

- When a QoS filter is added successfully, a policy handle is provided as an output parameter.

.. code-block::

    telux::data::TrafficFilterBuilder tfBuilder;
    tfBuilder.setDirection(direction).
        setIPv4Address(srcIPv4, telux::data::FieldType::SOURCE).
        setIPProtocol(protocol).
        setDataPath(dataPath).
        setPort(destPort, telux::data::FieldType::DESTINATION).
        setPort(srcPort, telux::data::FieldType::SOURCE);

    // Configure QoS filter
    telux::data::net::QoSFilterConfig qosFilterConfig = {0};
    // traffic class
    qosFilterConfig.trafficClass = trafficClass;
    // traffic filter
    qosFilterConfig.trafficFilter = tfBuilder.build();

    // Add QoS filter
    uint32_t policyHandle;
    telux::data::net::QoSFilterErrorCode qosFilterErrorCode;
    telux::common::ErrorCode errorCode =
        dataQoSManager_->addQoSFilter(qosFilterConfig, policyHandle, qosFilterErrorCode);
    if (errorCode == telux::common::ErrorCode::SUCCESS) {
        std::cout << " Add QoS filter is successful. Handle of the QoS filter = " << policyHandle
                    << std::endl;
        qosFilterHandles_.push_back(policyHandle);
        return policyHandle;
    } else {
        std::cout << " Add QoS filter is failed. ErrorCode: " << static_cast<int>(errorCode)
                    << " "  << static_cast<int>(qosFilterErrorCode) << std::endl;
        return 0;
    }


4. IPv4-based uplink traffic, tethered to the WAN hardware accelerated path (ETH <=> IPA <=> Modem):

4.1 Get VLAN, connection and QoS Manager and wait for service availability

- Get the data factory, VLAN, connection and QoS manager and wait until the service is available.
- Step same as 3.1.

4.2. Create and wait for VLAN

- Create VLAN with below attributes
    - ID = 18
    - HW Acceleration = True
    - PCP = 6
- Parameters are different, but the step is similar to 1.2.

4.3. Bind VLAN with Backhaul

- Bind VLAN-18 to WWAN default Backhaul ( for example slotId = 1, profileId = 1)

- Step same as 3.3.

4.4. Bring-up data call

- The data call is expected to be brought up on the same profile to successfully add a filter in the modem.

- Step same as 3.4.

4.5. Create traffic class

- Create traffic class
    - TC ID = 2
    - Data path = TETHERED_TO_WAN_HW
    - Direction = UPLINK

- Parameters are different, but the step is similar to 2.3.

4.6. Add IP based QoS filter

- Add IP based QoS filter

    - TC ID = 1
    - Data path = TETHERED_TO_WAN_HW (ETH <=> IPA <=> Modem) Note: Prioritization in the modem happens based on a 5-tuple (i.e., L3, L4 parameters).
    - Direction = UPLINK
    - Source IP = <Public IP> from IDataCall object (from step 4.4.)
    - Note: An IP-based filter involving the modem needs the source IP, protocol, and (destination port or destination IP) as mandatory parameters.
    - Destination port = 8081
    - Protocol = TCP (6 as per IANA)
    - PCP = 6 Note: It is not mandatory, but if available, providing PCP in the uplink direction for data paths involving the ETH module is beneficial.

- When a QoS filter is added successfully, a policy handle is provided as an output parameter.

.. code-block::

    telux::data::TrafficFilterBuilder tfBuilder;
    // The source IP in the uplink filter is expected to be the public IP of the data call from step 4.4.
    tfBuilder.setDirection(direction).
        setIPv4Address(srcIPv4, telux::data::FieldType::SOURCE).
        setIPProtocol(protocol).
        setDataPath(dataPath).
        setPort(destPort, telux::data::FieldType::DESTINATION)
        setPCP(6);

    // Configure QoS filter
    telux::data::net::QoSFilterConfig qosFilterConfig = {0};
    // traffic class
    qosFilterConfig.trafficClass = trafficClass;
    // traffic filter
    qosFilterConfig.trafficFilter = tfBuilder.build();

    // Add QoS filter
    uint32_t policyHandle;
    telux::data::net::QoSFilterErrorCode qosFilterErrorCode;
    telux::common::ErrorCode errorCode =
        dataQoSManager_->addQoSFilter(qosFilterConfig, policyHandle, qosFilterErrorCode);
    if (errorCode == telux::common::ErrorCode::SUCCESS) {
        std::cout << " Add QoS filter is successful. Handle of the QoS filter = " << policyHandle
                    << std::endl;
        qosFilterHandles_.push_back(policyHandle);
        return policyHandle;
    } else {
        std::cout << " Add QoS filter is failed. ErrorCode: " << static_cast<int>(errorCode)
                    << " "  << static_cast<int>(qosFilterErrorCode) << std::endl;
        return 0;
    }

5. IPv4-based uplink traffic, from apps to the WAN path (Apps <=> Modem):

5.1 Data connection and QoS Manager and wait for service availability

- Get the data factory, data connection and QoS manager and wait until the service is available.

.. code-block::

    std::promise<telux::common::ServiceStatus> qosProm;

    // QoS Manager instance
    dataQoSManager_  = telux::data::DataFactory::getInstance().getQoSManager([&qosProm]
        (telux::common::ServiceStatus status) {
        qosProm.set_value(status);
        });

    if (!dataQoSManager_) {
        std::cout << " Failed to get DataQoSManager object" << std::endl;
    }

    telux::common::ServiceStatus subSystemStatus = qosProm.get_future().get();
    if (subSystemStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        std::cout << " *** QoS manager is Ready *** " << std::endl;
    } else {
        std::cout << " *** Unable to initialize QoS subsystem *** " << std::endl;
    }

    //Register for listener
    dataQoSManager_->registerListener(qosListener);

    std::promise<telux::common::ServiceStatus> dcmProm;
    SlotId slotId = DEFAULT_SLOT_ID;
    // data connection mananger
    dataConnectionManager_ =
        telux::data::DataFactory::getInstance().getDataConnectionManager(slotId,
                                            [&dcmProm](telux::common::ServiceStatus status)
                                            { dcmProm.set_value(status); });

    if (!dataConnectionManager_) {
        std::cout << " Failed to get DataConnectionManager object" << std::endl;
        return false;
    }

    //wait for connection manager to get ready
    std::cout << " Initializing Data connection manager subsystem Please wait" << std::endl;
    telux::common::ServiceStatus subSystemStatus = dcmProm.get_future().get();

    if (subSystemStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        std::shared_ptr<telux::data::IDataConnectionListener> dcmListener =  shared_from_this();
        std::cout << " *** Data Connection Manager is ready *** " << std::endl;
        telux::common::Status status =
        dataConnectionManager_->registerListener(dcmListener);

        if (status != telux::common::Status::SUCCESS) {
        std::cout << " Unable to register data connection manager listener" << std::endl;
        return false;
        }
    } else {
        std::cout << " Data Connection Manager is failed" << std::endl;
        dataConnectionManager_ = nullptr;
        return false;
    }

5.2. Bring-up data call

- The data call is expected to be brought up on the same profile to successfully add a filter in the modem.

- Step same as 3.4.

5.3. Create traffic class

- Create traffic class

    - TC ID = 3
    - Data path = APPS_TO_WAN (Apps <=> Modem)
    - Direction = UPLINK

- Parameters are different, but the step is similar to 2.3.

5.4. Add IP based QoS filter

- Add IP based QoS filter

    - TC ID = 3
    - Data path = APPS_TO_WAN (Apps <=> Modem) Note: Prioritization in the modem happens based on a 5-tuple (i.e., L3, L4 parameters).
    - Direction = UPLINK
    - Source IP = <Public IP> from IDataCall object (from 5.2) Note: An IP-based filter involving the modem needs the source IP, protocol, and (destination port or destination IP) as mandatory parameters.
    - Destination port = 8080
    - Protocol = UPD (17 as per IANA)

- When a QoS filter is added successfully, a policy handle is provided as an output parameter.

.. code-block::

    telux::data::TrafficFilterBuilder tfBuilder;
    tfBuilder.setDirection(direction).
        setIPv4Address(srcIPv4, telux::data::FieldType::SOURCE).
        setIPProtocol(protocol).
        setDataPath(dataPath).
        setPort(destPort, telux::data::FieldType::DESTINATION);

    // Configure QoS filter
    telux::data::net::QoSFilterConfig qosFilterConfig = {0};
    // traffic class
    qosFilterConfig.trafficClass = trafficClass;
    // traffic filter
    qosFilterConfig.trafficFilter = tfBuilder.build();

    // Add QoS filter
    uint32_t policyHandle;
    telux::data::net::QoSFilterErrorCode qosFilterErrorCode;
    telux::common::ErrorCode errorCode =
        dataQoSManager_->addQoSFilter(qosFilterConfig, policyHandle, qosFilterErrorCode);
    if (errorCode == telux::common::ErrorCode::SUCCESS) {
        std::cout << " Add QoS filter is successful. Handle of the QoS filter = " << policyHandle
                    << std::endl;
        qosFilterHandles_.push_back(policyHandle);
        return policyHandle;
    } else {
        std::cout << " Add QoS filter is failed. ErrorCode: " << static_cast<int>(errorCode)
                    << " "  << static_cast<int>(qosFilterErrorCode) << std::endl;
        return 0;
    }
