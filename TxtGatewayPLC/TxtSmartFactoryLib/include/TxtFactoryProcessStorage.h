/*
 * TxtFactoryProcessStorage.h
 *
 *  Created on: 11.04.2019
 *      Author: steiger-a
 */

#ifndef TxtFactoryProcessStorage_H_
#define TxtFactoryProcessStorage_H_

#include <stdio.h>          // for printf()
#include <unistd.h>         // for sleep()
#include <iostream>
#include <thread>
#include <map>

#include "KeLibTxtDl.h"     // TXT Lib
#include "FtShmem.h"        // TXT Transfer Area

#include "TxtFactoryTypes.h"
#include "Utils.h"

#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"


namespace ft {


class TxtFactoryProcessStorage {
public:
	TxtFactoryProcessStorage();
	virtual ~TxtFactoryProcessStorage();

	void setTimestampNow(const std::string tag_uid, TxtHistoryIndex_t i);
	std::vector<int64_t> getTagUidVts(const std::string tag_uid);
	uint8_t getTagUidMaskTs(const std::string tag_uid);
	void resetTagUidMaskTs(const std::string tag_uid);

protected:
	void printMap();

	// map: tag_uid -> content
	std::map<std::string, std::vector<int64_t> > map_vts;
	std::map<std::string, uint8_t> map_mask_ts;
};


} /* namespace ft */


#endif /* TxtFactoryProcessStorage_H_ */
