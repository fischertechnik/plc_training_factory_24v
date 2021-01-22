/*
 * TxtJoystickXYBController.h
 *
 *  Created on: 08.11.2018
 *      Author: steiger-a
 */

#ifndef TXTJOYSTICKXYBCONTROLLER_H_
#define TXTJOYSTICKXYBCONTROLLER_H_

#include <stdio.h>          // for printf()
#include <unistd.h>         // for sleep()
#include <iostream>

#include "KeLibTxtDl.h"     // TXT Lib
#include "FtShmem.h"        // TXT Transfer Area

#include "Observer.h"
#include "TxtAxis.h"
#include "TxtVacuumGripperRobot.h"

#include "spdlog/spdlog.h"


namespace ft {

class TxtJoystickXYBController : public SubjectObserver {
public:
	const int JOY_MIN_TH = 7;
	const int center0 = 4100;
	const int delta0 = 1500;

	TxtJoystickXYBController(TxtTransfer* pT, uint8_t chX1, uint8_t chY1, uint8_t chB1, uint8_t chX2, uint8_t chY2, uint8_t chB2);
	virtual ~TxtJoystickXYBController();

	TxtJoysticksData getData() { return jd; }

	bool startThread();
	bool stopThread();
	bool isThreadRunning() { return m_running; }

private:
	void configInputs();

	TxtTransfer* pT;
	uint8_t chX1;
	uint8_t chY1;
	uint8_t chB1;
	uint8_t chX2;
	uint8_t chY2;
	uint8_t chB2;

	int offx1;
	int offy1;
	int offx2;
	int offy2;

	TxtJoysticksData jd;

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
		reinterpret_cast<TxtJoystickXYBController*>(obj)->run();
		return 0;
	}
};


} /* namespace ft */


#endif /* TXTJOYSTICKXYBCONTROLLER_H_ */
