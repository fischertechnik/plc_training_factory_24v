/*
 * TxtSimulationModel.h
 *
 *  Created on: 08.11.2018
 *      Author: steiger-a
 */

/*! \page "Overview State Machines" Overview State Machines
  The overview of the state machines implemented is shown in these sections.

  \section SLD Sorting Line
  @dotfile TxtSortingLineRun.gv

  \section MPO Multi Processing Station
  @dotfile TxtMultiProcessingStationRun.gv

  \section HBW High-Bay Warehouse
  @dotfile TxtHighBayWarehouseRun.gv

  \section VGR Vacuum Gripper Robot
  @dotfile TxtVacuumGripperRobotRun.gv
*/


#ifndef TXTSIMULATIONMODEL_H_
#define TXTSIMULATIONMODEL_H_


#include <stdio.h>          // for printf()
#include <unistd.h>         // for sleep()
#include <iostream>

#include "KeLibTxtDl.h"     // TXT Lib
#include "FtShmem.h"        // TXT Transfer Area

#include "TxtAxis.h"
#include "Observer.h"
#include "TxtSound.h"

#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"


namespace ft {


class TxtMqttFactoryClient;


typedef enum
{
	SM_NONE = 0,
	SM_ERROR = 1,
	SM_NOREF = 2,
	SM_READY = 3,
	SM_BUSY = 4,
	SM_CALIB = 5
} TxtSimulationModel_status_t;


class TxtSimulationModel : public SubjectObserver {
public:
	TxtSound sound;

	TxtSimulationModel(TxtTransfer* pT, ft::TxtMqttFactoryClient* mqttclient);
	virtual ~TxtSimulationModel();

	TxtSimulationModel_status_t getStatus() { return status; }
	bool isActive() { return active; }

	bool startThread();
	bool stopThread();
	bool isThreadRunning() { return m_running; }

protected:
	void setStatus(TxtSimulationModel_status_t s) { status = s; Notify(); }
	void setActStatus(bool a, TxtSimulationModel_status_t s) { active = a; status = s; Notify(); }

	ft::TxtMqttFactoryClient* mqttclient;
	TxtSimulationModel_status_t status;
	bool active;
	TxtTransfer* pT;

	//Thread
	volatile bool m_stoprequested;
	volatile bool m_running;
	pthread_mutex_t m_mutex;
	pthread_t m_thread;

	virtual void run() = 0;

	// This is the static class function that serves as a C style function pointer
	// for the pthread_create call
	static void* start_thread(void *obj)
	{
		//All we do here is call the do_work() function
		reinterpret_cast<TxtSimulationModel*>(obj)->run();
		return 0;
	}
};


} /* namespace ft */


#endif /* TXTSIMULATIONMODEL_H_ */
