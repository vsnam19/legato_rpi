..
   *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
   *  SPDX-License-Identifier: BSD-3-Clause-Clear

.. _start-tcp-keep-alive-offload:

Start TCP Keep-Alive Offload
=============================

This sample application demonstrates how to start or stop TCP keep-alive offload to the modem.

1. Implement the initialization callback and get the DataFactory instance

An optional initialization callback can be provided when obtaining the manager instance.
The DataFactory will invoke the callback once manager initialization is complete.

.. code-block::

    auto initCb = telux::common::ServiceStatus status {
        std::lock_guard<std::mutex> lock(mtx);
        status_ = status;
        initCv.notify_all();
    };
    auto &dataFactory = DataFactory::getInstance();

2. Get the KeepAliveManager instance

.. code-block::

    std::unique_lock<std::mutex> lck(mtx);
    keepAliveManager = dataFactory.getKeepAliveManager(slotId, initCb);

3. Wait for KeepAliveManager initialization to complete

.. code-block::

    initCv.wait(lck);

3.1 Check KeepAliveManager initialization state

If initialization fails, retry by repeating step 2. If successful, proceed to step 4.

.. code-block::

    if (status_ == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        // Proceed to step 4
    } else {
        // Retry step 2 for another initialization attempt
    }

4. Register the listener

Register a listener with the KeepAliveManager to receive updates if keep-alive offload fails.

.. code-block::

    keepAliveManager->registerListener(keepAliveListener);

5. Precondition Checks

   a. The data call has started.
   b. The TCP socket connection (required for offloading keep-alive to the modem) is established.


6. Start Keep-Alive Offload

### a. Monitor Mode

6.1 If operating in Monitor Mode, TCP session info (e.g., sequence numbers) is not needed.

.. code-block::

    struct TCPKAParams params{};
    params.srcIp    // Source IPv4/IPv6 address (e.g., DUT interface where TCP session is established)
    params.srcPort  // Source port
    params.dstIp    // Destination IPv4/IPv6 address (e.g., peer/remote end of TCP session)
    params.dstPort  // Destination port
    uint32_t interval        // Keep-alive interval in milliseconds (must be ≥ 60000 ms)

    ErrorCode errEnableTCPMonitor = keepAliveMgr_->enableTCPMonitor(params, monHandle);

    if (errEnableTCPMonitor != ErrorCode::SUCCESS) {
        std::cout << "Operation failed with error code: " << static_cast<int>(errEnableTCPMonitor) << std::endl;
    } else {
        std::cout << "Operation completed. TCP monitor handle: " << monHandle << std::endl;
    }

6.2 At least one packet must be exchanged between the TCP client and server after calling `enableTCPMonitor()`.

Ensure or wait sufficiently long to confirm that TCP packets are acknowledged and no in-flight packets exist in the TCP session.

6.3 Start offloading keep-alive for the TCP session

.. code-block::

    ErrorCode errTCPKeepAliveOffload = keepAliveMgr_->startTCPKeepAliveOffload(monHandle, interval, offloadHandle);

    if (errTCPKeepAliveOffload != ErrorCode::SUCCESS) {
        std::cout << "Operation failed with error code: " << static_cast<int>(errTCPKeepAliveOffload) << std::endl;
    } else {
        std::cout << "Operation completed. TCP keep-alive offload handle: " << offloadHandle << std::endl;
    }

### b. Default Mode

6.1 TCP session info (e.g., sequence numbers) is required for offloading keep-alive to the modem.

.. code-block::

    struct TCPSessionParams session{};
    session.recvNext      // Next sequence number expected on the incoming packet
    session.recvWindow    // Receive window
    session.sendNext      // Next sequence number to be sent
    session.sendWindow    // Send window

    struct TCPKAParams params{};
    params.srcIp    // Source IPv4/IPv6 address (e.g., DUT interface where TCP session is established)
    params.srcPort  // Source port
    params.dstIp    // Destination IPv4/IPv6 address (e.g., peer/remote end of TCP session)
    params.dstPort  // Destination port
    uint32_t interval        // Keep-alive interval in milliseconds (must be ≥ 60000 ms)

    ErrorCode errTCPKeepAliveOffload = keepAliveMgr_->startTCPKeepAliveOffload(params, session, interval, handle);

    if (errTCPKeepAliveOffload != ErrorCode::SUCCESS) {
        std::cout << "Operation failed with error code: " << static_cast<int>(errTCPKeepAliveOffload) << std::endl;
    } else {
        std::cout << "Operation completed. TCP keep-alive offload handle: " << offloadHandle << std::endl;
    }

7. Once offload is started, TCP keep-alive messages are sent by the modem at the configured interval. This continues even if the device enters suspend mode.

8. Any TCP packet transfer over the same session will result in failure or termination of the keep-alive offload. The application is notified via `onKeepAliveStatusChange(NETWORK_ERR, tcpKAOffloadHandle)` from `IKeepAliveListener` registered in step 4.
