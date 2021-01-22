/*
 * TxtVacuumGripper.cpp
 *
 *  Created on: 07.02.2019
 *      Author: steiger-a
 */

#include "TxtVacuumGripper.h"

#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"


namespace ft {


TxtVacuumGripper::TxtVacuumGripper(TxtTransfer* pT, uint8_t chComp, uint8_t chValve)
	: pT(pT), chComp(chComp), chValve(chValve)
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "TxtVacuumGripper chComp:{} chValve:{}",  chComp, chValve);
}

TxtVacuumGripper::~TxtVacuumGripper()
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "~TxtVacuumGripper", 0);
}

void TxtVacuumGripper::grip()
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "grip", 0);
	setCompressor(true);
	std::this_thread::sleep_for(std::chrono::milliseconds(2000));
	if (chValve < 8)
	{
		assert(pT->pTArea);
		pT->pTArea->ftX1out.duty[chValve] = 512; // Switch on with PWM Value 512 (= max speed)
	}
	else
	{
		assert(pT->pTArea+1);
		(pT->pTArea+1)->ftX1out.duty[chValve-8] = 512; // Switch on with PWM Value 512 (= max speed)
	}
}

void TxtVacuumGripper::release()
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "release", 0);
	if (chValve < 8)
	{
		assert(pT->pTArea);
		pT->pTArea->ftX1out.duty[chValve] = 0; // Switch on with PWM Value 512 (= max speed)
	}
	else
	{
		assert(pT->pTArea+1);
		(pT->pTArea+1)->ftX1out.duty[chValve-8] = 0; // Switch on with PWM Value 512 (= max speed)
	}
	setCompressor(false);
}

void TxtVacuumGripper::setCompressor(bool on)
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "setCompressor {}", on);
	if (chComp < 8)
	{
		assert(pT->pTArea);
		pT->pTArea->ftX1out.duty[chComp] = on ? 512 : 0; // Switch on with PWM Value 512 (= max speed)
	}
	else
	{
		assert(pT->pTArea+1);
		(pT->pTArea+1)->ftX1out.duty[chComp-8] = on ? 512 : 0; // Switch on with PWM Value 512 (= max speed)
	}
}


} /* namespace ft */
