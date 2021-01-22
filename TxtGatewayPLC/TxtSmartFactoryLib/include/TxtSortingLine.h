/*
 * TxtSortingLine.h
 *
 *  Created on: 03.04.2019
 *      Author: steiger-a
 */

#ifndef TXTSORTINGLINE_H_
#define TXTSORTINGLINE_H_

#ifndef __DOCFSM__
#include "KeLibTxtDl.h"     // TXT Lib
#include "FtShmem.h"        // TXT Transfer Area

#include "TxtAxis.h"
#include "TxtCalibData.h"
#include "TxtSimulationModel.h"
#include "TxtConveyorBelt.h"
#include "TxtFactoryTypes.h"
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


class TxtSortingLineObserver;

extern uint16_t u16Counter;


class TxtSortingLineCalibData : public ft::TxtCalibData {
public:
	TxtSortingLineCalibData()
		: TxtCalibData("Data/Calib.SLD.json") {};
	virtual ~TxtSortingLineCalibData() {}

	bool load();
	bool saveDefault();
	bool save();

	int color_th[2];

	int count_white;
	int count_red;
	int count_blue;
};


class TxtSortingLine : public ft::TxtSimulationModel {
public:
	const uint16_t COUNT_WRONG = 29;

	enum State_t
	{
		__NO_STATE,
		FSM_DECLARE_STATE_XE( FAULT, color=red ),
		FSM_DECLARE_STATE_XE( INIT, color=blue ),
		FSM_DECLARE_STATE_XE( IDLE, color=green ),
		FSM_DECLARE_STATE_XE( START, color=blue ),
		FSM_DECLARE_STATE_XE( COLOR_DETECTION, color=blue ),
		FSM_DECLARE_STATE_XE( START_COUNT, color=blue ),
		FSM_DECLARE_STATE_XE( CHECK_COUNT, color=blue ),
		FSM_DECLARE_STATE_XE( EJECTION_WHITE, color=blue ),
		FSM_DECLARE_STATE_XE( EJECTION_RED, color=blue ),
		FSM_DECLARE_STATE_XE( EJECTION_BLUE, color=blue ),
		FSM_DECLARE_STATE_XE( SORTED, color=blue ),
		FSM_DECLARE_STATE_XE( CALIB_SLD, color=orange ),
		FSM_DECLARE_STATE_XE( CALIB_SLD_DETECTION, color=orange ),
		FSM_DECLARE_STATE_XE( CALIB_SLD_NEXT, color=orange ),
	};

	inline const char * toString(State_t state)
	{
		#define _CASE_ITEM( _state ) case _state: return #_state;
		switch( state )
		{
		   _CASE_ITEM( FAULT )
		   _CASE_ITEM( INIT )
		   _CASE_ITEM( IDLE )
		   _CASE_ITEM( START )
		   _CASE_ITEM( COLOR_DETECTION )
		   _CASE_ITEM( START_COUNT )
		   _CASE_ITEM( CHECK_COUNT )
		   _CASE_ITEM( EJECTION_WHITE )
		   _CASE_ITEM( EJECTION_RED )
		   _CASE_ITEM( EJECTION_BLUE )
		   _CASE_ITEM( SORTED )
		   _CASE_ITEM( CALIB_SLD )
		   _CASE_ITEM( CALIB_SLD_DETECTION )
		   _CASE_ITEM( CALIB_SLD_NEXT )
		   default: break;
		}
		return "[TxtSortingLine::State_t] Unknown State";
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

	TxtSortingLine(TxtTransfer* pT, ft::TxtMqttFactoryClient* mqttclient = 0);
	virtual ~TxtSortingLine();

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
	void requestMPOproduced(TxtWorkflowState req) {
		SPDLOG_LOGGER_TRACE(spdlog::get("console"),"requestMPOproduced",0);
		requests.push(req);
		reqMPOproduced = true;
	}
	void requestVGRstart() {
		SPDLOG_LOGGER_TRACE(spdlog::get("console"),"requestVGRstart",0);
		reqVGRstart = true;
	}
	void requestVGRcalib() {
		SPDLOG_LOGGER_TRACE(spdlog::get("console"),"requestVGRcalib",0);
		reqVGRcalib = true;
	}
	void requestJoyBut(TxtJoysticksData jd) {
		SPDLOG_LOGGER_TRACE(spdlog::get("console"),"requestJoyBut",0);
		joyData = jd;
		reqJoyData = true;
	}

	bool isColorSensorTriggered();
	bool isEjectionTriggered();
	bool isWhite();
	bool isRed();
	bool isBlue();

	void ejectWhite();
	void ejectRed();
	void ejectBlue();
	void setCompressor(bool on);

	int readColorValue();
	ft::TxtWPType_t getLastColor();
	ft::TxtWPType_t getDetectedColor();

protected:
	State_t currentState;
	State_t newState;

    void configInputs();

    /*!
     * @dotfile TxtSortingLineRun.gv
     */
	void run();

	TxtConveyorBelt convBelt;
	uint8_t chEW;
	uint8_t chER;
	uint8_t chEB;
	uint8_t chComp;
	int lastColorValue;
	int detectedColorValue;
	TxtWPType_t calibColor;
	int calibColorValues[3];
	TxtSortingLineCalibData calibData;

	TxtRequestQuit reqQuit;
	double faultTime_s;
	bool reqMPOproduced;
	std::queue<TxtWorkflowState> requests;
	TxtWorkflowState request;
	bool reqVGRstart;
	bool reqVGRcalib;
	TxtJoysticksData joyData;
	bool reqJoyData;

	TxtSortingLineObserver* obs_sld;

	int round;

private:
   void fsmStep();
};


class TxtSortingLineObserver : public ft::Observer {
public:
	TxtSortingLineObserver(ft::TxtSortingLine* s, ft::TxtMqttFactoryClient* mqttclient)
		: _subject(s), _mqttclient(mqttclient)
	{
		SPDLOG_LOGGER_TRACE(spdlog::get("console"), "TxtSortingLineObserver",0);
		_subject->Attach(this);
	}
	virtual ~TxtSortingLineObserver() {
		SPDLOG_LOGGER_TRACE(spdlog::get("console"), "~TxtSortingLineObserver",0);
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
			} else if (st == SM_CALIB) {
				code = 7;
			}
			_mqttclient->publishStateSLD((ft::TxtLEDSCode_t)code,"",TIMEOUT_MS_PUBLISH,_subject->isActive()?1:0,"");

			//SPDLOG_LOGGER_TRACE(spdlog::get("console"), "Update 2",0);
		}
	}
private:
	ft::TxtSortingLine *_subject;
	ft::TxtMqttFactoryClient* _mqttclient;
};


} /* namespace ft */


#endif /* TXTSORTINGLINE_H_ */
