#
# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear
#

# Usage:
#    a. By default, this script captures detailed location reports from FUSED engine
#    at every 1000ms.
#    b. Detailed engine reports can be captured by passing following arguments or any combinations
#       to option '-n':
#       for example:
#       location_test_app -n
#       Captures the location reports for the required NTN location fields at 1000ms.

# Check if the file path is provided
if [ -z "$1" ]; then
    echo "Please provide the path to the CSV file."
    exit 1
fi

# Define the path to the CSV file
csv_file="$1"

# Clearing the csv before adding copyrights
> "$csv_file"

# Adding loc fields headers to the csv
echo "lat,lon,alt,uncerCircular,isEnuValueValid,enuEastingVel,enuNorthingVel,enuUpwardVel,isEnuUncerValid,enuEastingUncer,enuNorthingUncer,enuUpwardUncer,isHeadingValid,heading,isHeadingUncerValid,headingUncer,isConfidenceValid,confidence" >> "$csv_file"

adb shell " location_test_app -n | grep '^###' | sed 's/\#\#\#//g' " >> "$csv_file"
