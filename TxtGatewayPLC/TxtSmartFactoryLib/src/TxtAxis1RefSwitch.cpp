/*
 * Axis1RefSwitch.cpp
 *
 *  Created on: 08.11.2018
 *      Author: steiger-a
 */

#include "TxtAxis1RefSwitch.h"

#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"


namespace ft {


TxtAxis1RefSwitch::TxtAxis1RefSwitch(std::string name, TxtTransfer* pT, uint8_t chM, uint8_t chS1, uint16_t posEnd)
	: TxtAxis(name, pT, chM, chS1), pos(0), posEnd(posEnd)
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console_axes"), "{} TxtAxis1RefSwitch chM:{} chS1:{} posEnd:{}",name,chM,chS1,posEnd);
	configInputs(chS1);
}

TxtAxis1RefSwitch::~TxtAxis1RefSwitch()
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console_axes"), "{} ~TxtAxis1RefSwitch",name);
}

void TxtAxis1RefSwitch::moveRef()
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console_axes"), "{} moveRef",name);

	resetCounter();

	if (chM < 8)
	{
		assert(pT->pTArea);
		pT->pTArea->ftX1out.distance[chM] = 0;
		pT->pTArea->ftX1out.motor_ex_cmd_id[chM]++;
	}
	else
	{
		assert(pT->pTArea+1);
		(pT->pTArea+1)->ftX1out.distance[chM-8] = 0;
		(pT->pTArea+1)->ftX1out.motor_ex_cmd_id[chM-8]++;
	}

	if ((status != AXIS_READY)&&(status != AXIS_NOREF)) {
		spdlog::get("console_axes")->error("{} Error: status != AXIS_READY. Exit function.",name);
		std::cout << "exit moveRef" << std::endl;
		spdlog::get("file_logger")->error("exit moveRef",0);
		exit(1);
	}
	setMotorLeft();
	SPDLOG_LOGGER_DEBUG(spdlog::get("console_axes"), "{} set motor chM[{}] {} {}",name,chM,
			chM<8?pT->pTArea->ftX1out.duty[chM*2]:(pT->pTArea+1)->ftX1out.duty[(chM-8)*2],
			chM<8?pT->pTArea->ftX1out.duty[chM*2+1]:(pT->pTArea+1)->ftX1out.duty[(chM-8)*2+1]);

	setStatus(AXIS_MOVING_REF);

	auto start = std::chrono::system_clock::now();
	while (true)
	{
		//check switch ref
		if (isSwitchPressed(chS1))
		{
			setMotorOff();
			break;
		}
		//check stop req
		if (stopReq) {
			setMotorOff();
			setStatus(AXIS_NOREF);
			stopReq = false;
			SPDLOG_LOGGER_DEBUG(spdlog::get("console_axes"), "{} stopReq",name);
			break;
		}
		//check timeout
		auto end = std::chrono::system_clock::now();
		auto dur = end-start;
		auto diff_s = std::chrono::duration_cast< std::chrono::duration<float> >(dur).count();
		//SPDLOG_LOGGER_DEBUG(spdlog::get("console_axes"), "diff_s:{} diff_max:{}",diff_s,TIMEOUT_S_MOVEREF);
		if (diff_s > TIMEOUT_S_MOVEREF) {
			setStatus(AXIS_TIMEOUT_MOVEREF);
			spdlog::get("console_axes")->warn("{} diff_s > AXIS_TIMEOUT_MOVEREF, diff_s:{} diff_max:{}",name,diff_s,TIMEOUT_S_MOVEREF);
			break;
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(CYCLE_MS_AXIS));
	}
	if (status == AXIS_MOVING_REF)
	{
		resetCounter();
		pos = 0;
		setStatus(AXIS_READY);
	} else {
		std::string sst = toString(status);
		SPDLOG_LOGGER_DEBUG(spdlog::get("console_axes"), "{} setStatus:{}",name,sst);
		std::cout << "exit moveRef 2" << std::endl;
		spdlog::get("file_logger")->error("exit moveRef 2",0);
		exit(1);
	}
	Notify();
}

bool TxtAxis1RefSwitch::moveAbs(uint16_t p) {
	SPDLOG_LOGGER_TRACE(spdlog::get("console_axes"), "{} movePos p:{}",name,p);
	if (p > posEnd)
	{
		spdlog::get("console_axes")->error("{} Warning: p:{} > posEnd:{}",name,p,posEnd);
	}
	if (p == pos) {
		SPDLOG_LOGGER_DEBUG(spdlog::get("console_axes"), "{} p:{} == pos:{}",name,p,pos);
		return true;
	} else if (p > pos) {
		SPDLOG_LOGGER_DEBUG(spdlog::get("console_axes"), "{} p:{} > pos:{}",name,p,pos);
		int steps = p - pos;
		uint16_t pret = pos;
		moveRight(steps, &pret);
		if (status == AXIS_READY) {
			pos= pret;
			SPDLOG_LOGGER_DEBUG(spdlog::get("console_axes"), "{} pos:{}",name,pos);
			Notify();
			return true;
		} else {
			std::string sst = toString(status);
			SPDLOG_LOGGER_DEBUG(spdlog::get("console_axes"), "{} setStatus:{}",name,sst);
			std::cout << "exit p > pos" << std::endl;
		}
	} else if (p < pos) {
		SPDLOG_LOGGER_DEBUG(spdlog::get("console_axes"), "{} p:{} < pos:{}",name,p,pos);
		int steps = pos - p;
		uint16_t pret = pos;
		moveLeft(steps, &pret);
		if (status == AXIS_READY) {
			pos= pret;
			SPDLOG_LOGGER_DEBUG(spdlog::get("console_axes"), "{} pos:{}",name,pos);
			Notify();
			return true;
		} else {
			std::string sst = toString(status);
			SPDLOG_LOGGER_DEBUG(spdlog::get("console_axes"), "{} setStatus:{}",name,sst);
			std::cout << "exit p < pos" << std::endl;
		}
	}
	return false;
}

void TxtAxis1RefSwitch::setMotorRight()
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console_axes"), "{}({}) setMotorRight",name,status);
	if (pos >= posEnd)
	{
		setMotorOff();
	}
	else
	{
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
}

void TxtAxis1RefSwitch::reset()
{
	SPDLOG_LOGGER_DEBUG(spdlog::get("console_axes"), "{} reset",name);
	if (chM < 8)
	{
		assert(pT->pTArea);
		pT->pTArea->ftX1out.cnt_reset_cmd_id[chM] = 0;
		pT->pTArea->ftX1out.distance[chM] = 0;
		pT->pTArea->ftX1out.motor_ex_cmd_id[chM]++;
	}
	else
	{
		assert(pT->pTArea+1);
		(pT->pTArea+1)->ftX1out.cnt_reset_cmd_id[chM-8] = 0;
		(pT->pTArea+1)->ftX1out.distance[chM-8] = 0;
		(pT->pTArea+1)->ftX1out.motor_ex_cmd_id[chM-8]++;
	}
}

void TxtAxis1RefSwitch::resetCounter()
{
	SPDLOG_LOGGER_DEBUG(spdlog::get("console_axes"), "{} cnt {}",
			name,chM<8?pT->pTArea->ftX1in.counter[chM]:(pT->pTArea+1)->ftX1in.counter[chM-8]);
	chM<8?pT->pTArea->ftX1out.cnt_reset_cmd_id[chM]++:(pT->pTArea+1)->ftX1out.cnt_reset_cmd_id[chM-8]++;
	//while(chM<8?pT->pTArea->ftX1in.cnt_resetted[chM]:(pT->pTArea+1)->ftX1in.cnt_resetted[chM-8] != 1);
	SPDLOG_LOGGER_DEBUG(spdlog::get("console_axes"), "{} cnt_reset {}",
			name,chM<8?pT->pTArea->ftX1in.counter[chM]:(pT->pTArea+1)->ftX1in.counter[chM-8]);
}

void TxtAxis1RefSwitch::moveLeft(uint16_t steps, uint16_t* p)
{
	assert(p!=NULL);
	SPDLOG_LOGGER_TRACE(spdlog::get("console_axes"), "{} moveLeft chM:{} chS1:{} steps:{} p:{}",name,chM,chS1,steps,*p);
	if (steps <= 0) {
		spdlog::get("console_axes")->warn("{} Warning: steps<=0. Exit function.",name);
		return;
	}
	if (status == AXIS_NOREF) {
		spdlog::get("console_axes")->error("{} Error: status == AXIS_NOREF. Execute moveRef() first! Exit function.",name);
		return;
	}
	if (status != AXIS_READY) {
		spdlog::get("console_axes")->error("{} Error: status != AXIS_READY. Exit function.",name);
		return;
	}
	int16_t posa = *p;
	SPDLOG_LOGGER_DEBUG(spdlog::get("console_axes"), "{} posa: {}",name,posa);

	SPDLOG_LOGGER_DEBUG(spdlog::get("console_axes"), "{} setup distance:{}",name,steps);
	if (chM < 8)
	{
		pT->pTArea->ftX1out.distance[chM] = steps; // Distance to drive Motor 1 [0]
		pT->pTArea->ftX1out.motor_ex_cmd_id[chM]++; // Set new Distance Value for Motor 1 [0]
	}
	else
	{
		(pT->pTArea+1)->ftX1out.distance[chM-8] = steps; // Distance to drive Motor 1 [0]
		(pT->pTArea+1)->ftX1out.motor_ex_cmd_id[chM-8]++; // Set new Distance Value for Motor 1 [0]
	}
	setMotorLeft();

	setStatus(AXIS_MOVING_LEFT);
	SPDLOG_LOGGER_DEBUG(spdlog::get("console_axes"), "{} setup duty left",name);

	//resetCounter();

	auto start = std::chrono::system_clock::now();
	while ((chM<8?pT->pTArea->ftX1in.motor_ex_cmd_id[chM]:(pT->pTArea+1)->ftX1in.motor_ex_cmd_id[chM-8])
		< (chM<8?pT->pTArea->ftX1out.motor_ex_cmd_id[chM]:(pT->pTArea+1)->ftX1out.motor_ex_cmd_id[chM-8]))
	{
		//check home switch
		if (isSwitchPressed(chS1))
		{
			setMotorOff();
			resetCounter();
			pos = 0;
			break;
		}
		//check stop req
		if (stopReq) {
			SPDLOG_LOGGER_DEBUG(spdlog::get("console_axes"), "{} stopAllReq",name);
			setMotorOff();
			stopReq = false;
			break;
		}
		//check timeout
		auto end = std::chrono::system_clock::now();
		auto dur = end-start;
		auto diff_s = std::chrono::duration_cast< std::chrono::duration<float> >(dur).count();
		double diff_max = (double)TIMEOUT_S_MOVELEFT;
		//SPDLOG_LOGGER_DEBUG(spdlog::get("console_axes"), "diff_s:{} diff_max:{}",diff_s,diff_max);
		if (diff_s > diff_max) {
			setStatus(AXIS_TIMEOUT_MOVELEFT);
			spdlog::get("console_axes")->warn("{} diff_s > diff_max: diff_s:{} diff_max:{}",name,diff_s,diff_max);
			break;
		}
		SPDLOG_LOGGER_DEBUG(spdlog::get("console_axes"), "{} OutExCmd:{} InExCmd:{} Counter:{} reach:{}",
			name,
			chM<8?pT->pTArea->ftX1out.motor_ex_cmd_id[chM] :(pT->pTArea+1)->ftX1out.motor_ex_cmd_id[chM-8],
			chM<8?pT->pTArea->ftX1in.motor_ex_cmd_id[chM]  :(pT->pTArea+1)->ftX1in.motor_ex_cmd_id[chM-8],
			chM<8?pT->pTArea->ftX1in.counter[chM]          :(pT->pTArea+1)->ftX1in.counter[chM-8],
			chM<8?pT->pTArea->ftX1in.motor_ex_reached[chM] :(pT->pTArea+1)->ftX1in.motor_ex_reached[chM-8]);
		std::this_thread::sleep_for(std::chrono::milliseconds(CYCLE_MS_AXIS));
	}
	if (status == AXIS_MOVING_LEFT)
	{
		//set new pos
		SPDLOG_LOGGER_DEBUG(spdlog::get("console_axes"), "{} posa >= pT->pTArea->ftX1in.counter[ch:{}]",name,chM);
		INT16 chMcounter = chM<8?pT->pTArea->ftX1in.counter[chM]:(pT->pTArea+1)->ftX1in.counter[chM-8];
		if (posa >= chMcounter) {
			*p = posa - chMcounter;
		} else {
			*p = 0;
		}
		SPDLOG_LOGGER_DEBUG(spdlog::get("console_axes"), "{} *p:{}",name,*p);
		setStatus(AXIS_READY);
	} else {
		std::string sst = toString(status);
		SPDLOG_LOGGER_DEBUG(spdlog::get("console_axes"), "{} setStatus:{}",name,sst);
		std::cout << "wrong status" << std::endl;
		spdlog::get("file_logger")->error("wrong status",0);
		exit(1);
	}
}

void TxtAxis1RefSwitch::moveRight(uint16_t steps, uint16_t* p) {
	assert(p!=NULL);
	SPDLOG_LOGGER_TRACE(spdlog::get("console_axes"), "{} moveRight steps:{} p:{}",name,steps,*p);
	if (steps <= 0) {
		spdlog::get("console_axes")->warn("{} Warning: steps<=0. Exit function.",name);
		return;
	}
	if (status == AXIS_NOREF) {
		spdlog::get("console_axes")->error("{} Error: status == AXIS_NOREF. Execute moveRef() first! Exit function.",name);
		return;
	}
	if (status != AXIS_READY) {
		spdlog::get("console_axes")->error("{} Error: status != AXIS_READY. Exit function.",name);
		return;
	}
	int16_t posa = *p;
	SPDLOG_LOGGER_DEBUG(spdlog::get("console_axes"), "posa: {}", posa);

	if (chM < 8)
	{
		assert(pT->pTArea);
		pT->pTArea->ftX1out.distance[chM] = steps; // Distance to go back
		pT->pTArea->ftX1out.motor_ex_cmd_id[chM]++; // Set new Distance Value for Motor 1 [0]
	}
	else
	{
		assert(pT->pTArea+1);
		(pT->pTArea+1)->ftX1out.distance[chM-8] = steps; // Distance to go back
		(pT->pTArea+1)->ftX1out.motor_ex_cmd_id[chM-8]++; // Set new Distance Value for Motor 1 [0]
	}

	SPDLOG_LOGGER_DEBUG(spdlog::get("console_axes"), "{} setup distance:{}",name,steps);
	setMotorRight();
	SPDLOG_LOGGER_DEBUG(spdlog::get("console_axes"), "{} setup duty right",name);

	setStatus(AXIS_MOVING_RIGHT);

	auto start = std::chrono::system_clock::now();
	while ((chM<8?pT->pTArea->ftX1in.motor_ex_cmd_id[chM]:(pT->pTArea+1)->ftX1in.motor_ex_cmd_id[chM-8])
		< (chM<8?pT->pTArea->ftX1out.motor_ex_cmd_id[chM]:(pT->pTArea+1)->ftX1out.motor_ex_cmd_id[chM-8]))
	{
		//check end pos
		if (((posa+steps)) >= posEnd) {
			SPDLOG_LOGGER_DEBUG(spdlog::get("console_axes"), "{} STOPPING posEnd:{}",name,posEnd);
			setMotorOff();
			break;
		}
		//check stop req
		if (stopReq) {
			SPDLOG_LOGGER_DEBUG(spdlog::get("console_axes"), "{} stopAllReq",name);
			setMotorOff();
			stopReq = false;
			break;
		}
		//check timeout
		auto end = std::chrono::system_clock::now();
		auto dur = end-start;
		auto diff_s = std::chrono::duration_cast< std::chrono::duration<float> >(dur).count();
		double diff_max = (double)TIMEOUT_S_MOVERIGHT;
		//SPDLOG_LOGGER_DEBUG(spdlog::get("console_axes"), "diff_s:{} diff_max:{}",diff_s,diff_max);
		if (diff_s > diff_max) {
			setStatus(AXIS_TIMEOUT_MOVERIGHT);
			spdlog::get("console_axes")->warn("{} diff_s > diff_max: diff_s:{} diff_max:{}",name,diff_s,diff_max);
			break;
		}
		SPDLOG_LOGGER_DEBUG(spdlog::get("console_axes"), "{} OutExCmd:{} InExCmd:{} Counter:{} reach:{}",
			name,
			chM<8?pT->pTArea->ftX1out.motor_ex_cmd_id[chM] :(pT->pTArea+1)->ftX1out.motor_ex_cmd_id[chM-8],
			chM<8?pT->pTArea->ftX1in.motor_ex_cmd_id[chM]  :(pT->pTArea+1)->ftX1in.motor_ex_cmd_id[chM-8],
			chM<8?pT->pTArea->ftX1in.counter[chM]          :(pT->pTArea+1)->ftX1in.counter[chM-8],
			chM<8?pT->pTArea->ftX1in.motor_ex_reached[chM] :(pT->pTArea+1)->ftX1in.motor_ex_reached[chM-8]);
		std::this_thread::sleep_for(std::chrono::milliseconds(CYCLE_MS_AXIS));
	}
	if (status == AXIS_MOVING_RIGHT)
	{
		//set new pos
		INT16 chMcounter = chM<8?pT->pTArea->ftX1in.counter[chM]:(pT->pTArea+1)->ftX1in.counter[chM-8];
		if ((posa + chMcounter) >= posEnd) {
			setStatus(AXIS_ERROR);
			SPDLOG_LOGGER_DEBUG(spdlog::get("console_axes"), "{} AXIS_ERROR: *p:{} posa:{} pT->pTArea->ftX1in.counter[ch]:{}",name,*p,posa,chMcounter);
			std::cout << "exit moveRight" << std::endl;
			spdlog::get("file_logger")->error("exit moveRight",0);
			exit(1);
		} else {
			*p = posa + chMcounter;
			setStatus(AXIS_READY);
		}
	} else {
		std::string sst = toString(status);
		SPDLOG_LOGGER_DEBUG(spdlog::get("console_axes"), "{} setStatus:{}",name,sst);
		std::cout << "exit moveRight 2" << std::endl;
		spdlog::get("file_logger")->error("exit moveRight 2",0);
		exit(1);
	}
}


} /* namespace ft */
