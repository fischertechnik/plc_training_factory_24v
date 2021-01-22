/*
 * TxtHighBayWarehouse.h
 *
 *  Created on: 07.02.2019
 *      Author: steiger-a
 *		Edited: Mark-Oliver Masur
 */

#ifndef TxtHighBayWarehouse_H_
#define TxtHighBayWarehouse_H_

#ifndef __DOCFSM__
#include "TxtSimulationModel.h"
#include "TxtCalibData.h"
#include "TxtAxis1RefSwitch.h"
#include "TxtAxisNSwitch.h"
#include "TxtConveyorBelt.h"
#include "TxtHighBayWarehouseStorage.h"
#include "TxtMqttFactoryClient.h"
#include "TxtSimulationModel.h"

#include <queue>
#endif


#ifdef FSM_DECLARE_STATE_XE
 #undef FSM_DECLARE_STATE_XE
#endif
#define FSM_DECLARE_STATE_XE( stateName, attr... ) stateName


namespace ft {


class TxtHighBayWarehouseObserver;


typedef enum
{
	HBWCALIB_CV = 0,
	HBWCALIB_A1,
	HBWCALIB_B2,
	HBWCALIB_C3,
	HBWCALIB_END
} TxtHbwCalibPos_t;

inline const char * toString(TxtHbwCalibPos_t v)
{
	switch(v) {
	case HBWCALIB_CV: return "CV";
	case HBWCALIB_A1: return "A1";
	case HBWCALIB_B2: return "B2";
	case HBWCALIB_C3: return "B3";
	default: return "";
	}
}


class TxtHighBayWarehouseCalibData : public ft::TxtCalibData {
public:
	TxtHighBayWarehouseCalibData()
		: TxtCalibData("Data/Calib.HBW.json") {};
	virtual ~TxtHighBayWarehouseCalibData() {}

	bool load();
	bool saveDefault();
	bool save();

	uint16_t hbx[3];
	uint16_t hby[3];
	EncPos2 conv;
};


class TxtJoystickXYBController;
class TxtHighBayWarehouse : public TxtSimulationModel {
	friend class TxtJoystickXYBController;
public:
	const int ydelta = 40;

	enum State_t
	{
		__NO_STATE,
		FSM_DECLARE_STATE_XE( IDLE, color=green ),
		FSM_DECLARE_STATE_XE( INIT, color=blue ),
		FSM_DECLARE_STATE_XE( FAULT, color=red ),
		FSM_DECLARE_STATE_XE( FETCH_CONTAINER, color=blue ),
		FSM_DECLARE_STATE_XE( STORE_WP, color=blue ),
		FSM_DECLARE_STATE_XE( FETCH_WP, color=blue ),
		FSM_DECLARE_STATE_XE( FETCH_WP_WAIT, color=blue ),
		FSM_DECLARE_STATE_XE( STORE_CONTAINER, color=blue ),
		FSM_DECLARE_STATE_XE( CALIB_HBW, color=orange ),
		FSM_DECLARE_STATE_XE( CALIB_HBW_NAV, color=orange ),
		FSM_DECLARE_STATE_XE( CALIB_HBW_MOVE, color=orange ),
	};

	enum RequestType_t {
		RT_UNDEFINED,
		RT_FETCH_CONTAINER,
		RT_FETCH_WORKPIECE
	};

	struct Request_t {
		Request_t() {}
		Request_t(RequestType_t t, TxtWorkflowState r): wS(r), type(t) {}
		TxtWorkflowState wS;
		RequestType_t type;
	};

	inline const char * toString(State_t state)
	{
		#define _CASE_ITEM( _state ) case _state: return #_state;
		switch( state )
		{
		   _CASE_ITEM( IDLE )
		   _CASE_ITEM( INIT )
		   _CASE_ITEM( FAULT )
		   _CASE_ITEM( FETCH_CONTAINER )
		   _CASE_ITEM( STORE_WP )
		   _CASE_ITEM( FETCH_WP )
		   _CASE_ITEM( FETCH_WP_WAIT )
		   _CASE_ITEM( STORE_CONTAINER )
		   _CASE_ITEM( CALIB_HBW )
		   _CASE_ITEM( CALIB_HBW_NAV )
		   _CASE_ITEM( CALIB_HBW_MOVE )
		   default: break;
		}
		return "[TxtHighBayWarehouse::State_t] Unknown State";
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

	TxtHighBayWarehouse(TxtTransfer* pT, ft::TxtMqttFactoryClient* mqttclient = 0);
	virtual ~TxtHighBayWarehouse();

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

	void requestVGRfetchContainer(TxtWorkflowState req) {
		SPDLOG_LOGGER_TRACE(spdlog::get("console"),"reqVGRfetchContainer",0);
		requests.push(Request_t(RequestType_t::RT_FETCH_CONTAINER, req));
		reqVGRfetchContainer = true;
	}
	
	void requestVGRfetch(TxtWorkflowState req) {
		SPDLOG_LOGGER_TRACE(spdlog::get("console"),"reqVGRfetch",0);
		requests.push(Request_t(RequestType_t::RT_FETCH_WORKPIECE, req));
		reqVGRfetch = true;
	}

	void requestVGRstore() {
		SPDLOG_LOGGER_TRACE(spdlog::get("console"),"reqVGRstore",0);
		reqVGRstore = true;
	}

	void requestVGRstoreContainer() {
		SPDLOG_LOGGER_TRACE(spdlog::get("console"),"requestVGRstoreContainer",0);
		reqVGRstoreContainer = true;
	}

	void requestVGRcalib() {
		SPDLOG_LOGGER_TRACE(spdlog::get("console"),"requestVGRcalib",0);
		reqVGRcalib = true;
	}

	void requestVGRresetStorage() {
		SPDLOG_LOGGER_TRACE(spdlog::get("console"),"requestVGRresetStorage",0);
		reqVGRresetStorage = true;
	}

	void requestJoyBut(TxtJoysticksData jd) {
		SPDLOG_LOGGER_TRACE(spdlog::get("console"),"requestJoyBut",0);
		joyData = jd;
		reqJoyData = true;
	}


	bool loadCalib();
	bool saveCalibDefault();

	void stop();
	void moveRef();
	/*void reset() {
		axisX.reset();
		axisY.reset();
		axisZ.reset();
	}*/
	EncPos2 getPos2() {
		EncPos2 p2;
		p2.x = axisX.getPosAbs();
		p2.y = axisY.getPosAbs();
		return p2;
	}

	void moveJoystick();

	bool store(TxtWorkpiece wp);
	bool storeContainer();
	HbwFetchResult fetchByType(TxtWorkpiece wp);
	HbwFetchResult fetch(TxtWorkpiece wp);
	bool fetchContainer();

	bool canColorBeStored(TxtWPType_t c);

	void setSpeed(int16_t s);

	TxtHighBayWarehouseStorage* getStorage() { return &storage; }
	void publishStorage() { storage.Notify(); }

	TxtAxis1RefSwitch axisX;
	TxtAxis1RefSwitch axisY;
	TxtAxisNSwitch axisZ;

protected:
	State_t currentState;
	State_t newState;
	TxtHbwCalibPos_t calibPos;
	EncPos2 lastPos2;

	EncPos2 moveConv(bool stop = false);
	EncPos2 moveCR(int i, int j);

	bool getCR(int i, int j);
	bool putCR(int i, int j);
	bool getConv(bool stop = false);
	bool putConv(bool stop = false);

	void moveCalibPos();

    /*!
     * @dotfile TxtHighBayWarehouseRun.gv
     */
	void run();

	TxtConveyorBeltLightBarriers convBelt;
	TxtHighBayWarehouseStorage storage;
	TxtHighBayWarehouseCalibData calibData;

	TxtRequestQuit reqQuit;
	double faultTime_s;
	std::queue<Request_t> requests;
	Request_t _request;
	TxtWorkflowState request;
	bool reqVGRfetchContainer;
	bool reqVGRstore;
	bool reqVGRfetch;
	bool reqVGRstoreContainer;
	bool reqVGRcalib;
	bool reqVGRresetStorage;
	TxtJoysticksData joyData;
	bool reqJoyData;

	TxtHighBayWarehouseObserver* obs_hbw;
	TxtHighBayWarehouseStorageObserver* obs_storage;

private:
   void fsmStep();
};


class TxtHighBayWarehouseObserver : public ft::Observer {
public:
	TxtHighBayWarehouseObserver(ft::TxtHighBayWarehouse* s, ft::TxtMqttFactoryClient* mqttclient)
		: _subject(s), _mqttclient(mqttclient)
	{
		SPDLOG_LOGGER_TRACE(spdlog::get("console"), "TxtHighBayWarehouseObserver",0);
		_subject->Attach(this);
	}
	virtual ~TxtHighBayWarehouseObserver() {
		SPDLOG_LOGGER_TRACE(spdlog::get("console"), "~TxtHighBayWarehouseObserver",0);
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
			_mqttclient->publishStateHBW((ft::TxtLEDSCode_t)code, "", TIMEOUT_MS_PUBLISH,_subject->isActive()?1:0,"");

			//SPDLOG_LOGGER_TRACE(spdlog::get("console"), "Update 2",0);
		}
	}
private:
	ft::TxtHighBayWarehouse *_subject;
	ft::TxtMqttFactoryClient* _mqttclient;
};


} /* namespace ft */


#endif /* TxtHighBayWarehouse_H_ */
