/*
 * TxtDeliveryPickupStationCalibData.cpp
 *
 *  Created on: 08.11.2018
 *      Author: steiger-a
 */

#include "TxtDeliveryPickupStation.h"

#include "Utils.h"

#include <string>
#include <fstream>
#include <json/writer.h>
#include <json/reader.h>


namespace ft {


bool TxtDeliveryPickupStationCalibData::load()
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "loadCalib",0);

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
        const Json::Value colorsens = root["VGR"]["colorsens"];
    	color_th[0] = colorsens["th1"].asInt();
    	color_th[1] = colorsens["th2"].asInt();
    	std::cout <<  "colorsens th: " << color_th[0] << ", " << color_th[1] << std::endl;

        const Json::Value actions = root["VGR"]["uid_actions"];
    	uid_actions[0] = actions["calib_mode"].asString();
    	uid_actions[1] = actions["reset_hbw"].asString();
    	uid_actions[2] = actions["order_white"].asString();
    	uid_actions[3] = actions["order_red"].asString();
    	uid_actions[4] = actions["order_blue"].asString();
    	std::cout <<  "action uids: " << uid_actions[0] << ", " << uid_actions[1] << uid_actions[2] << ", " << uid_actions[3] << uid_actions[4] << std::endl;

		valid = true;
    	return true;
    }
	return false;
}

bool TxtDeliveryPickupStationCalibData::saveDefault()
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "saveDefault",0);
    //white=450, red=1250, blue=1700
	color_th[0] = 850;
	color_th[1] = 1550;
	uid_actions[0] = "";
	uid_actions[1] = "";
	uid_actions[2] = "";
	uid_actions[3] = "";
	uid_actions[4] = "";
	return save();
}

bool TxtDeliveryPickupStationCalibData::save()
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "save",0);
	Json::Value event;

    event["VGR"]["colorsens"]["th1"] = color_th[0];
    event["VGR"]["colorsens"]["th2"] = color_th[1];

    event["VGR"]["uid_actions"]["calib_mode"] =  uid_actions[0];
    event["VGR"]["uid_actions"]["reset_hbw"] =   uid_actions[1];
    event["VGR"]["uid_actions"]["order_white"] = uid_actions[2];
    event["VGR"]["uid_actions"]["order_red"] =   uid_actions[3];
    event["VGR"]["uid_actions"]["order_blue"] =  uid_actions[4];

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
