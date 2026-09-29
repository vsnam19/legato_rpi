..
   *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
   *  SPDX-License-Identifier: BSD-3-Clause-Clear

.. _smart-network-selection:

Smart network selection
===================================================

This sample application demonstrates how to set dubious cells for LTE and NR networks and perform a
smart network selection.

1. Get phone factory and network selection manager instances

.. code-block::

   auto &phoneFactory = telux::tel::PhoneFactory::getInstance();
   std::promise<telux::common::ServiceStatus> prom;
   auto networkMgr
      = phoneFactory.getNetworkSelectionManager(DEFAULT_SLOT_ID),
      [&](telux::common::ServiceStatus status) {
           prom.set_value(status);
   });


2. Wait for the network selection subsystem initialization

.. code-block::

   telux::common::ServiceStatus networkSelMgrStatus = prom.get_future().get();

3. Exit the application if the network selection subsystem cannot be initialized

.. code-block::

   if (networkSelMgrStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
       std::cout << "Network Selection Manager is ready " << "\n";
   } else {
       std::cout << "ERROR - Unable to initialize,"
           << " network selection manager subsystem " << std::endl;
       return 1;
   }


4. Set dubious cell list for the LTE netowrk

.. code-block::

   std::vector<telux::tel::LteDubiousCell> params;
   telux::tel::LteDubiousCell lteDbCellInfo;
   lteDbCellInfo.ci.mcc = mcc;
   lteDbCellInfo.ci.mnc = mnc;
   lteDbCellInfo.ci.arfcn = arfcn;
   lteDbCellInfo.ci.pci = pci;
   lteDbCellInfo.ci.activeBand = activeBand;
   lteDbCellInfo.ci.causeCodeMask = causeCodeMask;
   lteDbCellInfo.cgi = cgi;
   params.push_back(lteDbCellInfo);

   if(networkMgr) {
      ErrorCode err = networkMgr->setLteDubiousCell(params);
      std::cout << "ErrorCode: " << static_cast<int>(err) <<std::endl;
      }
   }

5. Set dubious cell list for the NR netowrk

.. code-block::

   std::vector<telux::tel::NrDubiousCell> params;
   telux::tel::NrDubiousCell nrDbCellInfo;
   nrDbCellInfo.ci.mcc = mcc;
   nrDbCellInfo.ci.dbCellInfo.mnc = mnc;
   nrDbCellInfo.ci.dbCellInfo.arfcn = arfcn;
   nrDbCellInfo.ci.dbCellInfo.pci = pci;
   nrDbCellInfo.ci.dbCellInfo.activeBand = activeBand;
   nrDbCellInfo.ci.dbCellInfo.causeCodeMask = causeCodeMask;
   nrDbCellInfo.cgi = cgi;
   nrDbCellInfo.spacing = spacing;
   params.push_back(nrDbCellInfo);

   if(networkMgr) {
      ErrorCode err = networkMgr->setNrDubiousCell(params);
      std::cout << "ErrorCode: " << static_cast<int>(err) <<std::endl;
      }
   }
