/*
 * TxtHighBayWarehouseRun.cpp
 *
 *  Created on: 07.02.2019
 *      Author: steiger-a
 *		Edited: Mark-Oliver Masur
 */

#ifndef __DOCFSM__
#include "TxtHighBayWarehouse.h"

#include "TxtMqttFactoryClient.h"
#include "Utils.h"
#endif

#ifdef FSM_INIT_FSM
#undef FSM_INIT_FSM
#endif
#define FSM_INIT_FSM( startState, attr... )                                \
		currentState = startState;                                         \
		newState = startState;

#ifdef FSM_TRANSITION
#undef FSM_TRANSITION
#endif
#ifdef _DEBUG
#define FSM_TRANSITION( _newState, attr... )                               \
		do                                                                 \
		{                                                                  \
			std::cerr << state2str( currentState ) << " -> "               \
			<< state2str( _newState ) << std::endl;                        \
			newState = _newState;                                          \
		}                                                                  \
		while( false )
#else
#define FSM_TRANSITION( _newState, attr... )  newState = _newState
#endif


namespace ft {


void TxtHighBayWarehouse::fsmStep()
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "fsmStep",0);

	// Entry activities ===================================================
	if( newState != currentState )
	{
		switch( newState )
		{
		//-------------------------------------------------------------
		case FAULT:
		{
			printEntryState(FAULT);
			setStatus(SM_ERROR);
			sound.error();
			faultTime_s = ft::getnowtimestamp_s();
			break;
		}
		//-----------------------------------------------------------------
		case IDLE:
		{
			printEntryState(IDLE);
			setSpeed(512);
			moveRef();
			setActStatus(false, SM_READY);
			publishStorage();
			break;
		}
		//-----------------------------------------------------------------
		case CALIB_HBW:
		{
			printEntryState(CALIB_HBW);
			sound.info2();
			moveRef();
			break;
		}
		//-----------------------------------------------------------------
		default: break;
		}
		currentState = newState;
	}

	// Do activities ==================================================
	switch( currentState )
	{
	//-----------------------------------------------------------------
	case FAULT:
	{
		printState(FAULT);
		if (reqQuit.timestamp_s >= faultTime_s && reqQuit.resume)
		{
			setStatus(SM_READY);
			sound.info1();
			FSM_TRANSITION( IDLE, color=green, label='req\nquit' );
			reqQuit.resume = false;
		}
#ifdef __DOCFSM__
		FSM_TRANSITION( FAULT, color=red, label='wait' );
#endif
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
		break;
	}
	//-----------------------------------------------------------------
	case INIT:
	{
		printState(INIT);
#ifdef __DOCFSM__ //TODO remove, needed for graph
		FSM_TRANSITION( INIT );
#endif
		moveRef();
		FSM_TRANSITION( IDLE, color=green, label='initialized' );
		break;
	}
	//-----------------------------------------------------------------
	case IDLE:
	{
		//printState(IDLE);

		if (!requests.empty()) 
		{
			//Main Acitivies
			_request = requests.front();
			requests.pop();
			request = _request.wS;

			if (_request.type == RT_FETCH_CONTAINER)
			{
				FSM_TRANSITION( FETCH_CONTAINER, color=blue, label='req fetch\ncontainer' );
				reqVGRfetchContainer = false;
			}
			else if (_request.type == RT_FETCH_WORKPIECE)
			{
				FSM_TRANSITION( FETCH_WP, color=blue, label='req fetch\nworkpiece' );
				reqVGRfetch = false;
			}
		}
		else if (reqVGRcalib) //Secondary Activities
		{
			FSM_TRANSITION( CALIB_HBW, color=orange, label='req\ncalib'  );
			reqVGRcalib = false;
		}
		else if (reqVGRresetStorage)
		{
			storage.resetStorageState();
			sound.info1();
			reqVGRresetStorage = false;
		}

#ifdef __DOCFSM__
		FSM_TRANSITION( IDLE, color=green, label='wait' );
#endif
		break;
	}
	//-----------------------------------------------------------------
	case FETCH_CONTAINER:
	{
		printState(FETCH_CONTAINER);
		if (fetchContainer())
		{
			assert(mqttclient);
			mqttclient->publishHBW_Ack(HBW_FETCHED, &(request.wp), TIMEOUT_MS_PUBLISH);

			FSM_TRANSITION( STORE_WP, color=blue, label='req\nstore' );
		}
		else
		{
			mqttclient->publishHBW_Fault(TxtFaultCodes_t::CNT_NOT_AVAILABLE, request.wp, TIMEOUT_MS_PUBLISH);
			FSM_TRANSITION( FAULT, color=red, label='error' );
		}
#ifdef __DOCFSM__
		FSM_TRANSITION( FETCH_CONTAINER, color=blue, label='wait' );
#endif
		break;
	}
	//-----------------------------------------------------------------
	case STORE_WP:
	{
		printState(STORE_WP);
		if (reqVGRstore)
		{
			if (store(request.wp))
			{
				FSM_TRANSITION( IDLE, color=green, label='workpiece\nstored' );
			}
			else
			{
				mqttclient->publishHBW_Fault(TxtFaultCodes_t::STORAGE_FAULT, request.wp, TIMEOUT_MS_PUBLISH);
				FSM_TRANSITION( FAULT, color=red, label='error' );
			}
			reqVGRstore = false;
		}
#ifdef __DOCFSM__
		FSM_TRANSITION( STORE_WP, color=blue, label='wait' );
#endif
		break;
	}
	//-----------------------------------------------------------------
	case FETCH_WP:
	{
		printState(FETCH_WP);
		assert(mqttclient);
		HbwFetchResult res = fetch(request.wp);
		if (res.result)
		{
			request.wp = res.wp;
			mqttclient->publishHBW_Ack(HBW_FETCHED, &(request.wp), TIMEOUT_MS_PUBLISH);
			FSM_TRANSITION( FETCH_WP_WAIT, color=blue, label='wait req' );
		}
		else
		{
			mqttclient->publishHBW_Fault(TxtFaultCodes_t::WP_NOT_AVAILABLE, request.wp, TIMEOUT_MS_PUBLISH);
			FSM_TRANSITION( FAULT, color=red, label='error' );
		}
#ifdef __DOCFSM__
		FSM_TRANSITION( FETCH_WP, color=blue, label='wait' );
#endif
		break;
	}
	//-----------------------------------------------------------------
	case FETCH_WP_WAIT:
	{
		printState(FETCH_WP_WAIT);
		if (reqVGRstoreContainer)
		{
			FSM_TRANSITION( STORE_CONTAINER, color=blue, label='req store\ncontainer' );
			reqVGRstoreContainer = false;
		}
#ifdef __DOCFSM__
		FSM_TRANSITION( FETCH_WP_WAIT, color=blue, label='wait' );
#endif
		break;
	}
	//-----------------------------------------------------------------
	case STORE_CONTAINER:
	{
		printState(STORE_CONTAINER);
		if (storeContainer())
		{
			FSM_TRANSITION( IDLE, color=green, label='container\nstored' );
		}
		else
		{
			FSM_TRANSITION( FAULT, color=red, label='error' );
		}
#ifdef __DOCFSM__
		FSM_TRANSITION( STORE_CONTAINER, color=blue, label='wait' );
#endif
		break;
	}
	//-----------------------------------------------------------------
	case CALIB_HBW:
	{
		printState(CALIB_HBW);
		setStatus(SM_CALIB);
		calibPos = (TxtHbwCalibPos_t)0;
		FSM_TRANSITION( CALIB_HBW_NAV, color=orange, label='init' );
		break;
	}
	//-----------------------------------------------------------------
	case CALIB_HBW_NAV:
	{
		printState(CALIB_HBW_NAV);
		bool reqmove = true;
		while(true)
		{
			if (joyData.b1) {
				break;
			} else if (joyData.b2) {
				assert(mqttclient);
				mqttclient->publishHBW_Ack(HBW_CALIB_END, 0, TIMEOUT_MS_PUBLISH);
				FSM_TRANSITION( IDLE, color=green, label='cancel' );
				break;
			} else if (joyData.aX2 > 500) {
				calibPos=(TxtHbwCalibPos_t)(calibPos-1);
				std::cout << ft::toString(calibPos) << std::endl;
				reqmove = true;
			} else if (joyData.aX2 < -500) {
				calibPos=(TxtHbwCalibPos_t)(calibPos+1);
				std::cout << ft::toString(calibPos) << std::endl;
				reqmove = true;
			}
			//check pos valid
			if (calibPos < 0)
			{
				calibPos = (TxtHbwCalibPos_t)(HBWCALIB_END-1);
			} else if (calibPos >= HBWCALIB_END)
			{
				calibPos = (TxtHbwCalibPos_t)0;
			}
			//move pos
			if (reqmove)
			{
				moveCalibPos();
				setStatus(SM_CALIB);
				assert(mqttclient);
				mqttclient->publishHBW_Ack(HBW_CALIB_NAV, 0, TIMEOUT_MS_PUBLISH);
				FSM_TRANSITION( CALIB_HBW_MOVE, color=orange, label='calib pos' );
				reqmove = false;
				break;
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}
#ifdef __DOCFSM__
		FSM_TRANSITION( CALIB_HBW_NAV, color=orange, label='select pos' );
#endif
		break;
	}
	//-----------------------------------------------------------------
	case CALIB_HBW_MOVE:
	{
		printState(CALIB_HBW_MOVE);
		//reset();
		while(true)
		{
			if (joyData.b2) {
				break; //-> NAV
			} else if (joyData.b1) {
				EncPos2 p2 = getPos2();
				switch(calibPos)
				{
				case HBWCALIB_A1:
					calibData.hbx[0] = p2.x;
					calibData.hby[0] = p2.y;
					calibData.save();
					break;
				case HBWCALIB_B2:
					calibData.hbx[1] = p2.x;
					calibData.hby[1] = p2.y;
					calibData.save();
					break;
				case HBWCALIB_C3:
					calibData.hbx[2] = p2.x;
					calibData.hby[2] = p2.y;
					calibData.save();
					break;
				case HBWCALIB_CV:
					calibData.conv.x = p2.x;
					calibData.conv.y = p2.y;
					calibData.save();
					break;
				default:
					break;
				}
				//same pos again
				break; //-> NAV
			} else if ((joyData.aX2 > 500)|(joyData.aX2 < -500)) {
				break; //-> NAV
			}
			moveJoystick();
#ifdef __DOCFSM__
			FSM_TRANSITION( CALIB_HBW_MOVE, color=orange, label='move' );
#endif
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}
		FSM_TRANSITION( CALIB_HBW_NAV, color=green, label='ok' );
		break;
	}
	//-----------------------------------------------------------------
	default: assert( 0 ); break;
	}

	if( newState == currentState )
		return;

	// Exit activities ================================================
	switch( currentState )
	{
	//-----------------------------------------------------------------
	//-----------------------------------------------------------------
	default: break;
	}
}

void TxtHighBayWarehouse::moveCalibPos()
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "moveCalibPos",0);
	switch(calibPos)
	{
	case HBWCALIB_CV:
		moveRef();
		moveConv(false);
		break;
	case HBWCALIB_A1:
		moveRef();
		moveCR(0,0);
		break;
	case HBWCALIB_B2:
		moveRef();
		moveCR(1,1);
		break;
	case HBWCALIB_C3:
		moveRef();
		moveCR(2,2);
		break;
	default:
		assert(0);
		break;
	}
}

void TxtHighBayWarehouse::run()
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "run",0);
	assert(mqttclient);
	obs_hbw = new TxtHighBayWarehouseObserver(this, mqttclient);
	obs_storage = new TxtHighBayWarehouseStorageObserver(getStorage(), mqttclient);

	FSM_INIT_FSM(INIT, color=black, label='init' );
	while (!m_stoprequested)
	{
		fsmStep();
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}

	assert(mqttclient);
	mqttclient->publishHBW_Ack(HBW_EXIT, 0, TIMEOUT_MS_PUBLISH);
}


} /* namespace ft */
