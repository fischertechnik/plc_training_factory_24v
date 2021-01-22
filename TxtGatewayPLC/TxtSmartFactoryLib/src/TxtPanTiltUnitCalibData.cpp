/*
 * TxtPanTiltUnitCalibData.cpp
 *
 *  Created on: 12.04.2019
 *      Author: steiger-a
 */

#include "TxtPanTiltUnit.h"

#include "TxtMqttFactoryClient.h"
#include "Utils.h"

#include <string>
#include <fstream>
#include <json/writer.h>
#include <json/reader.h>


#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"


namespace ft {


bool TxtPanTiltUnitCalibData::load()
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
        const Json::Value jposCenterPan = root["SSC"]["posCenterPan"];
        const Json::Value jposCenterTilt = root["SSC"]["posCenterTilt"];
        posCenterPan = jposCenterPan.asInt();
        posCenterTilt = jposCenterTilt.asInt();
		std::cout <<  "posCenter: " << posCenterPan << ", " << posCenterTilt << std::endl;

		const Json::Value jposEndPan = root["SSC"]["posEndPan"];
        const Json::Value jposEndTilt = root["SSC"]["posEndTilt"];
        posEndPan = jposEndPan.asInt();
        posEndTilt = jposEndTilt.asInt();
		std::cout <<  "posEnd: " << posEndPan << ", " << posEndTilt << std::endl;

		const Json::Value jposHBWPan = root["SSC"]["posHBWPan"];
        const Json::Value jposHBWTilt = root["SSC"]["posHBWTilt"];
        posHBWPan = jposHBWPan.asInt();
        posHBWTilt = jposHBWTilt.asInt();
		std::cout <<  "posHBW: " << posHBWPan << ", " << posHBWTilt << std::endl;

		valid = true;
    	return true;
    }
	return false;
}

bool TxtPanTiltUnitCalibData::saveDefault()
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "saveDefault",0);

	posCenterPan = 925;
	posCenterTilt = 425;
	posEndPan = 1500;
	posEndTilt = 700;
	posHBWPan = 1450;
	posHBWTilt = 290;

	return save();
}

bool TxtPanTiltUnitCalibData::save()
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "save",0);
	Json::Value event;
    event["SSC"]["posCenterPan"]  = posCenterPan;
    event["SSC"]["posCenterTilt"] = posCenterTilt;
    event["SSC"]["posEndPan"]     = posEndPan;
    event["SSC"]["posEndTilt"]    = posEndTilt;
    event["SSC"]["posHBWPan"]     = posHBWPan;
    event["SSC"]["posHBWTilt"]    = posHBWTilt;

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
