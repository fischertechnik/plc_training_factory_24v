/*
 * TxtPanTiltUnit.cpp
 *
 *  Created on: 20.12.2017
 *      Author: steiger-a
 */

#include "TxtPanTiltUnit.h"

#include <chrono>
#include <ctime>

namespace ft {


TxtPanTiltUnit::TxtPanTiltUnit(TxtTransfer* pT, uint8_t chPan, uint8_t chTilt)
: pT(pT), stopAllReq(false),  status(PTU_NOHOME),
  posPan(0), posTilt(0), speedPan(512), speedTilt(512), stepsPan(1), stepsTilt(1),
  chPan(chPan), chTilt(chTilt)
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "TxtPanTiltUnit chPan:{} chTilt:{}",  chPan, chTilt);
	if (!calibData.existCalibFilename()) calibData.saveDefault();
	calibData.load();
	configInputs(chPan);
	configInputs(chTilt);
}

TxtPanTiltUnit::~TxtPanTiltUnit() {
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "~TxtPanTiltUnit");
	if (pT->pTArea)
	{
		//switch off Motors
		setMotorsOff();
	}
}

bool TxtPanTiltUnit::init() {
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "init");
	status = PTU_NOHOME;
	SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "status=PTU_NOHOME");
	return true;
}

void TxtPanTiltUnit::stop() {
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "stop");
	stopAllReq = true;
}

void TxtPanTiltUnit::configInputs(uint8_t chS)
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "configInputs chS:{}", chS);
	assert(pT->pTArea);
	pT->pTArea->ftX1config.uni[chS].mode = MODE_R; // Digital Switch with PullUp resistor
	pT->pTArea->ftX1config.uni[chS].digital = 1;
	pT->pTArea->ftX1state.config_id ++; // Save the new Setup
}

void TxtPanTiltUnit::setMotorsOff() {
	//SPDLOG_LOGGER_TRACE(spdlog::get("console"), "setMotorsOff");
	setMotorPanOff();
	setMotorTiltOff();
	stopAllReq = false;
}

void TxtPanTiltUnit::setMotorPanOff() {
	//SPDLOG_LOGGER_TRACE(spdlog::get("console"), "setMotorPanOff");
	assert(pT->pTArea);
	pT->pTArea->ftX1out.duty[chPan*2] = 0;
	pT->pTArea->ftX1out.duty[chPan*2+1] = 0;
}

void TxtPanTiltUnit::setMotorTiltOff() {
	//SPDLOG_LOGGER_TRACE(spdlog::get("console"), "setMotorTiltOff");
	assert(pT->pTArea);
	pT->pTArea->ftX1out.duty[chTilt*2] = 0;
	pT->pTArea->ftX1out.duty[chTilt*2+1] = 0;
}

void TxtPanTiltUnit::moveLeft(uint8_t ch, uint16_t steps, int16_t speed, uint16_t* p, uint16_t posEnd) {
	assert(p!=NULL);
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "moveLeft ch:{} steps:{} speed:{} p:{} posEnd:{}", ch, steps, speed, *p, posEnd);
	if (steps <= 0) {
		spdlog::get("console")->warn("Warning: steps<=0. Exit function.");
		return;
	}
	if (status == PTU_NOHOME) {
		spdlog::get("console")->error("Error: status == PTU_NOHOME. Execute moveHome() first! Exit function.");
		return;
	}
	if (status != PTU_READY) {
		spdlog::get("console")->error("Error: status != PTU_READY. Exit function.");
		return;
	}
	int16_t posa = *p;
	SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "posa: {}", posa);
	//int16_t lastCounter = 0;

	SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "setup distance:{}", steps);
	pT->pTArea->ftX1out.distance[ch] = steps; // Distance to drive Motor 1 [0]
	pT->pTArea->ftX1out.motor_ex_cmd_id[ch]++; // Set new Distance Value for Motor 1 [0]
	pT->pTArea->ftX1out.duty[ch*2] = speed; // Switch Motor 1 ( O1 [0] ) on with PWM Value 512 (= max speed)
	pT->pTArea->ftX1out.duty[ch*2+1] = 0; // Switch Motor 1 ( O2 [1] ) with minus
	status = PTU_MOVING;
	SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "setup duty left");

	auto start = std::chrono::system_clock::now();
	while (pT->pTArea->ftX1in.motor_ex_cmd_id[ch] < pT->pTArea->ftX1out.motor_ex_cmd_id[ch])
	{
		//check home switch
		if (pT->pTArea->ftX1in.uni[ch] == 1)
		{   // Input IX on Master Interface is "1"
			SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "end switch:{} duty 0 0", ch);
			pT->pTArea->ftX1out.duty[ch*2] = 0;
			pT->pTArea->ftX1out.duty[ch*2+1] = 0;
			break;
		}
		//check stop req
		if (stopAllReq) {
			SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "stopAllReq=TRUE");
			setMotorsOff();
			break;
		}
		//check timeout
		auto end = std::chrono::system_clock::now();
		auto dur = end-start;
		auto diff_s = std::chrono::duration_cast< std::chrono::duration<float> >(dur).count();
		double diff_max = (double)steps*TIMEOUT_S_MOVEPERSTEP;
		//SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "diff_s:{} diff_max:{}",diff_s,diff_max);
		if (diff_s > diff_max) {
			status = PTU_TIMEOUT_MOVELEFT;
			spdlog::get("console")->warn("diff_s > diff_max: status==PTU_TIMEOUT_MOVELEFT, diff_s:{} diff_max:{}",diff_s,diff_max);
			break;
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}
	SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "OutExCmd:{} InExCmd:{}  Counter:{}",
			pT->pTArea->ftX1out.motor_ex_cmd_id[ch], pT->pTArea->ftX1in.motor_ex_cmd_id[ch], pT->pTArea->ftX1in.counter[ch]);
	//set new pos
	if (posa >= pT->pTArea->ftX1in.counter[ch]) {
		SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "posa >= pT->pTArea->ftX1in.counter[ch:{}]", ch);
		*p = posa - pT->pTArea->ftX1in.counter[ch];
	} else {
		status = PTU_ERROR;
		SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "status = PTU_ERROR: *p:{} posa:{} pT->pTArea->ftX1in.counter[ch]:{}", *p, posa, pT->pTArea->ftX1in.counter[ch]);
		return;
	}
	//reset counter
	pT->pTArea->ftX1out.cnt_reset_cmd_id[ch]++;
	status = PTU_READY;
}

void TxtPanTiltUnit::moveRight(uint8_t ch, uint16_t steps, int16_t speed, uint16_t* p, uint16_t posEnd) {
	assert(p!=NULL);
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "moveRight ch:{} steps:{} speed:{} p:{} posEnd:{}", ch, steps, speed, *p, posEnd);
	if (steps <= 0) {
		spdlog::get("console")->warn("Warning: steps<=0. Exit function.");
		return;
	}
	if (status == PTU_NOHOME) {
		spdlog::get("console")->error("Error: status == PTU_NOHOME. Execute moveHome() first! Exit function.");
		return;
	}
	if (status != PTU_READY) {
		spdlog::get("console")->error("Error: status != PTU_READY. Exit function.");
		return;
	}
	int16_t posa = *p;
	SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "posa: {}", posa);

	pT->pTArea->ftX1out.distance[ch] = steps; // Distance to go back
	pT->pTArea->ftX1out.motor_ex_cmd_id[ch]++; // Set new Distance Value for Motor 1 [0]
	SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "setup distance:{}", steps);
	pT->pTArea->ftX1out.duty[ch*2] = 0; // Switch Motor 1 ( O1 [0] ) with minus
	pT->pTArea->ftX1out.duty[ch*2+1] = speed; // Switch Motor 1 ( O2 [1] ) on with PWM Value 512 (max speed)
	status = PTU_MOVING;
	SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "setup duty right");

	auto start = std::chrono::system_clock::now();
	while (pT->pTArea->ftX1in.motor_ex_cmd_id[ch] < pT->pTArea->ftX1out.motor_ex_cmd_id[ch])
	{
		//check end pos
		if (((posa+pT->pTArea->ftX1in.counter[ch])) >= posEnd) {
			SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "((posa+pT->pTArea->ftX1in.counter[ch:{}])) >= posEnd:{}", ch, posEnd);
			pT->pTArea->ftX1out.duty[ch*2] = 0; // Switch Motor 1 ( O1 [0] ) with minus
			pT->pTArea->ftX1out.duty[ch*2+1] = 0; // Switch Motor 1 ( O2 [1] ) on with PWM Value 512 (max speed)
			break;
		}
		//check stop req
		if (stopAllReq) {
			SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "stopAllReq=TRUE");
			setMotorsOff();
			break;
		}
		//check timeout
		auto end = std::chrono::system_clock::now();
		auto dur = end-start;
		auto diff_s = std::chrono::duration_cast< std::chrono::duration<float> >(dur).count();
		double diff_max = (double)steps*TIMEOUT_S_MOVEPERSTEP;
		//SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "diff_s:{} diff_max:{}",diff_s,diff_max);
		if (diff_s > diff_max) {
			status = PTU_TIMEOUT_MOVERIGHT;
			spdlog::get("console")->warn("diff_s > diff_max: status==PTU_TIMEOUT_MOVERIGHT, diff_s:{} diff_max:{}",diff_s,diff_max);
			break;
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}
	SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "OutExCmd:{} InExCmd:{}  Counter:{}",
			pT->pTArea->ftX1out.motor_ex_cmd_id[ch], pT->pTArea->ftX1in.motor_ex_cmd_id[ch], pT->pTArea->ftX1in.counter[ch]);
	//set new pos
	*p = posa + pT->pTArea->ftX1in.counter[ch];
	pT->pTArea->ftX1out.cnt_reset_cmd_id[ch]++;
	status = PTU_READY;
}

void TxtPanTiltUnit::movePanLeft(uint16_t steps) {
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "movePanLeft steps:{} posPan:{}", steps, posPan);
	uint16_t p = posPan;
	moveLeft(chPan, steps, speedPan, &p, calibData.posEndPan);
	if (status == PTU_READY) {
		posPan = p;
		SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "posPan:{}", posPan);
		Notify();
	}
}

void TxtPanTiltUnit::movePanRight(uint16_t steps) {
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "movePanRight steps:{} posPan:{}", steps, posPan);
	uint16_t p = posPan;
	moveRight(chPan, steps, speedPan, &p, calibData.posEndPan);
	if (status == PTU_READY) {
		posPan = p;
		SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "posPan:{}", posPan);
		Notify();
	}
}

void TxtPanTiltUnit::moveTiltUp(uint16_t steps) {
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "moveTiltUp steps:{} posTilt:{}", steps, posTilt);
	uint16_t p = posTilt;
	moveRight(chTilt, steps, speedTilt, &p, calibData.posEndTilt);
	if (status == PTU_READY) {
		posTilt = p;
		SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "posTilt:{}", posTilt);
		Notify();
	}
}

void TxtPanTiltUnit::moveTiltDown(uint16_t steps) {
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "moveTiltDown steps:{} posTilt:{}", steps, posTilt);
	uint16_t p = posTilt;
	moveLeft(chTilt, steps, speedTilt, &p, calibData.posEndTilt);
	if (status == PTU_READY) {
		posTilt = p;
		SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "posTilt:{}", posTilt);
		Notify();
	}
}

bool TxtPanTiltUnit::movePanPos(uint16_t pPan) {
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "movePanPos pPan:{}", pPan);
	if (pPan == posPan) {
		SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "pPan:{} == posPan:{}", pPan, posPan);
		return true;
	} else if (pPan > posPan) {
		SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "pPan:{} > posPan:{}", pPan, posPan);
		setStepPan(pPan-posPan);
		moveStepPanRight();
		return true;
	} else if (pPan < posPan) {
		SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "pPan:{} < posPan:{}", pPan, posPan);
		setStepPan(posPan-pPan);
		moveStepPanLeft();
		return true;
	}
	return false;
}

bool TxtPanTiltUnit::moveTiltPos(uint16_t pTilt) {
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "moveTiltPos pTilt:{}", pTilt);
	if (pTilt == posTilt) {
		SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "pTilt:{} == posTilt:{}", pTilt, posTilt);
		return true;
	} else if (pTilt > posTilt) {
		SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "pTilt:{} > posTilt:{}", pTilt, posTilt);
		setStepTilt(pTilt-posTilt);
		moveStepTiltUp();
		return true;
	} else if (pTilt < posTilt) {
		SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "pTilt:{} < posTilt:{}", pTilt, posTilt);
		setStepTilt(posTilt-pTilt);
		moveStepTiltDown();
		return true;
	}
	return false;
}

bool TxtPanTiltUnit::movePos(uint16_t pPan, uint16_t pTilt) {
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "movePos pPan:{} pTilt:{}", pPan, pTilt);
	bool retP = movePanPos(pPan);
	if (!retP) {
		return false;
	}
	bool retT = moveTiltPos(pTilt);
	if (!retT) {
		return false;
	}
	return true;
}

void TxtPanTiltUnit::moveHome() {
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "moveHome");
	/*TODO if (status != PTU_READY) {
    	SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "status != PTU_READY, return");
    	return;
    }*/

	// Switch Motor on
	//pT->pTArea->ftX1out.distance[chPan] = 1; // Distance
	pT->pTArea->ftX1out.motor_ex_cmd_id[chPan]++;
	pT->pTArea->ftX1out.duty[chPan*2] = speedPan; // Switch Motor 1 ( O1 [0] ) on with PWM Value 512 (= max speed)
	pT->pTArea->ftX1out.duty[chPan*2+1] = 0; // Switch Motor 1 ( O2 [1] ) with minus

	//pT->pTArea->ftX1out.distance[chTilt] = 1; // Distance
	pT->pTArea->ftX1out.motor_ex_cmd_id[chTilt]++;
	pT->pTArea->ftX1out.duty[chTilt*2] = speedTilt; // Switch Motor 1 ( O1 [0] ) on with PWM Value 512 (= max speed)
	pT->pTArea->ftX1out.duty[chTilt*2+1] = 0; // Switch Motor 1 ( O2 [1] ) with minus

	status = PTU_MOVING;

	auto start = std::chrono::system_clock::now();
	while (true) //pT->pTArea->ftX1in.motor_ex_cmd_id[chPan] < pT->pTArea->ftX1out.motor_ex_cmd_id[chPan])
	{
		//check switch home pan
		if (pT->pTArea->ftX1in.uni[chPan] == 1)
		{
			setMotorPanOff();
		}
		//check switch home tilt
		if (pT->pTArea->ftX1in.uni[chTilt] == 1)
		{
			setMotorTiltOff();
		}
		//check both switch
		if ((pT->pTArea->ftX1in.uni[chPan] == 1)&&(pT->pTArea->ftX1in.uni[chTilt] == 1))
		{
			break;
		}
		//check stop req
		if (stopAllReq) {
			setMotorsOff();
			status = PTU_NOHOME;
			SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "stopAllReq: status == PTU_NOHOME");
			break;
		}
		//check timeout
		auto end = std::chrono::system_clock::now();
		auto dur = end-start;
		auto diff_s = std::chrono::duration_cast< std::chrono::duration<float> >(dur).count();
		//SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "diff_ms:{} diff_max:{}",diff_ms,TIMEOUTMS_MOVEHOME);
		if (diff_s > TIMEOUT_S_MOVEHOME) {
			status = PTU_TIMEOUT_MOVEHOME;
			spdlog::get("console")->warn("diff_s > TIMEOUT_S_MOVEHOME: status==PTU_TIMEOUT_MOVEHOME, diff_s:{} diff_max:{}",diff_s,TIMEOUT_S_MOVEHOME);
			break;
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}
	//reset counter
	pT->pTArea->ftX1out.cnt_reset_cmd_id[chPan]++;
	posPan = 0;
	pT->pTArea->ftX1out.cnt_reset_cmd_id[chTilt]++;
	posTilt = 0;
	status = PTU_READY;
	SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "status == PTU_READY");
	Notify();
}

bool TxtPanTiltUnit::moveCenter() {
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "moveCenter");
	return movePos(calibData.posCenterPan, calibData.posCenterTilt);
}

bool TxtPanTiltUnit::moveHBW() {
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "moveHBW");
	return movePos(calibData.posHBWPan, calibData.posHBWTilt);
}

void TxtPanTiltUnit::movePan0() {
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "movePan0");
	movePanPos(0);
}

void TxtPanTiltUnit::movePanCenter() {
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "movePanCenter");
	movePanPos(calibData.posCenterPan);
}

void TxtPanTiltUnit::movePanEnd() {
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "movePanEnd");
	movePanPos(calibData.posEndPan);
}

void TxtPanTiltUnit::moveTilt0() {
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "moveTilt0");
	moveTiltPos(0);
}

void TxtPanTiltUnit::moveTiltCenter() {
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "moveTiltStart");
	moveTiltPos(calibData.posCenterTilt);
}

void TxtPanTiltUnit::moveTiltEnd() {
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "moveTiltEnd");
	moveTiltPos(calibData.posEndTilt);
}

void TxtPanTiltUnit::moveStepPanLeft() {
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "moveStepPanLeft");
	movePanLeft(stepsPan);
}

void TxtPanTiltUnit::moveStepPanRight() {
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "moveStepPanRight");
	movePanRight(stepsPan);
}

void TxtPanTiltUnit::moveStepTiltUp() {
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "moveStepTiltUp");
	moveTiltUp(stepsTilt);
}

void TxtPanTiltUnit::moveStepTiltDown() {
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "moveStepTiltDown");
	moveTiltDown(stepsTilt);
}


TxtPanTiltUnitController::TxtPanTiltUnitController(TxtPanTiltUnit* ptu, int mode)
: ptu(ptu), mode(mode), scmd(""), busy(false), steps(0), m_stoprequested(false), m_running(false), m_mutex(), m_thread()
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "TxtPanTiltUnitController mode:{}", mode);
	pthread_mutexattr_t attr;
	pthread_mutexattr_init(&attr);
	pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
	pthread_mutex_init(&m_mutex, &attr);
}

TxtPanTiltUnitController::~TxtPanTiltUnitController() {
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "~TxtPanTiltUnitController");
	if (m_running) stopThread();
	pthread_mutex_destroy(&m_mutex);
}

bool TxtPanTiltUnitController::startThread() {
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "start");
	//go
	assert(m_running == false);
	m_running = true;

	/*pthread_attr_t tattr;
	int ret;
	sched_param param;
	int newprio = 20;
	ret = pthread_attr_init(&tattr);
	ret = pthread_attr_getschedparam (&tattr, &param);
	SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "old priority {}", param.sched_priority);
	param.sched_priority = newprio;
	ret = pthread_attr_setschedparam (&tattr, &param);
	SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "new priority {}",newprio);
	return pthread_create(&m_thread, &tattr, start_thread, this) == 0;*/
	return pthread_create(&m_thread, 0, start_thread, this) == 0;
}

bool TxtPanTiltUnitController::stopThread() {
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "stop");
	//stop
	assert(m_running == true);
	m_running = false;
	m_stoprequested = true;
	return pthread_join(m_thread, 0) == 0;
}

bool TxtPanTiltUnitController::executeCmd(const std::string scmd, int steps) {
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "executeCmd {} {}", scmd.c_str(), steps);
	if (!busy) {
		pthread_mutex_lock(&m_mutex);
		this->scmd = scmd;
		this->steps = steps;
		pthread_mutex_unlock(&m_mutex);
		return true;
	}
	return false;
}

void TxtPanTiltUnitController::run() {
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "run");
	assert(ptu);
	while (!m_stoprequested)
	{
		if(scmd=="stop") {
			SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "### scmd==stop START");
			//pthread_mutex_lock(&m_mutex);
			//TODO: ptu->stop();
			std::cout << "NOT IMPLEMENTED" << std::endl;
			busy = false;
			scmd = "";
			steps = 0;
			//pthread_mutex_unlock(&m_mutex);
			SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "### scmd==stop END");
		} else if (scmd=="home") { //TODO home -> center
			SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "### scmd==center START");
			pthread_mutex_lock(&m_mutex);
			if (mode >= 3) {
				busy = true;
				ptu->moveCenter();
			}
			scmd = "";
			busy = false;
			steps = 0;
			pthread_mutex_unlock(&m_mutex);
			SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "### scmd==center END");
		} else if (scmd=="start_pan") {
			SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "### scmd==start_pan START");
			pthread_mutex_lock(&m_mutex);
			if (mode >= 2) {
				busy = true;
				ptu->movePan0();
			}
			scmd = "";
			busy = false;
			steps = 0;
			pthread_mutex_unlock(&m_mutex);
			SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "### scmd==start_pan END");
		} else if (scmd=="end_pan") {
			SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "### scmd==end_pan START");
			pthread_mutex_lock(&m_mutex);
			if (mode >= 3) {
				busy = true;
				ptu->movePanEnd();
			}
			scmd = "";
			busy = false;
			steps = 0;
			pthread_mutex_unlock(&m_mutex);
			SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "### scmd==end_pan END");
		} else if (scmd=="start_tilt") {
			SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "### scmd==start_tilt START");
			pthread_mutex_lock(&m_mutex);
			if (mode >= 2) {
				busy = true;
				ptu->moveTilt0();
			}
			scmd = "";
			busy = false;
			steps = 0;
			pthread_mutex_unlock(&m_mutex);
			SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "### scmd==start_tilt END");
		} else if (scmd=="end_tilt") {
			SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "### scmd==end_tilt START");
			pthread_mutex_lock(&m_mutex);
			if (mode >= 3) {
				busy = true;
				ptu->moveTiltEnd();
			}
			scmd = "";
			busy = false;
			steps = 0;
			pthread_mutex_unlock(&m_mutex);
			SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "### scmd==end_tilt END");
		} else if (scmd=="relmove_left") {
			SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "### scmd==relmove_left START");
			pthread_mutex_lock(&m_mutex);
			if (mode >= 1) {
				busy = true;
				ptu->setStepPan(steps);
				ptu->moveStepPanLeft();
			}
			scmd = "";
			busy = false;
			steps = 0;
			pthread_mutex_unlock(&m_mutex);
			SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "### scmd==relmove_left END");
		} else if (scmd=="relmove_right") {
			SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "### scmd==relmove_right START");
			pthread_mutex_lock(&m_mutex);
			if (mode >= 1) {
				busy = true;
				ptu->setStepPan(steps);
				ptu->moveStepPanRight();
			}
			scmd = "";
			busy = false;
			steps = 0;
			pthread_mutex_unlock(&m_mutex);
			SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "### scmd==relmove_right END");
		} else if (scmd=="relmove_up") {
			SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "### scmd==relmove_up START");
			pthread_mutex_lock(&m_mutex);
			if (mode >= 1) {
				busy = true;
				ptu->setStepTilt(steps);
				ptu->moveStepTiltUp();
			}
			scmd = "";
			busy = false;
			steps = 0;
			pthread_mutex_unlock(&m_mutex);
			SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "### scmd==relmove_up END");
		} else if (scmd=="relmove_down") {
			SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "### scmd==relmove_down START");
			pthread_mutex_lock(&m_mutex);
			if (mode >= 1) {
				busy = true;
				ptu->setStepTilt(steps);
				ptu->moveStepTiltDown();
			}
			scmd = "";
			busy = false;
			steps = 0;
			pthread_mutex_unlock(&m_mutex);
			SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "### scmd==relmove_down END");
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}
	ptu->stop();
}


} /* namespace ft */
