/*
 * TxtAxis.cpp
 *
 *  Created on: 08.11.2018
 *      Author: steiger-a
 */

#include "TxtAxis.h"

#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"


namespace ft {


TxtAxis::TxtAxis(std::string name, TxtTransfer* pT, uint8_t chM, uint8_t chS1)
	: name(name), pT(pT), status(AXIS_NOREF), speed(512), stopReq(false), chM(chM), chS1(chS1)
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console_axes"), "{} TxtAxis chM:{} chS1:{}",name,chM,chS1);
	setStatus(AXIS_NOREF);
}

TxtAxis::~TxtAxis() {
	SPDLOG_LOGGER_TRACE(spdlog::get("console_axes"), "{} ~TxtAxis",name);
	setMotorOff();
}

void TxtAxis::setSpeed(int16_t s) {
	SPDLOG_LOGGER_TRACE(spdlog::get("console_axes"), "{} setSpeed:{}",name,s);
	speed=s;
}

void TxtAxis::stop() {
	SPDLOG_LOGGER_TRACE(spdlog::get("console_axes"), "{} stop",name);
	stopReq = true;
}

void TxtAxis::configInputs(uint8_t chS)
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "{} configInputs chS:{}", name, chS);
	if ((chS<0) || (chS>15))
	{
		std::cout << "chS out of range master:[0-7] extension:[8-15]!" << std::endl;
		spdlog::get("file_logger")->error("chS out of range master:[0-7] extension:[8-15]!",0);
		exit(1);
	}
	if (chS < 8)
	{
		assert(pT->pTArea);
		pT->pTArea->ftX1config.uni[chS].mode = MODE_R; // Digital Switch with PullUp resistor
		pT->pTArea->ftX1config.uni[chS].digital = 1;
		pT->pTArea->ftX1state.config_id ++; // Save the new Setup
	}
	else
	{
		assert(pT->pTArea+1);
		(pT->pTArea+1)->ftX1config.uni[chS-8].mode = MODE_R; // Digital Switch with PullUp resistor
		(pT->pTArea+1)->ftX1config.uni[chS-8].digital = 1;
		(pT->pTArea+1)->ftX1state.config_id ++; // Save the new Setup
	}
}

bool TxtAxis::isSwitchPressed(uint8_t chS)
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "{} isSwitchPressed chS:{}", name, chS);
	bool ret = (chS<8?pT->pTArea->ftX1in.uni[chS]:(pT->pTArea+1)->ftX1in.uni[chS-8]) == 1;
	return ret;
}

void TxtAxis::setStatus(TxtAxis_status_t st) {
	SPDLOG_LOGGER_TRACE(spdlog::get("console_axes"), "{} setStatus:{}",name,st);
	status=st;
	std::string sst = toString(status);
	SPDLOG_LOGGER_DEBUG(spdlog::get("console_axes"), "{} setStatus:{}",name,sst);
}

void TxtAxis::setMotorOff()
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console_axes"), "{}({}) setMotorOff",name,status);
	if (chM < 8)
	{
		assert(pT->pTArea);
		pT->pTArea->ftX1out.duty[chM*2] = 0;
		pT->pTArea->ftX1out.duty[chM*2+1] = 0;
	}
	else
	{
		assert(pT->pTArea+1);
		(pT->pTArea+1)->ftX1out.duty[(chM-8)*2] = 0;
		(pT->pTArea+1)->ftX1out.duty[(chM-8)*2+1] = 0;
	}
}

void TxtAxis::setMotorLeft()
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console_axes"), "{}({}) setMotorLeft",name,status);
	assert(chS1<8?pT->pTArea:pT->pTArea+1);
	if (chS1<8?pT->pTArea->ftX1in.uni[chS1]:(pT->pTArea+1)->ftX1in.uni[chS1-8] == 1)
	{
		setMotorOff();
	}
	else
	{
		if (chM < 8)
		{
			assert(pT->pTArea);
			pT->pTArea->ftX1out.duty[chM*2] = speed;
			pT->pTArea->ftX1out.duty[chM*2+1] = 0;
		}
		else
		{
			assert(pT->pTArea+1);
			(pT->pTArea+1)->ftX1out.duty[(chM-8)*2] = speed;
			(pT->pTArea+1)->ftX1out.duty[(chM-8)*2+1] = 0;
		}
	}
}

void TxtAxis::setMotorRight()
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console_axes"), "{}({}) setMotorRight",name,status);
	if (chM < 8)
	{
		assert(pT->pTArea);
		pT->pTArea->ftX1out.duty[chM*2] = 0;
		pT->pTArea->ftX1out.duty[chM*2+1] = speed;
	}
	else
	{
		assert(pT->pTArea+1);
		(pT->pTArea+1)->ftX1out.duty[(chM-8)*2] = 0;
		(pT->pTArea+1)->ftX1out.duty[(chM-8)*2+1] = speed;
	}
}


} /* namespace ft */
