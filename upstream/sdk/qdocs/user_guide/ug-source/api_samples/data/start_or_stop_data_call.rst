..
   *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
   *  SPDX-License-Identifier: BSD-3-Clause-Clear

.. _start-or-stop-data-call:

Start/Stop Cellular Data Call
=============================

This sample application demonstrates how to start or stop a cellular data call.

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

2. Get the DataConnectionManager instance

.. code-block::

   std::unique_lock<std::mutex> lck(mtx);
   dataConnMgr = dataFactory.getDataConnectionManager(slotId, initCb);

3. Wait for DataConnectionManager initialization to complete

.. code-block::

   initCv.wait(lck);

3.1 Check DataConnectionManager initialization state

If initialization fails, retry by repeating step 2. If successful, proceed to step 4.

.. code-block::

   if (status_ == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
      // Proceed to step 4
   } else {
      // Retry step 2 for another initialization attempt
   }

4. Register the listener

Register a listener with the DataConnectionManager to monitor data call status.

.. code-block::

   dataConnMgr->registerListener(dataConnectionListener);

5. Implement the DataCallResponseCb callback for startDataCall

.. code-block::

   void startDataCallResponseCallBack(const std::shared_ptr<telux::data::IDataCall> &dataCall,
                                      telux::common::ErrorCode error) {
      std::cout << "Received callback for startDataCall" << std::endl;
      if (error == telux::common::ErrorCode::SUCCESS) {
         std::cout << "Request sent successfully" << std::endl;
      } else {
         std::cout << "Request failed with errorCode: " << static_cast<int>(error) << std::endl;
      }
   }

6. Send a start data call request with profile ID, IP family type, and the callback method

.. code-block::

   telux::common::Status status =
      dataConnectionManager->startDataCall(profileId, telux::data::IpFamilyType::IPV4V6,
                                           startDataCallResponseCallBack);
   if (status == telux::common::Status::SUCCESS) {
      // Wait for startDataCallResponseCallBack for further updates
   }

7. The response callback will be triggered if startDataCall returns success.
   If the data call was not already connected, additional information will be provided via the listener registered in step 4, i.e., IDataConnectionListener::onDataCallInfoChanged.

8. Implement the DataCallResponseCb callback for stopDataCall

.. code-block::

   void stopDataCallResponseCallBack(const std::shared_ptr<telux::data::IDataCall> &dataCall,
                                     telux::common::ErrorCode error) {
      std::cout << "Received callback for stopDataCall" << std::endl;
      if (error == telux::common::ErrorCode::SUCCESS) {
         std::cout << "Request sent successfully" << std::endl;
      } else {
         std::cout << "Request failed with errorCode: " << static_cast<int>(error) << std::endl;
      }
   }

9. Send a stop data call request with profile ID, IP family type, and the callback method

.. code-block::

   dataConnectionManager->stopDataCall(profileId, telux::data::IpFamilyType::IPV4V6,
                                       stopDataCallResponseCallBack);

The response callback will be triggered for the stopDataCall response.
