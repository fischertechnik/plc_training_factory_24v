/*
 * TxtMultiProcessingStation.h
 *
 *  Created on: 03.04.2019
 *      Author: steiger-a
 */

#ifndef TxtMultiProcessingStation_H_
#define TxtMultiProcessingStation_H_

#ifndef __DOCFSM__
#include "TxtSimulationModel.h"
#include "TxtCalibData.h"
#include "TxtVacuumGripper.h"
#include "TxtAxisNSwitch.h"
#include "TxtConveyorBelt.h"
#include "TxtMqttFactoryClient.h"

#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"

#include <queue>
#endif


#ifdef FSM_DECLARE_STATE_XE
 #undef FSM_DECLARE_STATE_XE
#endif
#define FSM_DECLARE_STATE_XE( stateName, attr... ) stateName


namespace ft {


class TxtMultiProcessingStationObserver;


class TxtMultiProcessingStationCalibData : public ft::TxtCalibData {
public:
	TxtMultiProcessingStationCalibData()
		: TxtCalibData("Data/Calib.MPO.json") {};
	virtual ~TxtMultiProcessingStationCalibData() {}

	bool load();
	bool saveDefault();
	bool save();
};


class TxtMultiProcessingStation : public ft::TxtSimulationModel {
public:

	enum State_t
	{
		__NO_STATE,
		FSM_DECLARE_STATE_XE( FAULT, color=red ),
		FSM_DECLARE_STATE_XE( INIT, color=blue ),
		FSM_DECLARE_STATE_XE( IDLE, color=green ),
		FSM_DECLARE_STATE_XE( BURN, color=blue ),
		FSM_DECLARE_STATE_XE( VGR_TRANSPORT, color=blue ),
		FSM_DECLARE_STATE_XE( TABLE_SAW, color=blue ),
		FSM_DECLARE_STATE_XE( TABLE_BELT, color=blue ),
		FSM_DECLARE_STATE_XE( EJECT, color=blue ),
		FSM_DECLARE_STATE_XE( TRANSPORT, color=blue )
	};

	inline const char * toString(State_t state)
	{
		#define _CASE_ITEM( _state ) case _state: return #_state;
		switch( state )
		{
		   _CASE_ITEM( FAULT )
		   _CASE_ITEM( INIT )
		   _CASE_ITEM( IDLE )
		   _CASE_ITEM( BURN )
		   _CASE_ITEM( VGR_TRANSPORT )
		   _CASE_ITEM( TABLE_SAW )
		   _CASE_ITEM( TABLE_BELT )
		   _CASE_ITEM( EJECT )
		   _CASE_ITEM( TRANSPORT )
		   default: break;
		}
		return "[TxtMultiProcessingStation::State_t] Unknown State";
		#undef _CASE_ITEM
	}

	inline void printState(State_t state)
	{
		std::cout << toString(state) << std::endl;
	}
	inline void printEntryState(State_t state)
	{
		std::cout << "entry " << toString(state) << std::endl;
	}

	inline void printExitState(State_t state)
	{
		std::cout << "exit " << toString(state) << std::endl;
	}

	TxtMultiProcessingStation(TxtTransfer* pT, ft::TxtMqttFactoryClient* mqttclient = 0);
	virtual ~TxtMultiProcessingStation();

	/* remote */
	void requestQuit() {
		SPDLOG_LOGGER_TRACE(spdlog::get("console"),"requestQuit",0);
		reqQuit = TxtRequestQuit();
	}
	/* local */
	void requestExit(const std::string name) {
		std::cout << "program terminated by " << name << std::endl;
		spdlog::get("file_logger")->error("program terminated by {}",name);
		exit(1);
	}
	void requestVGRproduce(TxtWorkflowState req) {
		SPDLOG_LOGGER_TRACE(spdlog::get("console"),"requestVGRproduce",0);
		requests.push(req);
		reqVGRproduce = true;
	}
	void requestSLDstarted() {
		SPDLOG_LOGGER_TRACE(spdlog::get("console"),"requestSLDstarted",0);
		reqSLDstarted = true;
	}

	//master
	bool isEndConveyorBeltTriggered();

	void setSawOff();
	void setSawLeft();
	void setSawRight();
	void setValveEjection(bool on);
    void setCompressor(bool on);

    //extension
    bool isOvenTriggered();

    void setValveVacuum(bool on);
    void setValveLowering(bool on);
    void setValveOvenDoor(bool on);
	void setLightOven(bool on);

	TxtAxisNSwitch axisGripper;
	TxtAxisNSwitch axisOvenInOut;
	TxtAxisNSwitch axisRotTable;

protected:
	State_t currentState;
	State_t newState;

    void configInputs();

    /*!
     * @dotfile TxtMultiProcessingStationRun.gv
     */
	void run();

	uint8_t chMsaw;
	TxtVacuumGripper vgripper;
	TxtConveyorBelt convBelt;
	TxtMultiProcessingStationCalibData calibData;

	TxtRequestQuit reqQuit;
	double faultTime_s;
	std::queue<TxtWorkflowState> requests;
	TxtWorkflowState request;
	bool reqVGRproduce;
	bool reqSLDstarted;

	TxtMultiProcessingStationObserver* obs_mpo;

private:
   void fsmStep();
};


class TxtMultiProcessingStationObserver : public ft::Observer {
public:
	TxtMultiProcessingStationObserver(ft::TxtMultiProcessingStation* s, ft::TxtMqttFactoryClient* mqttclient)
		: _subject(s), _mqttclient(mqttclient)
	{
		SPDLOG_LOGGER_TRACE(spdlog::get("console"), "TxtMultiProcessingStationObserver",0);
		_subject->Attach(this);
	}
	virtual ~TxtMultiProcessingStationObserver() {
		SPDLOG_LOGGER_TRACE(spdlog::get("console"), "~TxtMultiProcessingStationObserver",0);
		_subject->Detach(this);
	}
	void Update(ft::SubjectObserver* theChangedSubject) {
		if(theChangedSubject == _subject) {
			//SPDLOG_LOGGER_TRACE(spdlog::get("console"), "Update 1",0);

			ft::TxtSimulationModel_status_t st = _subject->getStatus();
			assert(_mqttclient);
			int code = 0;
			if (st == SM_READY) {
				code = 1;
			} else if (st == SM_BUSY) {
				code = 2;
			} else if (st == SM_ERROR) {
				code = 4;
			}
			_mqttclient->publishStateMPO((ft::TxtLEDSCode_t)code,"",TIMEOUT_MS_PUBLISH,_subject->isActive()?1:0,"");

			//SPDLOG_LOGGER_TRACE(spdlog::get("console"), "Update 2",0);
		}
	}
private:
	ft::TxtMultiProcessingStation *_subject;
	ft::TxtMqttFactoryClient* _mqttclient;
};


} /* namespace ft */


#endif /* TxtMultiProcessingStation_H_ */
