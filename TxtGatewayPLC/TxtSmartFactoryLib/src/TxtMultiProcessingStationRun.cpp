/*
 * TxtMultiProcessingStationRun.cpp
 *
 *  Created on: 03.04.2019
 *      Author: steiger-a
 */

#ifndef __DOCFSM__
#include "TxtMultiProcessingStation.h"

#include "TxtMqttFactoryClient.h"

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


void TxtMultiProcessingStation::fsmStep()
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
		printState(INIT);
#ifdef __DOCFSM__ //TODO remove, needed for graph
		FSM_TRANSITION( INIT );
#endif
		setCompressor(true);
		setValveOvenDoor(true);
		std::this_thread::sleep_for(std::chrono::milliseconds(300));

		std::thread tG = axisGripper.moveS1Thread();
		std::thread tR = axisRotTable.moveS1Thread();
		std::thread tO = axisOvenInOut.moveS1Thread();
		tO.join();
		tR.join();
		tG.join();

		setCompressor(false);
		FSM_TRANSITION( IDLE, color=green, label='initialized' );
		break;
	}
	//-----------------------------------------------------------------
	case IDLE:
	{
		//printState(IDLE);
		if (!requests.empty())
		{	
			request = requests.front();
			requests.pop();

			auto start = std::chrono::system_clock::now();
			bool ok = true;
			while (!isOvenTriggered())
			{
				auto end = std::chrono::system_clock::now();
				auto dur = end-start;
				auto diff_s = std::chrono::duration_cast< std::chrono::duration<float> >(dur).count();
				double diff_max = 5.0;
				if (diff_s > diff_max) {
					assert(mqttclient);
					mqttclient->publishMPO_Fault(TxtFaultCodes_t::REQUEST_TIMEOUT ,&request, TIMEOUT_MS_PUBLISH);
					FSM_TRANSITION( FAULT, color=red, label='timeout\n5 sec' ); // Kein Workpiece in MPO Input
					ok = false;
					break;
				}
				std::this_thread::sleep_for(std::chrono::milliseconds(10));
			}

			if(!ok) {
				break;
			}

			assert(mqttclient);
			mqttclient->publishMPO_Ack(MPO_STARTED, &request, TIMEOUT_MS_PUBLISH);
			FSM_TRANSITION( BURN, color=blue, label='req\nVGR' );
			reqVGRproduce = false;
		}
#ifdef __DOCFSM__
		FSM_TRANSITION( IDLE, color=green, label='wait' );
#endif
		break;
	}
	//-----------------------------------------------------------------
	case BURN:
	{
		printState(BURN);

		setActStatus(true, SM_BUSY);

		//in
		setCompressor(true);
		std::this_thread::sleep_for(std::chrono::milliseconds(2500));
		axisOvenInOut.moveS2();
		setValveOvenDoor(false);
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));

		std::thread tGripper = axisGripper.moveS2Thread();

		//burn
		for(int i = 0; i < 14; i++)
		{
			setLightOven(true);
			std::this_thread::sleep_for(std::chrono::milliseconds(200));
			setLightOven(false);
			std::this_thread::sleep_for(std::chrono::milliseconds(200));
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));

		//out
		setCompressor(true);
		setValveOvenDoor(true);
		axisOvenInOut.moveS1();

		tGripper.join();

		FSM_TRANSITION( VGR_TRANSPORT, color=blue, label='burned' );
		break;
	}
	//-----------------------------------------------------------------
	case VGR_TRANSPORT:
	{
		printState(VGR_TRANSPORT);
		axisRotTable.moveS1();

		//pickup
		setValveLowering(true);
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
		setValveVacuum(true);
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
		setValveLowering(false);

		//move
		axisGripper.moveS1();

		//release
		setValveLowering(true);
		std::this_thread::sleep_for(std::chrono::milliseconds(400));
		setValveVacuum(false);
		std::this_thread::sleep_for(std::chrono::milliseconds(300));
		setValveLowering(false);
		std::this_thread::sleep_for(std::chrono::milliseconds(500));

		setCompressor(false);
		FSM_TRANSITION( TABLE_SAW, color=blue, label='transported' );
		break;
	}
	//-----------------------------------------------------------------
	case TABLE_SAW:
	{
		printState(TABLE_SAW);
		axisRotTable.moveS2();
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
		setSawRight();
		std::this_thread::sleep_for(std::chrono::milliseconds(2500));
		setSawOff();
		setSawLeft();
		std::this_thread::sleep_for(std::chrono::milliseconds(2500));
		setSawOff();
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
		FSM_TRANSITION( TABLE_BELT, color=blue, label='processed' );
		break;
	}
	//-----------------------------------------------------------------
	case TABLE_BELT:
	{
		printState(TABLE_BELT);
		axisRotTable.moveS3();
		FSM_TRANSITION( EJECT, color=blue, label='produced' );
		break;
	}
	//-----------------------------------------------------------------
	case EJECT:
	{
		printState(EJECT);
		convBelt.moveRight();
		setCompressor(true);
		std::this_thread::sleep_for(std::chrono::milliseconds(400));
		//eject
		setValveEjection(true);
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
		setValveEjection(false);
		setCompressor(false);

		assert(mqttclient);
		request.wp.state = TxtWPState_t::WP_STATE_PROCESSED;
		mqttclient->publishMPO_Ack(MPO_PRODUCED, &request, TIMEOUT_MS_PUBLISH);

		FSM_TRANSITION( TRANSPORT, color=blue, label='ejected' );
		break;
	}
	//-----------------------------------------------------------------
	case TRANSPORT:
	{
		printState(TRANSPORT);
		setActStatus(false, SM_READY);
		/* TODO
		if (reqSLDstarted)
		{*/
			auto start = std::chrono::system_clock::now();
			while (!isEndConveyorBeltTriggered())
			{
				auto end = std::chrono::system_clock::now();
				auto dur = end-start;
				auto diff_s = std::chrono::duration_cast< std::chrono::duration<float> >(dur).count();
				double diff_max = 10.0;
				if (diff_s > diff_max) {
					assert(mqttclient);
					mqttclient->publishMPO_Fault(TxtFaultCodes_t::TRANSPORT_FAULT, &request, TIMEOUT_MS_PUBLISH);
					FSM_TRANSITION( FAULT, color=red, label='timeout\n10 sec' );
					break;
				}
				std::this_thread::sleep_for(std::chrono::milliseconds(10));
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(2000));
			convBelt.stop();
			mqttclient->publishMPO_Ack(MPO_TRANSPORT, &request, TIMEOUT_MS_PUBLISH);
			FSM_TRANSITION( IDLE, color=green, label='next' );
		/*	reqSLDstarted = false;
		}
#ifdef __DOCFSM__
		FSM_TRANSITION( TRANSPORT, color=blue, label='wait' );
#endif
		*/
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


void TxtMultiProcessingStation::run()
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "run",0);
	assert(mqttclient);
	obs_mpo = new TxtMultiProcessingStationObserver(this, mqttclient);

	FSM_INIT_FSM( INIT, color=black, label='init' );
	while (!m_stoprequested)
	{
		fsmStep();
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}

	assert(mqttclient);
	mqttclient->publishMPO_Ack(MPO_EXIT, 0, TIMEOUT_MS_PUBLISH);
}


} /* namespace ft */
