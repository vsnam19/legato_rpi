/*
 *  Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */


#include "JsonParser.hpp"
#include "Logger.hpp"
#include "FileInfo.hpp"

#include <fstream>
#include <sys/stat.h>

std::mutex JsonParser::fileMutex_;

telux::common::ErrorCode JsonParser::readFromJsonFile(Json::Value &rootNode,
        std::string path) {
    telux::common::ErrorCode error = telux::common::ErrorCode::SUCCESS;
    std::lock_guard<std::mutex> lk(JsonParser::fileMutex_);

    std::string filePath = std::string(DEFAULT_JSON_FILE_PATH) + path;
    LOG(DEBUG, "Trying to read: ", filePath);
    std::ifstream ifs(filePath);

    if (!ifs.good()) {
        filePath = std::string(DEFAULT_SIM_FILE_PREFIX)
            + std::string(DEFAULT_JSON_FILE_PATH) + path;
        ifs.open(filePath);
        LOG(DEBUG, "ReTrying to read: ", filePath);
        if (!ifs.good())
        {
            LOG(ERROR, "Failed to open Json file");
        }
    }

    try {
        ifs >> rootNode;
    } catch (std::exception &e) {
        LOG(ERROR, "Parsing the json file failed with ", e.what());
        error = telux::common::ErrorCode::INTERNAL_ERR;
    }
    ifs.close();
    return error;
}

telux::common::ErrorCode JsonParser::writeToJsonFile(Json::Value rootNode,
        std::string path) {
    telux::common::ErrorCode error = telux::common::ErrorCode::SUCCESS;

    std::lock_guard<std::mutex> lk(JsonParser::fileMutex_);
    std::string filePath = std::string(DEFAULT_JSON_FILE_PATH) + path;

    std::ofstream ofs(filePath);
    if (!ofs.good()) {
        filePath = std::string(DEFAULT_SIM_FILE_PREFIX)
            + std::string(DEFAULT_JSON_FILE_PATH) + path;
        // Create the directory if it doesn't exist
        size_t pos = filePath.find_last_of('/');
        if (pos != std::string::npos) {
            std::string dirPath = filePath.substr(0, pos);
            if (mkdir(dirPath.c_str(), 0777) == -1) {
                if (errno != EEXIST) {
                    LOG(ERROR, "Failed to create directory");
                    error = telux::common::ErrorCode::INTERNAL_ERR;
                    return error;
                }
            }
        }
        ofs.open(filePath);
        if (!ofs.good())
        {
            LOG(ERROR, "Failed to open Json file");
        }
    }
    ofs << rootNode;
    ofs.close();
    return error;
}