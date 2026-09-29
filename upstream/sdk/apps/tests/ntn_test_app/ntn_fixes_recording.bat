:: Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
:: SPDX-License-Identifier: BSD-3-Clause-Clear
:: Usage:
::    a. By default, this script captures detailed location reports from FUSED engine
::    at every 1000ms.
::    b. Detailed engine reports can be captured by passing following arguments or any combinations
::       to option '-n':
::       for example:
::       location_test_app -n
::       Captures the location reports for the required NTN location fields at 1000ms.

@echo off
IF "%~1"=="" (
    echo Please provide the path to the CSV file.
    exit /b 1
)

set "CsvFilePath=%~1"

(
echo lat,lon,alt,uncerCircular,isEnuValueValid,enuEastingVel,enuNorthingVel,enuUpwardVel,isEnuUncerValid,enuEastingUncer,enuNorthingUncer,enuUpwardUncer,isHeadingValid,heading,isHeadingUncerValid,headingUncer,isConfidenceValid,confidence
) > %CsvFilePath%

adb shell " location_test_app -n | grep '^###' | sed 's/\#\#\#//g' " >> %CsvFilePath%