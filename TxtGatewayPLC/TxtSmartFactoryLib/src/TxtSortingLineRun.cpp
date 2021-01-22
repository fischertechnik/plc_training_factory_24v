/*
 * TxtSortingLineRun.cpp
 *
 *  Created on: 03.04.2019
 *      Author: steiger-a
 */

#ifndef __DOCFSM__
#include "TxtSortingLine.h"

#include "TxtMqttFactoryClient.h"
#include "Utils.h"

#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"
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
 #define FSM_TRANSITION( _newState, attr... )                              \
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


void TxtSortingLine::fsmStep()
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "fsmStep",0);

	// Entry activities ===============================================
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
		//-------------------------------------------------------------
		case IDLE:
		{
			printEntryState(IDLE);
			setActStatus(false, SM_READY);
			break;
		}
		//-------------------------------------------------------------
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
#ifdef __DOCFSM__ //TODO remove, needed for graph
		FSM_TRANSITION( INIT );
#endif
		printState(INIT);
		FSM_TRANSITION( IDLE, color=blue, label='initialized' );
		break;
	}
	//-----------------------------------------------------------------
	case IDLE:
	{
		//printState(IDLE);
		/*TODO wait req MPO
		if (reqMPOproduced)
		{
			auto start = std::chrono::system_clock::now();
			while (!isColorSensorTriggered())
			{
				auto end = std::chrono::system_clock::now();
				auto dur = end-start;
				auto diff_s = std::chrono::duration_cast< std::chrono::duration<float> >(dur).count();
				double diff_max = 5.0;
				if (diff_s > diff_max) {
					FSM_TRANSITION( FAULT, color=red, label='timeout\n5 sec' );
					break;
				}
				std::this_thread::sleep_for(std::chrono::milliseconds(10));
			}

			assert(mqttclient);
			mqttclient->publishSLD_Ack(SLD_STARTED, ft::WP_TYPE_NONE, 0, TIMEOUT_MS_PUBLISH);

			FSM_TRANSITION( START, color=blue, label='req\nMPO' );
			reqMPOproduced = false;
		}
		if (reqVGRstart)
		{
			FSM_TRANSITION( START, color=blue, label='req\nVGR' );
			reqVGRstart = false;
		}*/
		if (isColorSensorTriggered())
		{
			if (!requests.empty()) {
				request = requests.front();
				requests.pop();
				std::cout << "request uid: " << request.uid << std::endl;
				FSM_TRANSITION( START, color=blue, label='start' );
			}
		}
		else if (reqVGRcalib)
		{
			FSM_TRANSITION( CALIB_SLD, color=orange, label='req\ncalib'  );
			reqVGRcalib = false;
		}


#ifdef __DOCFSM__
		FSM_TRANSITION( IDLE, color=green, label='wait' );
#endif
		break;
	}
	//-----------------------------------------------------------------
	case START:
	{
		printState(START);
		setActStatus(true, SM_BUSY);
		convBelt.moveRight();
		detectedColorValue = 3000;
		round = 0;
		mqttclient->publishSLD_Ack(SLD_STARTED, &request, TIMEOUT_MS_PUBLISH);
		FSM_TRANSITION( COLOR_DETECTION, color=blue, label='conv\non' );
		break;
	}
	//-----------------------------------------------------------------
	case COLOR_DETECTION:
	{
		printState(COLOR_DETECTION);
		if (readColorValue() < detectedColorValue)
		{
			std::cout << "read color value: " << lastColorValue << std::endl;
			detectedColorValue = lastColorValue;
			SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "color value [min]: {} [{}]", lastColorValue, detectedColorValue);
		}

		if (round == 500 && detectedColorValue > 1700)
		{
			std::cout << std::endl << "Detected workpiece-type is NOT valid!" << std::endl;
			assert(mqttclient);
			mqttclient->publishSLD_Fault(TxtFaultCodes_t::INVALID_WP_TYPE, &request, TIMEOUT_MS_PUBLISH);
			convBelt.stop();
			FSM_TRANSITION( FAULT, color=red, label='error' );
		}
		round++;

		if (isEjectionTriggered())
		{
			//min color is final color
			std::cout << "final color value: " << detectedColorValue << std::endl;
			if (getDetectedColor() != request.wp.type) {
				std::cout << std::endl << "Detected workpiece-type" << ft::toString(getDetectedColor())<< " does NOT match requested workpiece-type " << request.wp.type << std::endl;
				assert(mqttclient);
				mqttclient->publishSLD_Fault(TxtFaultCodes_t::WRONG_WP_TYPE, &request, TIMEOUT_MS_PUBLISH);
				convBelt.stop();
				FSM_TRANSITION( FAULT, color=red, label='error' );
			}
			else {
				FSM_TRANSITION( START_COUNT, color=blue, label='start\ncounter' );
			}
		}
#ifdef __DOCFSM__
		FSM_TRANSITION( COLOR_DETECTION, color=blue, label='find min\nvalue' );
#endif
		break;
	}
	//-----------------------------------------------------------------
	case START_COUNT:
	{
		printState(START_COUNT);
		setCompressor(true);
		// Reset counter
		u16Counter = 0;
		FSM_TRANSITION( CHECK_COUNT, color=blue, label='check\ncounter' );
		break;
	}
	//-----------------------------------------------------------------
	case CHECK_COUNT:
	{
		printState(CHECK_COUNT);
		switch (getDetectedColor())
		{
		case WP_TYPE_WHITE:
			SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "WHITE",0);
			if (u16Counter >= calibData.count_white)
			{
				FSM_TRANSITION( EJECTION_WHITE, color=blue, label='counter\nw' );
			}
			break;
		case WP_TYPE_RED:
			SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "RED",0);
			if (u16Counter >= calibData.count_red)
			{
				FSM_TRANSITION( EJECTION_RED, color=blue, label='counter\nr' );
			}
			break;
		case WP_TYPE_BLUE:
			SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "BLUE",0);
			if (u16Counter >= calibData.count_blue)
			{
				FSM_TRANSITION( EJECTION_BLUE, color=blue, label='counter\nb' );
			}
			break;
		default: //WP_TYPE_NONE
			if (u16Counter >= COUNT_WRONG)
			{
				convBelt.stop();
				setCompressor(false);
				std::cout << std::endl << "Detected workpiece-type" << ft::toString(getDetectedColor()) << "is NOT valid!" << std::endl;
				assert(mqttclient);
				mqttclient->publishSLD_Fault(TxtFaultCodes_t::INVALID_WP_TYPE, &request, TIMEOUT_MS_PUBLISH); //extend by requested and detected color
				FSM_TRANSITION( FAULT, color=red, label='counter\nwrong' );
			}
			break;
		}
		SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "counter: {}",u16Counter);
#ifdef __DOCFSM__
		FSM_TRANSITION( CHECK_COUNT, color=blue, label='check\ncounter' );
#endif
		break;
	}
	//-----------------------------------------------------------------
	case EJECTION_WHITE:
	{
		printState(EJECTION_WHITE);
		ejectWhite();
		FSM_TRANSITION( SORTED, color=blue, label='eject\nw' );
		break;
	}
	//-----------------------------------------------------------------
	case EJECTION_RED:
	{
		printState(EJECTION_RED);
		ejectRed();
		FSM_TRANSITION( SORTED, color=blue, label='eject\nr' );
		break;
	}
	//-----------------------------------------------------------------
	case EJECTION_BLUE:
	{
		printState(EJECTION_BLUE);
		ejectBlue();
		FSM_TRANSITION( SORTED, color=blue, label='eject\nb' );
		break;
	}
	//-----------------------------------------------------------------
	case SORTED:
	{
		printState(SORTED);
		convBelt.stop();
		setActStatus(false, SM_READY);

		/* TODO sorting line should work stand alone!
		auto start = std::chrono::system_clock::now();
		while (!isWhite() && !isRed() && !isBlue())
		{
			auto end = std::chrono::system_clock::now();
			auto dur = end-start;
			auto diff_s = std::chrono::duration_cast< std::chrono::duration<float> >(dur).count();
			double diff_max = 10.0;
			if (diff_s > diff_max) {
				FSM_TRANSITION( FAULT, color=red, label='timeout\n5 sec' );
				break;
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}

		if (isWhite())
		{
			if (getDetectedColor() != ft::WP_TYPE_WHITE)
			{
				FSM_TRANSITION( FAULT, color=red, label='color\nwrong W' );
				break;
			}
		}
		else if (isRed())
		{
			if (getDetectedColor() != ft::WP_TYPE_RED)
			{
				FSM_TRANSITION( FAULT, color=red, label='color\nwrong R' );
				break;
			}
		}
		else if (isBlue())
		{
			if (getDetectedColor() != ft::WP_TYPE_BLUE)
			{
				FSM_TRANSITION( FAULT, color=red, label='color\nwrong B' );
				break;
			}
		}
		else
		{
			FSM_TRANSITION( FAULT, color=red, label='color\nwrong' );
			break;
		}*/

		assert(mqttclient);
		mqttclient->publishSLD_Ack(SLD_SORTED, &request, TIMEOUT_MS_PUBLISH); //publishes ord_state back to VGR!!
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));

		FSM_TRANSITION( IDLE, color=green, label='next' );
		break;
	}
	//-----------------------------------------------------------------
	case CALIB_SLD:
	{
		printState(CALIB_SLD);
		setStatus(SM_CALIB);
		calibColorValues[0] = -1;
		calibColorValues[1] = -1;
		calibColorValues[2] = -1;
		detectedColorValue = 3000;
		convBelt.moveRight();
		calibColor = ft::TxtWPType_t::WP_TYPE_WHITE;
		FSM_TRANSITION( CALIB_SLD_DETECTION, color=orange, label='next' );
		break;
	}
	//-----------------------------------------------------------------
	case CALIB_SLD_DETECTION:
	{
		printState(CALIB_SLD_DETECTION);
		//check exit
		if (joyData.b2) {
			assert(mqttclient);
			mqttclient->publishSLD_Ack(SLD_CALIB_END, 0, TIMEOUT_MS_PUBLISH);
			convBelt.stop();
			FSM_TRANSITION( IDLE, color=green, label='cancel' );
			break;
		}

		if (readColorValue() < detectedColorValue)
		{
			detectedColorValue = lastColorValue;
			SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "color value [min]: {} [{}]", lastColorValue, detectedColorValue);
		}
		if (isEjectionTriggered())
		{
			sound.info1();
			//min color is final color
			std::cout << "color final value: " << detectedColorValue << std::endl;

			switch(calibColor)
			{
			case ft::TxtWPType_t::WP_TYPE_WHITE:
				calibColorValues[0] = detectedColorValue;
				SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "value white: {}",calibColorValues[0]);

				calibColor = ft::TxtWPType_t::WP_TYPE_RED;
				break;
			case ft::TxtWPType_t::WP_TYPE_RED:
				calibColorValues[1] = detectedColorValue;
				SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "value red: {}",calibColorValues[1]);

				calibColor = ft::TxtWPType_t::WP_TYPE_BLUE;
				break;
			case ft::TxtWPType_t::WP_TYPE_BLUE:
				calibColorValues[2] = detectedColorValue;
				SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "value blue: {}",calibColorValues[2]);
				break;
			default: assert( 0 ); break;
			}

			std::this_thread::sleep_for(std::chrono::milliseconds(1000));

			FSM_TRANSITION( CALIB_SLD_NEXT, color=orange, label='next\ncolor' );
			break;
		}
#ifdef __DOCFSM__
		FSM_TRANSITION( CALIB_SLD_DETECTION, color=orange, label='next' );
#endif
		break;
	}
	//-----------------------------------------------------------------
	case CALIB_SLD_NEXT:
	{
		printState(CALIB_SLD_NEXT);
		setStatus(SM_CALIB);

		//check finish
		SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "w:{} r:{} b:{}",calibColorValues[0],calibColorValues[1],calibColorValues[2]);
		if ((calibColorValues[0] > 0)&&
			(calibColorValues[1] > 0)&&
			(calibColorValues[2] > 0))
		{
			calibData.color_th[0] = (calibColorValues[0] + calibColorValues[1]) / 2;
			calibData.color_th[1] = (calibColorValues[1] + calibColorValues[2]) / 2;
			SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "th1:{} th2:{}",calibData.color_th[0],calibData.color_th[1]);
			//check
			if ((calibColorValues[0] < calibColorValues[1])&&
				(calibColorValues[1] < calibColorValues[2])&&
				(calibColorValues[0] < calibColorValues[2])&&
				(calibData.color_th[0] >= 200)&&
				(calibData.color_th[1] < 2000))
			{
				calibData.save();
			} else {
				sound.error();
			}
			assert(mqttclient);
			mqttclient->publishSLD_Ack(SLD_CALIB_END, 0, TIMEOUT_MS_PUBLISH);
			convBelt.stop();
			FSM_TRANSITION( IDLE, color=green, label='SLD calibrated' );
			break;
		}

		//check exit
		if (joyData.b2) {
			assert(mqttclient);
			mqttclient->publishSLD_Ack(SLD_CALIB_END, 0, TIMEOUT_MS_PUBLISH);
			convBelt.stop();
			FSM_TRANSITION( IDLE, color=green, label='cancel' );
			break;
		}

		detectedColorValue = 3000;
		FSM_TRANSITION( CALIB_SLD_DETECTION, color=orange, label='next\ncolor' );
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


void TxtSortingLine::run()
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "run",0);
	assert(mqttclient);
	obs_sld = new TxtSortingLineObserver(this, mqttclient);

	FSM_INIT_FSM( INIT, color=black, label='init' );
	while (!m_stoprequested)
	{
		fsmStep();
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}

	assert(mqttclient);
	mqttclient->publishSLD_Ack(SLD_EXIT, 0, TIMEOUT_MS_PUBLISH);
}


} /* namespace ft */
