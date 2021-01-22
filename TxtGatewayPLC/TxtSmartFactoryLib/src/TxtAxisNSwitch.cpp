/*
 * TxtAxisNSwitch.cpp
 *
 *  Created on: 08.11.2018
 *      Author: steiger-a
 */

#include "TxtAxisNSwitch.h"
#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"


namespace ft {


TxtAxisNSwitch::TxtAxisNSwitch(std::string name, TxtTransfer* pT, uint8_t chM, uint8_t chS1, uint8_t chS2)
	: TxtAxis(name, pT, chM, chS1), chS2X()
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console_axes"), "{} TxtAxisNSwitch chM:{} chS1:{} chS2:{}",name,chM,chS1,chS2);
	chS2X.push_back(chS1);
	chS2X.push_back(chS2);
	configInputs(chS1);
	configInputs(chS2);
	setStatus(AXIS_READY);
}

TxtAxisNSwitch::TxtAxisNSwitch(std::string name, TxtTransfer* pT, uint8_t chM, uint8_t chS1, uint8_t chS2, uint8_t chS3)
	: TxtAxis(name, pT, chM, chS1), chS2X()
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console_axes"), "{} TxtAxisNSwitch chM:{} chS1:{} chS2:{} chS3:{}",name,chM,chS1,chS2,chS3);
	chS2X.push_back(chS1);
	chS2X.push_back(chS2);
	chS2X.push_back(chS3);
	configInputs(chS1);
	configInputs(chS2);
	configInputs(chS3);
	setStatus(AXIS_READY);
}

TxtAxisNSwitch::~TxtAxisNSwitch()
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console_axes"), "{} ~TxtAxisNSwitch",name);
}

void TxtAxisNSwitch::moveS2X(int idx)
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console_axes"), "{} moveS2X[{}]",name,idx);
	if (!isS2XValid(idx))
	{
		spdlog::get("console_axes")->error("{} Error: index {} for S2X is out of bounds!",name,idx);
		std::cout << "exit moveS2X" << std::endl;
		spdlog::get("file_logger")->error("exit moveS2X",0);
		exit(1);
	}
	uint8_t chS = chS2X[idx];
	SPDLOG_LOGGER_DEBUG(spdlog::get("console_axes"), "{} chS[{}]",name,chS);

	//check switch ref
	if (isSwitchPressed(chS))
	{
		setMotorOff();
		setStatus(AXIS_READY);
		return;
	}

	if (idx==0)
	{
		setMotorLeft();
	}
	else
	{
		setMotorRight();
	}
	setStatus(AXIS_MOVING_S2X);

	auto start = std::chrono::system_clock::now();
	while (true)
	{
		//check switch ref
		if (isSwitchPressed(chS))
		{
			setMotorOff();
			break;
		}
		//check stop req
		if (stopReq) {
			setMotorOff();
			stopReq = false;
			SPDLOG_LOGGER_DEBUG(spdlog::get("console_axes"), "{} stopReq",name);
			break;
		}
		//check timeout
		auto end = std::chrono::system_clock::now();
		auto dur = end-start;
		auto diff_s = std::chrono::duration_cast< std::chrono::duration<float> >(dur).count();
		//SPDLOG_LOGGER_DEBUG(spdlog::get("console_axes"), "diff_s:{} diff_max:{}",diff_s,TIMEOUT_S_MOVEREF);
		if (diff_s > TIMEOUT_S_MOVERIGHT) {
			setStatus(AXIS_TIMEOUT_MOVERIGHT);
			spdlog::get("console_axes")->warn("{} diff_s > TIMEOUT_S_MOVERIGHT, diff_s:{} diff_max:{}",name,diff_s,TIMEOUT_S_MOVERIGHT);
			break;
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(CYCLE_MS_AXIS));
	}
	if (status == AXIS_MOVING_S2X)
	{
		setStatus(AXIS_READY);
	} else {
		std::string sst = toString(status);
		SPDLOG_LOGGER_DEBUG(spdlog::get("console_axes"), "{} setStatus:{}",name,sst);
		std::cout << "exit moveS2X 3" << std::endl;
		spdlog::get("file_logger")->error("exit moveS2X 3",0);
		exit(1);
	}
}


} /* namespace ft */
