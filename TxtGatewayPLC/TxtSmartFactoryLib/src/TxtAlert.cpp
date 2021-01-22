/*
 * TxtAlert.cpp
 *
 *  Created on: 26.04.2018
 *      Author: steiger-a
 */

#include "TxtAlert.h"

#include "Utils.h"


namespace ft {


TxtFlapping::TxtFlapping()
	: stateChangeBuf(0), flappingIndex(0), flapping(false),
	  flappingThresholdLow(0.25), flappingThresholdHigh(0.30), flappingValue(0.0),
	  rawtimeLastChange(0)
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "");
}

TxtFlapping::~TxtFlapping()
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "");
}

void TxtFlapping::UpdateFlappingStatus(bool stateChange)
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "{}",stateChange);

	int oldestIndex = this->flappingIndex;

	stateChangeBuf.Modify(oldestIndex, stateChange);
	oldestIndex = (oldestIndex + 1) % 20;

	double stateChanges = 0;

	/* Iterate over our state array and compute a weighted total */
	for (int i = 0; i < 20; i++) {
		if (stateChangeBuf.Get((oldestIndex + i) % 20))
			stateChanges += 0.8 + (0.02 * i);
	}

	double flappingValue = 100.0 * stateChanges / 20.0;

	bool flapping;

	if (this->flapping)
		flapping = flappingValue > flappingThresholdLow;
	else
		flapping = flappingValue > flappingThresholdHigh;

	this->flappingIndex = oldestIndex;
	this->flappingValue = flappingValue;
	this->flapping = flapping;

	//double rawtimeLastChange = ft::getnowtimestamp_s(); //UTC
}


} /* namespace ft */
