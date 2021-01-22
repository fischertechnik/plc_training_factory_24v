/*
 * TxtPanTiltUnit.h
 *
 *  Created on: 20.12.2017
 *      Author: steiger-a
 */

#ifndef TXTPANTILTUNIT_H_
#define TXTPANTILTUNIT_H_

#include <stdio.h>          // for printf()
#include <unistd.h>         // for sleep()
#include <iostream>

#include "KeLibTxtDl.h"     // TXT Lib
#include "FtShmem.h"        // TXT Transfer Area

#include "Observer.h"
#include "TxtAxis.h"
#include "TxtCalibData.h"

#include "spdlog/spdlog.h"


namespace ft {


typedef enum
{
	PTU_NONE = 0,
	PTU_ERROR = 1,
	PTU_NOHOME = 2,
	PTU_READY = 3,
	PTU_MOVING = 4,
	PTU_TIMEOUT_MOVEHOME = 5,
	PTU_TIMEOUT_MOVELEFT = 6,
	PTU_TIMEOUT_MOVERIGHT = 7
} TxtPanTiltUnit_status_t;


#define STEPS_PAN_MAX 50
#define STEPS_TILT_MAX 50

#define TIMEOUT_S_MOVEHOME 11.0
#define TIMEOUT_S_MOVEPERSTEP 0.01 //1100*0.01=11.0


class TxtPanTiltUnitCalibData : public ft::TxtCalibData {
public:
	TxtPanTiltUnitCalibData()
		: TxtCalibData(("Data/Calib.SSC.json")) {};
	virtual ~TxtPanTiltUnitCalibData() {}

	bool load();
	bool saveDefault();
	bool save();

	uint16_t posCenterPan = 925; //center (needed because non symmetrical!)
	uint16_t posCenterTilt = 425; //center
	uint16_t posEndPan = 1500; //end
	uint16_t posEndTilt = 700; //end
	uint16_t posHBWPan = 1450; //center (needed because non symmetrical!)
	uint16_t posHBWTilt = 290; //center
};


class TxtPanTiltUnit : public SubjectObserver {
public:

	TxtPanTiltUnit(TxtTransfer* pT, uint8_t chPan=0, uint8_t chTilt=1);
	virtual ~TxtPanTiltUnit();

	bool init();

	/* status */
	TxtPanTiltUnit_status_t getStatus() { return status; }

	/* configuration */
	void setStepPan(uint16_t steps) {
		SPDLOG_LOGGER_TRACE(spdlog::get("console"), "stepsPan:{}", steps);
		stepsPan=steps;
	}//(steps<=STEPS_PAN_MAX)?stepsPan=steps:stepsPan=STEPS_PAN_MAX; }

	void setStepTilt(uint16_t steps) {
		SPDLOG_LOGGER_TRACE(spdlog::get("console"), "stepsTilt:{}", steps);
		stepsTilt=steps;
	}//(steps<=STEPS_TILT_MAX)?stepsTilt=steps:stepsTilt=STEPS_TILT_MAX; }

	uint16_t getStepPan() { return stepsPan; }
	uint16_t getStepTilt() { return stepsTilt; }

	void setSpeedPan(int16_t speed) {
		SPDLOG_LOGGER_TRACE(spdlog::get("console"), "stepsPan:{}", speed);
		speedPan=speed;
	}
	void setSpeedTilt(int16_t speed) {
		SPDLOG_LOGGER_TRACE(spdlog::get("console"), "stepsTilt:{}", speed);
		speedTilt=speed;
	}
	int16_t getSpeedPan() { return speedPan; }
	int16_t getSpeedTilt() { return speedTilt; }

	/* position */
	uint16_t getPosPan() { return posPan; }
	uint16_t getPosTilt() { return posTilt; }

	/* position relative: -1.0 ... 0.0 ... +1.0 */
	float getPosPanRel()
	{
		return (float)(posPan-calibData.posCenterPan)/(float)(posPan>calibData.posCenterPan?calibData.posEndPan-calibData.posCenterPan:calibData.posCenterPan);
	}
	float getPosTiltRel()
	{
		return (float)(posTilt-calibData.posCenterTilt)/(float)(posTilt>calibData.posCenterTilt?calibData.posEndTilt-calibData.posCenterTilt:calibData.posCenterTilt);
	}

	/* move commands */
	void stop();
	bool movePos(uint16_t pPan, uint16_t pTilt);
	bool moveCenter();
	bool moveHBW();
	void moveHome();

	void movePan0();
	void movePanCenter();
	void movePanEnd();

	void moveTilt0();
	void moveTiltCenter();
	void moveTiltEnd();

	void moveStepPanLeft();
	void moveStepPanRight();
	void moveStepTiltUp();
	void moveStepTiltDown();

	bool movePanPos(uint16_t pPan);
	bool moveTiltPos(uint16_t pTilt);

protected:
	void configInputs(uint8_t ch);

	void setMotorsOff();
	void setMotorPanOff();
	void setMotorTiltOff();

	void moveLeft(uint8_t ch, uint16_t steps, int16_t speed, uint16_t* pPos, uint16_t posEnd);
	void moveRight(uint8_t ch, uint16_t steps, int16_t speed, uint16_t* pPos, uint16_t posEnd);

	void movePanLeft(uint16_t steps);
	void movePanRight(uint16_t steps);
	void moveTiltUp(uint16_t steps);
	void moveTiltDown(uint16_t steps);

	TxtTransfer* pT;
	bool stopAllReq;
	/* status */
	TxtPanTiltUnit_status_t status;
	/* pos */
	uint16_t posPan;
	uint16_t posTilt;
	/* speed */
	int16_t speedPan;
	int16_t speedTilt;
	/* steps */
	uint16_t stepsPan;
	uint16_t stepsTilt;
	/* ports */
	uint8_t chPan; //M1 + S1
	uint8_t chTilt; //M2 + S2
	TxtPanTiltUnitCalibData calibData;
};

class TxtPanTiltUnitController {
public:
	TxtPanTiltUnitController(TxtPanTiltUnit* ptu, int mode = -1);
	virtual ~TxtPanTiltUnitController();

	bool startThread();
	bool stopThread();

	bool executeCmd(const std::string scmd, int steps = 0);
	bool isBusy() { return busy; }

private:
	TxtPanTiltUnit* ptu;
	int mode;
	std::string scmd;
	bool busy;
	int steps;

	//Thread
	volatile bool m_stoprequested;
	volatile bool m_running;
	pthread_mutex_t m_mutex;
	pthread_t m_thread;

	void run();

	// This is the static class function that serves as a C style function pointer
	// for the pthread_create call
	static void* start_thread(void *obj)
	{
		//All we do here is call the do_work() function
		reinterpret_cast<TxtPanTiltUnitController*>(obj)->run();
		return 0;
	}
};


} /* namespace ft */


#endif /* TXTPANTILTUNIT_H_ */
