/*
 * TxtHighBayWarehouseCalibData.cpp
 *
 *  Created on: 07.02.2019
 *      Author: steiger-a
 */

#include "TxtHighBayWarehouse.h"

#include "TxtMqttFactoryClient.h"
#include "Utils.h"

#include <string>
#include <json/writer.h>
#include <json/reader.h>


namespace ft {


bool TxtHighBayWarehouseCalibData::load()
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
        const Json::Value val_hbw = root["HBW"];
        const Json::Value val_hbx = val_hbw["hbx"];
        hbx[0] = val_hbx["1"].asInt();
        hbx[1] = val_hbx["2"].asInt();
        hbx[2] = val_hbx["3"].asInt();
        const Json::Value val_hby = val_hbw["hby"];
        hby[0] = val_hby["1"].asInt();
        hby[1] = val_hby["2"].asInt();
        hby[2] = val_hby["3"].asInt();
        const Json::Value val_conv = val_hbw["conv"];
        EncPos2 c;
        c.x = val_conv["x"].asInt();
        c.y = val_conv["y"].asInt();
        conv = c;
		std::cout << "hbx : "
				<< hbx[0] << ", "
				<< hbx[1]<< ", "
				<< hbx[2]<< std::endl;
		std::cout << "hby : "
				<< hby[0] << ", "
				<< hby[1]<< ", "
				<< hby[2]<< std::endl;
		std::cout << "conv : "
				<< conv.x << ", "
				<< conv.y << std::endl;

		valid = true;
    	return true;
    }
	return false;
}

bool TxtHighBayWarehouseCalibData::saveDefault()
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "saveDefault",0);

	hbx[0] = 780;
	hbx[1] = 1390;
	hbx[2] = 1995;

	hby[0] = 80;
	hby[1] = 445;
	hby[2] = 855;

	conv = EncPos2(20, 720);

	return save();
}

bool TxtHighBayWarehouseCalibData::save()
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "save",0);
	Json::Value event;

	event["HBW"]["hbx"]["1"] = hbx[0];
    event["HBW"]["hbx"]["2"] = hbx[1];
    event["HBW"]["hbx"]["3"] = hbx[2];
    event["HBW"]["hby"]["1"] = hby[0];
    event["HBW"]["hby"]["2"] = hby[1];
    event["HBW"]["hby"]["3"] = hby[2];
    event["HBW"]["conv"]["x"] = conv.x;
    event["HBW"]["conv"]["y"] = conv.y;

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
