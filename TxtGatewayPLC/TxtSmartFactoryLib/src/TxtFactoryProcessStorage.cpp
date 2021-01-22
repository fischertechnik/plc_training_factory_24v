/*
 * TxtFactoryProcessStorage.cpp
 *
 *  Created on: 11.04.2019
 *      Author: steiger-a
 */

#include "TxtFactoryProcessStorage.h"

#include "TxtNfcDevice.h"


namespace ft {


TxtFactoryProcessStorage::TxtFactoryProcessStorage()
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "TxtFactoryProcessStorage",0);
}

TxtFactoryProcessStorage::~TxtFactoryProcessStorage()
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "~TxtFactoryProcessStorage",0);
}

void TxtFactoryProcessStorage::setTimestampNow(const std::string tag_uid, TxtHistoryIndex_t i)
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "setTimestampNow",0);
	if (tag_uid.empty())
	{
    	printf("uid: %s\n",tag_uid.c_str());
    	//TODO check empty
		//exit(1);
	}
	auto search = map_vts.find(tag_uid);
    if (search != map_vts.end())
	{
		std::vector<int64_t> vts = map_vts[tag_uid];
		vts[(int)i] = ft::getnowtimestamp_s()*1000000000.;
		map_vts[tag_uid] = vts;

		uint8_t mask_ts = map_mask_ts[tag_uid];
		mask_ts |= 0x01 << (int)i;
		map_mask_ts[tag_uid] = mask_ts;
	}
    else
    {
		std::vector<int64_t> vts;
    	for(unsigned int i = 0; i < NUM_INDEX_MAX; i++)
    	{
    		vts.push_back(0);
    	}
    	vts[(int)i] = ft::getnowtimestamp_s()*1000000000.;
		map_vts[tag_uid] = vts;

    	uint8_t mask_ts = 0x0;
		mask_ts |= 0x01 << (int)i;
		map_mask_ts[tag_uid] = mask_ts;
    }
	//printMap();
}

std::vector<int64_t> TxtFactoryProcessStorage::getTagUidVts(const std::string tag_uid)
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "getTagUidVts",0);
	std::vector<int64_t> vts;
	auto search = map_vts.find(tag_uid);
    if (search != map_vts.end())
	{
		vts = map_vts[tag_uid];
	} else {
		//exit(1);
	}
	//printMap();
	return vts;
}

uint8_t TxtFactoryProcessStorage::getTagUidMaskTs(const std::string tag_uid)
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "getTagUidMaskTs",0);
	uint8_t mask_ts = 0x0;
	auto search = map_mask_ts.find(tag_uid);
    if (search != map_mask_ts.end())
	{
    	mask_ts = map_mask_ts[tag_uid];
	} else {
		//exit(1);
	}
	//printMap();
	return mask_ts;
}

void TxtFactoryProcessStorage::resetTagUidMaskTs(const std::string tag_uid)
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "resetTagUidMaskTs",0);
	map_mask_ts[tag_uid] = 0x0;
	//printMap();
}

void TxtFactoryProcessStorage::printMap()
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "printMap",0);
    for (std::map<std::string, std::vector<int64_t> > ::const_iterator it=map_vts.begin(); it!=map_vts.end(); ++it)
    {
    	std::string uid = it->first;
    	printf("uid: %s\n",uid.c_str());
    	std::vector<int64_t> vts = it->second;
    	for(unsigned int i = 0; i < vts.size(); i++ )
    	{
    		int64_t ts = vts[i];
	    	printf("  %d: %lld\n",i,ts);
    	}
    }
    for (std::map<std::string, uint8_t> ::const_iterator it=map_mask_ts.begin(); it!=map_mask_ts.end(); ++it)
    {
    	std::string uid = it->first;
    	printf("uid: %s\n",uid.c_str());
    	uint8_t mask_ts = it->second;
    	printf("  %d\n",mask_ts);
    }
}


} /* namespace ft */
