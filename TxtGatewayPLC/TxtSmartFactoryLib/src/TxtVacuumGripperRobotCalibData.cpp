/*
 * TxtVacuumGripperRobotCalibData.cpp
 *
 *  Created on: 08.11.2018
 *      Author: steiger-a
 */

#include "TxtVacuumGripperRobot.h"

#include "Utils.h"

#include <string>
#include <fstream>
#include <json/writer.h>
#include <json/reader.h>


namespace ft {


bool TxtVacuumGripperRobotCalibData::load()
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
        const Json::Value pos3list = root["VGR"]["pos3list"];
		if (pos3list.isArray())
		{

			for ( Json::Value::ArrayIndex i = 0; i < pos3list.size(); ++i )
			{
			    for (Json::Value::const_iterator it=pos3list[i].begin(); it!=pos3list[i].end(); ++it)
			    {
			    	std::string key = it.key().asString();
					if (!key.empty())
					{
						EncPos3 pos3;
						pos3.x = pos3list[i][key]["x"].asInt();
						pos3.y = pos3list[i][key]["y"].asInt();
						pos3.z = pos3list[i][key]["z"].asInt();
						map_pos3[key] = pos3;
						std::cout << key  << " : "
								<< pos3.x << ", "
								<< pos3.y << ", "
								<< pos3.z << std::endl;
					}
			    }
			}
		}

		valid = true;
    	return true;
    }
	return false;
}

bool TxtVacuumGripperRobotCalibData::saveDefault()
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "saveDefault",0);

	setPos3("DIN0", EncPos3(22, 600, 19));
	setPos3("DIN", EncPos3(22, 758, 19));

	setPos3("WDC0", EncPos3(300, 400, 0));
	setPos3("WDC", EncPos3(300, 600, 0));

	setPos3("DCS0", EncPos3(123, 500, 65));
	setPos3("DCS", EncPos3(123, 645, 75));

	setPos3("DNFC0", EncPos3(200, 600, 250));
	setPos3("DNFC", EncPos3(200, 643, 260));

	setPos3("DOUT0", EncPos3(263, 50, 280));
	setPos3("DOUT", EncPos3(263, 320, 590));
	setPos3("DOUT1", EncPos3(255, 460, 585));

	setPos3("HBW0", EncPos3(1410, 0, 0));
	setPos3("HBW", EncPos3(1410, 50, 186));
	setPos3("HBW1", EncPos3(1410, 170, 186));

	setPos3("MPO0", EncPos3(931, 0, 880));
	setPos3("MPO", EncPos3(931, 490, 934));

	setPos3("SSD10", EncPos3(469, 300, 280));
	setPos3("SSD1", EncPos3(469, 845, 380));

	setPos3("SSD20", EncPos3(390, 300, 280));
	setPos3("SSD2", EncPos3(390, 845, 434));

	setPos3("SSD30", EncPos3(316, 300, 280));
	setPos3("SSD3", EncPos3(316, 845, 588));

	return save();
}

bool TxtVacuumGripperRobotCalibData::save()
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "save",0);
	Json::Value event;
	int i = 0;
    event["VGR"]["pos3list"][i]["DIN0"]["x"] =   map_pos3["DIN0"].x;
    event["VGR"]["pos3list"][i]["DIN0"]["y"] =   map_pos3["DIN0"].y;
    event["VGR"]["pos3list"][i++]["DIN0"]["z"] = map_pos3["DIN0"].z;
    event["VGR"]["pos3list"][i]["DIN"]["x"] =    map_pos3["DIN"].x;
    event["VGR"]["pos3list"][i]["DIN"]["y"] =    map_pos3["DIN"].y;
    event["VGR"]["pos3list"][i++]["DIN"]["z"] =  map_pos3["DIN"].z;

    event["VGR"]["pos3list"][i]["WDC0"]["x"] =   map_pos3["WDC0"].x;
    event["VGR"]["pos3list"][i]["WDC0"]["y"] =   map_pos3["WDC0"].y;
    event["VGR"]["pos3list"][i++]["WDC0"]["z"] = map_pos3["WDC0"].z;
    event["VGR"]["pos3list"][i]["WDC"]["x"] =    map_pos3["WDC"].x;
    event["VGR"]["pos3list"][i]["WDC"]["y"] =    map_pos3["WDC"].y;
    event["VGR"]["pos3list"][i++]["WDC"]["z"] =  map_pos3["WDC"].z;

    event["VGR"]["pos3list"][i]["DCS0"]["x"] =   map_pos3["DCS0"].x;
    event["VGR"]["pos3list"][i]["DCS0"]["y"] =   map_pos3["DCS0"].y;
    event["VGR"]["pos3list"][i++]["DCS0"]["z"] = map_pos3["DCS0"].z;
    event["VGR"]["pos3list"][i]["DCS"]["x"] =    map_pos3["DCS"].x;
    event["VGR"]["pos3list"][i]["DCS"]["y"] =    map_pos3["DCS"].y;
    event["VGR"]["pos3list"][i++]["DCS"]["z"] =  map_pos3["DCS"].z;

    event["VGR"]["pos3list"][i]["DNFC0"]["x"] =   map_pos3["DNFC0"].x;
    event["VGR"]["pos3list"][i]["DNFC0"]["y"] =   map_pos3["DNFC0"].y;
    event["VGR"]["pos3list"][i++]["DNFC0"]["z"] = map_pos3["DNFC0"].z;
    event["VGR"]["pos3list"][i]["DNFC"]["x"] =    map_pos3["DNFC"].x;
    event["VGR"]["pos3list"][i]["DNFC"]["y"] =    map_pos3["DNFC"].y;
    event["VGR"]["pos3list"][i++]["DNFC"]["z"] =  map_pos3["DNFC"].z;

    event["VGR"]["pos3list"][i]["DOUT0"]["x"] =   map_pos3["DOUT0"].x;
    event["VGR"]["pos3list"][i]["DOUT0"]["y"] =   map_pos3["DOUT0"].y;
    event["VGR"]["pos3list"][i++]["DOUT0"]["z"] = map_pos3["DOUT0"].z;
    event["VGR"]["pos3list"][i]["DOUT"]["x"] =    map_pos3["DOUT"].x;
    event["VGR"]["pos3list"][i]["DOUT"]["y"] =    map_pos3["DOUT"].y;
    event["VGR"]["pos3list"][i++]["DOUT"]["z"] =  map_pos3["DOUT"].z;
    event["VGR"]["pos3list"][i]["DOUT1"]["x"] =    map_pos3["DOUT1"].x;
    event["VGR"]["pos3list"][i]["DOUT1"]["y"] =    map_pos3["DOUT1"].y;
    event["VGR"]["pos3list"][i++]["DOUT1"]["z"] =  map_pos3["DOUT1"].z;

    event["VGR"]["pos3list"][i]["HBW0"]["x"] =    map_pos3["HBW0"].x;
    event["VGR"]["pos3list"][i]["HBW0"]["y"] =    map_pos3["HBW0"].y;
    event["VGR"]["pos3list"][i++]["HBW0"]["z"] =  map_pos3["HBW0"].z;
    event["VGR"]["pos3list"][i]["HBW"]["x"] =     map_pos3["HBW"].x;
    event["VGR"]["pos3list"][i]["HBW"]["y"] =     map_pos3["HBW"].y;
    event["VGR"]["pos3list"][i++]["HBW"]["z"] =   map_pos3["HBW"].z;
    event["VGR"]["pos3list"][i]["HBW1"]["x"] =    map_pos3["HBW1"].x;
    event["VGR"]["pos3list"][i]["HBW1"]["y"] =    map_pos3["HBW1"].y;
    event["VGR"]["pos3list"][i++]["HBW1"]["z"] =  map_pos3["HBW1"].z;

    event["VGR"]["pos3list"][i]["MPO0"]["x"] =    map_pos3["MPO0"].x;
    event["VGR"]["pos3list"][i]["MPO0"]["y"] =    map_pos3["MPO0"].y;
    event["VGR"]["pos3list"][i++]["MPO0"]["z"] =  map_pos3["MPO0"].z;
    event["VGR"]["pos3list"][i]["MPO"]["x"] =     map_pos3["MPO"].x;
    event["VGR"]["pos3list"][i]["MPO"]["y"] =     map_pos3["MPO"].y;
    event["VGR"]["pos3list"][i++]["MPO"]["z"] =   map_pos3["MPO"].z;

    event["VGR"]["pos3list"][i]["SSD10"]["x"] =   map_pos3["SSD10"].x;
    event["VGR"]["pos3list"][i]["SSD10"]["y"] =   map_pos3["SSD10"].y;
    event["VGR"]["pos3list"][i++]["SSD10"]["z"] = map_pos3["SSD10"].z;
    event["VGR"]["pos3list"][i]["SSD1"]["x"] =    map_pos3["SSD1"].x;
    event["VGR"]["pos3list"][i]["SSD1"]["y"] =    map_pos3["SSD1"].y;
    event["VGR"]["pos3list"][i++]["SSD1"]["z"] =  map_pos3["SSD1"].z;

    event["VGR"]["pos3list"][i]["SSD20"]["x"] =   map_pos3["SSD20"].x;
    event["VGR"]["pos3list"][i]["SSD20"]["y"] =   map_pos3["SSD20"].y;
    event["VGR"]["pos3list"][i++]["SSD20"]["z"] = map_pos3["SSD20"].z;
    event["VGR"]["pos3list"][i]["SSD2"]["x"] =    map_pos3["SSD2"].x;
    event["VGR"]["pos3list"][i]["SSD2"]["y"] =    map_pos3["SSD2"].y;
    event["VGR"]["pos3list"][i++]["SSD2"]["z"] =  map_pos3["SSD2"].z;

    event["VGR"]["pos3list"][i]["SSD30"]["x"] =   map_pos3["SSD30"].x;
    event["VGR"]["pos3list"][i]["SSD30"]["y"] =   map_pos3["SSD30"].y;
    event["VGR"]["pos3list"][i++]["SSD30"]["z"] = map_pos3["SSD30"].z;
    event["VGR"]["pos3list"][i]["SSD3"]["x"] =    map_pos3["SSD3"].x;
    event["VGR"]["pos3list"][i]["SSD3"]["y"] =    map_pos3["SSD3"].y;
    event["VGR"]["pos3list"][i++]["SSD3"]["z"] =  map_pos3["SSD3"].z;

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
