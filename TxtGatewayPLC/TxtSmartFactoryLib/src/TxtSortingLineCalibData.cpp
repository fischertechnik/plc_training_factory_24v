/*
 * TxtSortingLineCalibData.cpp
 *
 *  Created on: 03.04.2019
 *      Author: steiger-a
 */

#include "TxtSortingLine.h"

#include "TxtMqttFactoryClient.h"
#include "Utils.h"

#include <string>
#include <fstream>
#include <json/writer.h>
#include <json/reader.h>


#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"


namespace ft {


bool TxtSortingLineCalibData::load()
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
        const Json::Value colorsens = root["SLD"]["colorsens"];

    	color_th[0] = colorsens["th1"].asInt();
    	color_th[1] = colorsens["th2"].asInt();
		std::cout <<  "colorsens th: " << color_th[0] << ", " << color_th[1] << std::endl;

		count_white = root["SLD"]["count"]["white"].asInt();
		count_red = root["SLD"]["count"]["red"].asInt();
		count_blue = root["SLD"]["count"]["blue"].asInt();
		std::cout <<  "count w,r,b: " << count_white << ", " << count_red  << ", " << count_blue << std::endl;

		valid = true;
    	return true;
    }
	return false;
}

bool TxtSortingLineCalibData::saveDefault()
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "saveDefault",0);

    //white=564, red=1320, blue=1540
	color_th[0] = 940;
	color_th[1] = 1430;

	count_white = 5;
	count_red = 15;
	count_blue = 26;

	return save();
}

bool TxtSortingLineCalibData::save()
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "save",0);
	Json::Value event;

	event["SLD"]["colorsens"]["th1"] = color_th[0];
    event["SLD"]["colorsens"]["th2"] = color_th[1];

    event["SLD"]["count"]["white"] = count_white;
    event["SLD"]["count"]["red"] = count_red;
    event["SLD"]["count"]["blue"] = count_blue;

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
