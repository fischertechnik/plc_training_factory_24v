/*
 * TxtMultiProcessingStationCalibData.cpp
 *
 *  Created on: 23.10.2019
 *      Author: steiger-a
 */

#include "TxtMultiProcessingStation.h"

#include "TxtMqttFactoryClient.h"
#include "Utils.h"

#include <string>
#include <fstream>
#include <json/writer.h>
#include <json/reader.h>


#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"


namespace ft {


bool TxtMultiProcessingStationCalibData::load()
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "load",0);

    std::ifstream infile(filename.c_str());
    if ( infile.good())
    {
    	//load file
        Json::Value root;
        Json::CharReaderBuilder builder;
        std::string errs;
        std::ifstream test(filename.c_str(), std::ifstream::binary);
        if (test.is_open()) {
            std::cout << "load file " << filename << std::endl;
            bool ok = Json::parseFromStream(builder, test, &root, &errs);
            if ( !ok )
            {
                std::cout  << "error: " << errs << std::endl;
                return false;
            }
        }
        const Json::Value colorsens = root["MPO"]["colorsens"];

		valid = true;
    	return true;
    }
	return false;
}

bool TxtMultiProcessingStationCalibData::saveDefault()
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "saveDefault",0);
	return save();
}

bool TxtMultiProcessingStationCalibData::save()
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "save",0);
	Json::Value event;

    Json::StreamWriterBuilder builder;
    builder["commentStyle"] = "None";
    builder["indentation"] = " ";

    std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter());
    std::ofstream outputFileStream(filename.c_str(), std::ios_base::out);
    if(!outputFileStream.is_open())
	{
    	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "filename {} is not opened!",filename.c_str());
    	return false;
	}
    return (writer->write(event, &outputFileStream) == 0);
}


} /* namespace ft */
