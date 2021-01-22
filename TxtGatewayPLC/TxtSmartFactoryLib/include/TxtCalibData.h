/*
 * TxtCalibData.h
 *
 *  Created on: 18.02.2019
 *      Author: wrk
 */

#ifndef TXTCALIBDATA_H_
#define TXTCALIBDATA_H_


#include <string>
#include <fstream>

#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"

namespace ft {


class TxtCalibData {
public:
	TxtCalibData(std::string filename);
	virtual ~TxtCalibData();

	virtual bool load() = 0;
	virtual bool saveDefault() = 0;
	virtual bool save() = 0;

	bool isValid() { return valid; }
	bool existCalibFilename();

protected:
	std::string filename;
	bool valid;
};


} /* namespace ft */


#endif /* TXTCALIBDATA_H_ */
