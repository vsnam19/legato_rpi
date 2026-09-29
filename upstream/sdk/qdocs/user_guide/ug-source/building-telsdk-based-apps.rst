.. #=============================================================================
   #
   # Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
   # SPDX-License-Identifier: BSD-3-Clause-Clear
   #
   #=============================================================================

====================================
Building TelSDK based applications
====================================

-----------------------------------
You have access to ChipCode portal
-----------------------------------

The TelSDK deliverables (headers, libraries, prebuilt artifacts, selinux policies etc.) are delivered to the Qualcomm Technologies, Inc customers as part of a software product release through ChipCode portal. This delivery contains both public and non-public TelSDK deliverables. Given this information, the steps to develop and build an application would be:

1. Sync whole software product release package from ChipCode portal.
2. Write a TelSDK based application.
3. Write a Yocto recipe to build this application.
4. Integrate this application and recipe in the Yocto build system.
5. Finally, build the application and test it on the target hardware.


Setting up a build environment
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The following are the steps to be followed to setup build environment.

1. Setup the machine and OS distribution environment.

  .. code-block::

    # For SA515M.LE.2.3
    $ export MACHINE=sa515m
    $ export DISTRO=qti-distro-tele-debug
    # For SA415M.LE.1.8
    $ export MACHINE=sa415m
    $ export DISTRO=qti-distro-tele-debug
    # For MDM9607.LE.2.5
    $ export MACHINE=mdm9607
    $ export DISTRO=qti-distro-tele-debug
    # For MDM9650.LE.2.6
    # For MDM9650-2K build
    $ export MACHINE=mdm9650
    $ export DISTRO=qti-distro-tele-debug
    # For MDM9650 build
    $ export MACHINE=mdm9650-4k
    $ export DISTRO=qti-distro-tele-debug

2. Source set_bb_env.sh script in poky directory and build the target image.

  a. The below build steps hold good for the following SP's SA515M.LE.2.1.3, SA415M.LE.1.7, SA2150P.LE.1.1, SA2150P.LE.1.3, SA2150P.LE.1.4, MDM9607.LE.2.2, MDM9607.LE.2.4, MDM9607.LE.2.0.1 and MDM9650.LE.2.3.

    .. code-block::

      $ source poky/build/conf/set_bb_env.sh
      $ build-<target>-image

    The target can be sa515m, sa415m, sa2150p, 9607, 9650 or 9650-2k etc.

  b. The below build steps hold good for the following SP's SA515M.LE.2.3, SA415M.LE.1.8, MDM9607.LE.2.5 and MDM9650.LE.2.6.

    .. code-block::

      $ source poky/qti-conf/set_bb_env.sh
      $ bitbake qti-tele-image

The following are the steps to setup build environment for SA525M

  1. For PVM:

   .. code-block::

     # For Kernel compilation
     $ cd src/kernel-5.15/kernel_platform/
     $ BUILD_CONFIG=msm-kernel/build.config.msm.sa525 EXTRA_CONFIGS=./prebuilts/qcom_boot_artifacts/build.config.qc.standalone VARIANT=debug_defconfig OUT_DIR=../out/msm-kernel-sa525-debug_defconfig ./build/build.sh  -j24 V=1 2>&1 | tee pvm_kernel_compile.log
     # For Userspace compilation
     $ export SHELL=/bin/bash && export MACHINE=sa525m && export DISTRO=qti-distro-tele-debug;source poky/qti-conf/set_bb_env.sh
     $ bitbake qti-tele-image

  2. For MULTIVM:

    a. HOSTVM

    .. code-block::

     # For Kernel compilation
     $ cd src/kernel-5.15/kernel_platform/
     $ BUILD_CONFIG=msm-kernel/build.config.msm.sa525 EXTRA_CONFIGS=./prebuilts/qcom_boot_artifacts/build.config.qc.standalone VARIANT=hostvm_debug_defconfig OUT_DIR=../out/msm-kernel-sa525-hostvm_debug_defconfig ./build/build.sh  -j24 V=1 2>&1 | tee hostvm_kernel_compile.log
     # For Userspace compilation
     $ export SHELL=/bin/bash && export MACHINE=sa525m && export DISTRO=qti-distro-tele-debug;source poky/qti-conf/set_bb_env.sh
     $ bitbake -R conf/machine/include/telematics-hostvm.inc qti-tele-hostvm-image

    b. TELEVM

    .. code-block::

     # For Kernel compilation
     $ cd src/kernel-5.15/kernel_platform/
     $ BUILD_CONFIG=msm-kernel/build.config.msm.sa525m.televm EXTRA_CONFIGS=./prebuilts/qcom_boot_artifacts/build.config.qc.standalone VARIANT=debug_defconfig OUT_DIR=../out/msm-kernel-sa525m_televm-debug_defconfig ./build/build.sh  -j24 V=1 2>&1 | tee svm_kernel_compile.log
     # For Userspace compilation
     $ export MACHINE=sa525m-televm; export DISTRO=qti-distro-tele-debug; export SHELL=/bin/bash;source poky/qti-conf/set_bb_env.sh
     $ bitbake qti-tele-vm-image

    c. FOTAVM

    .. code-block::

     # For Kernel compilation
     $ cd src/kernel-5.15/kernel_platform/
     $ BUILD_CONFIG=msm-kernel/build.config.msm.sa525m.fotavm EXTRA_CONFIGS=./prebuilts/qcom_boot_artifacts/build.config.qc.standalone VARIANT=debug_defconfig OUT_DIR=../out/msm-kernel-sa525m_fotavm-debug_defconfig ./build/build.sh  -j24 V=1 2>&1 | tee fotavm_kernel_compile.log
     # For Userspace compilation
     $ export MACHINE=sa525m-fotavm; export DISTRO=qti-distro-tele-debug; export SHELL=/bin/bash;source poky/qti-conf/set_bb_env.sh
     $ bitbake qti-fota-vm-image

  3. For NAD2:

    .. code-block::

     # For Kernel compilation
     $ cd src/kernel-5.15/kernel_platform/
     $ BUILD_CONFIG=msm-kernel/build.config.msm.sa525 EXTRA_CONFIGS=./prebuilts/qcom_boot_artifacts/build.config.qc.standalone VARIANT=minimal_debug_defconfig OUT_DIR=../out/msm-kernel-sa525-minimal_debug_defconfig ./build/build.sh  -j24 V=1 2>&1 | tee minimal_pvm_kernel_compile.log
     # For Userspace compilation
     $ export SHELL=/bin/bash && export MACHINE=sa525m && export DISTRO=qti-distro-tele-debug; source poky/qti-conf/set_bb_env.sh
     $ bitbake -R conf/machine/include/telematics-minimal.inc qti-tele-minimal

  4. For APQ:

    .. code-block::

     # For Kernel compilation
     $ cd src/kernel-5.15/kernel_platform/
     $ BUILD_CONFIG=msm-kernel/build.config.msm.sa525 EXTRA_CONFIGS=./prebuilts/qcom_boot_artifacts/build.config.qc.standalone VARIANT=debug_defconfig OUT_DIR=../out/msm-kernel-sa525-debug_defconfig ./build/build.sh  -j24 V=1 2>&1 | tee apq_kernel_compile.log
     # For Userspace compilation
     $ export SHELL=/bin/bash && export MACHINE=sa525m && export DISTRO=qti-distro-tele-debug ; source poky/qti-conf/set_bb_env.sh
     $ bitbake -R conf/machine/include/sa525m-apq.inc qti-tele-apq-image

Building a sample application
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The build environment is ready once set_bb_env.sh script has been run. The TelSDK supplied sample applications can be built by using telux-samples recipe as shown below.

.. code-block::

  $ bitbake telux-samples

------------------------------------------
You do not have access to ChipCode portal
------------------------------------------

It is possible to develop an application based on TelSDK up to a certain extent, using only publicly available part of the TelSDK source code. Minimum requirements and how they are met is discussed below:

1. An overall build environment which comprises of a cross-toolchain to compile application for a given QTI target processor's architecture. Standard Linux header files, libraries, root file system directory structure and various environment variables to facilitate build and link process. This build environment is obtained with the help of standard Yocto platform SDK application development and environment creation support feature.

2. The TelSDK header files and libraries to compile and link an application. This is done by enabling TelSDK specific recipes that builds and installs these header files and libraries in root file system used in our build environment.


Creating build environment
~~~~~~~~~~~~~~~~~~~~~~~~~~~

The steps to create such a build environment are as follows:

1. Install necessary packages on host machine to sync and build the Yocto platform SDK.

  .. code-block::

    $ sudo apt-get install repo gawk wget git-core diffstat unzip texinfo xterm
    $ sudo apt-get install gcc-multilib build-essential chrpath socat libsdl1.2-dev

2. Identify tag of the software image (SI) release from Qualcomm Technologies, Inc for the desired target processor from "https://git.codelinaro.org/clo/le/le/manifest/-/tags". LE.UM.7.4.1.1.C1 is the SI for SA525M.LE.3.0 target, LE.UM.7.4.1.C3 is the SI for SA525M.LE.1.0 target, LE.UM.7.1.3.c1 is the SI for SA515M target, LE.UM.3.2.1.C1 is the SI for SA415M target and LE.UM.1.3.r5 is SI for MDM9650 target.

3. Create fully qualified manifest file name for the source code repository. Add ".xml" as suffix to the tag obtained at step 2. Example manifest file name: AU_LINUX_EMBEDDED_LE.UM.7.4.1.1.C1_TARGET_ALL.01.681.068.xml

4. Sync the open-source code part of the software image release using manifest file obtained at step 3.

  .. code-block::

    $ repo init -u https://git.codelinaro.org/clo/le/le/manifest.git -b refs/tags/${AU_TAG} -m ${MANIFEST} --repo-url=https://git.codelinaro.org/clo/tools/repo.git --repo-branch=qc-stable
    $ repo sync -j8 --no-tags -c --no-clone-bundle --optimized-fetch

5. Enable building and installing TelSDK libraries on target image by appending following lines in poky/build/conf/local.conf file.

   a. For all software images except LE.UM.1.3.r5:

    .. code-block::

      CORE_IMAGE_EXTRA_INSTALL += "telux"
      CORE_IMAGE_EXTRA_INSTALL += "telux-lib"

   b. For LE.UM.1.3.r5 software image:

    .. code-block::

      CORE_IMAGE_EXTRA_INSTALL += "telux"
      CORE_IMAGE_EXTRA_INSTALL += "telephony-lib"

Note: This step is not applicable for SI's corresponding to SA525M SP's.

6. Run the SDK environment setup script (required by Yocto build system).

  .. code-block::

    $ cd poky
    $ source build/conf/set_bb_env.sh

7. Setup the machine and OS distribution environment.

  .. code-block::

    # For SA525M
    $ export MACHINE=sa525m
    $ export DISTRO=qti-distro-tele-debug
    # For sa515m
    $ export MACHINE=sa515m
    $ export DISTRO=auto
    # For sa415m
    $ export MACHINE=sa415m
    $ export DISTRO=auto
    # For sa2150p
    $ export MACHINE=sa2150p
    $ export DISTRO=msm

8. Build the Yocto platform SDK and generate SDK installer.

  .. code-block::

   $ bitbake core-image-minimal -c do_populate_sdk

  Once the build completes, the SDK installer will be found in poky/build/tmp-glibc/deploy/sdk directory. For example, oecore-x86_64-armv7at2hf-neon-toolchain-nodistro.0.sh is the installer for SA515M LE.2.1.

9. Finally, install the build environment on host machine using installer generated at step 8. When prompted specify directory for installation.

  .. code-block::

   $ poky/build/tmp-glibc/deploy/sdk/oecore-x86_64-<arch>-toolchain-nodistro.0.sh

10. An application can be now written and compiled against TelSDK libraries by following build procedure mentioned in Yocto SDK project manual here:
https://docs.yoctoproject.org/sdk-manual/index.html


Building a sample application
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Our build environment is setup now. As an example, TelSDK supplied location sample application for SA515M LE.2.1 can be built by executing following instructions.

.. code-block::

  $ cd <platform_sdk_installation_directory>
  $ source environment-setup-armv7at2hf-neon-oe-linux-gnueabi
  $ cd telux/public/apps/samples/loc/loc_app/
  $ ${CC} SampleLocationApp.cpp -ltelux_loc -std=c++11 -lstdc++ -o testapp
