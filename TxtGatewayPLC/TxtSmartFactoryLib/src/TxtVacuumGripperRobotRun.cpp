/*
 * TxtVacuumGripperRobotRun.cpp
 *
 *  Created on: 08.11.2018
 *      Author: steiger-a
 *		Edited: Mark-Oliver Masur
 */

#ifndef __DOCFSM__
#include "TxtVacuumGripperRobot.h"

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


void TxtVacuumGripperRobot::fsmStep()
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "fsmStep",0);
	
	if (reqOrder)
	{
		TxtOrderState tmp_order_state = reqWP_orders.front();
		assert(mqttclient);
		mqttclient->publishStateOrder(tmp_order_state, TIMEOUT_MS_PUBLISH);
		reqOrder = false;
	}

	if (reqPickup)
	{
		TxtPickupState tmp_pickup_state = reqWP_pickups.front();
		assert(mqttclient);
		mqttclient->publishStatePickup(tmp_pickup_state, TIMEOUT_MS_PUBLISH);
		reqPickup = false;
	}

	if (reqStore)
	{
		TxtStoreState tmp_store_state = reqWP_stores.front();
		assert(mqttclient);
		mqttclient->publishStateStore(tmp_store_state, TIMEOUT_MS_PUBLISH);
		reqStore = false;
	}

	// joystick released?
	if (joyData.aY2 <= 10 && joyData.aY2 >= -10)
	{
		allowWorkingModeChange = true;
	}

	if (joyData.aY2 < -500 && allowWorkingModeChange)
	{
		allowWorkingModeChange = false;
		changeWorkingMode = true;
	}

	if (changeWorkingMode)
	{
		if (workingModeNew != TxtVgrWorkingModes_t::UNDEFINED) {
			workingMode = workingModeNew;
			workingModeNew = TxtVgrWorkingModes_t::UNDEFINED;
			std::cout << "WorkingMode switched to '" << workingMode << "'" << std::endl;
		}
		else if (workingMode == TxtVgrWorkingModes_t::NORMAL)
		{
			workingMode = TxtVgrWorkingModes_t::STORE_AFTER_PRODUCE;
			std::cout << "WorkingMode switched to 'STORE_AFTER_PRODUCE'" << std::endl;
		}
		else if (workingMode == TxtVgrWorkingModes_t::STORE_AFTER_PRODUCE)
		{
			workingMode = TxtVgrWorkingModes_t::ENDLESS;
			std::cout << "WorkingMode switched to 'ENDLESS'" << std::endl;
		}
		else if (workingMode == TxtVgrWorkingModes_t::ENDLESS)
		{
			workingMode = TxtVgrWorkingModes_t::NORMAL;
			std::cout << "WorkingMode switched to 'NORMAL'" << std::endl;
		}
		else {
			std::cout << "WorkingMode unknown. This is an internal error." << std::endl;
			spdlog::get("file_logger")->error("WorkingMode unknown.", 0);
			sound.error();
			exit(1);
		}
		sound.warn();
		changeWorkingMode = false;
		assert(mqttclient);
		mqttclient->pusblishWorkingModeChanged(workingMode, TIMEOUT_MS_PUBLISH);
	}
	// Entry activities ===================================================
	if( newState != currentState )
	{	
		switch( newState )
		{
		//-----------------------------------------------------------------
		case FAULT:
		{
			printEntryState(FAULT);
			setStatus(SM_ERROR);
			sound.error();
			release();
			faultTime_s = ft::getnowtimestamp_s();
			break;
		}
		//-----------------------------------------------------------------
		case IDLE:
		{
			printEntryState(IDLE);
			dps.Notify();
			release();
			setSpeed(512);
			moveRef();
			setActStatus(false, SM_READY);
			dps.setActiveDSI(false);
			dps.setActiveDSI(false);
			// WorkflowState Cleanup
			resetStoreState();
			resetPickupState();
			resetOrderState();

			break;
		}
		//-----------------------------------------------------------------
		case CALIB_VGR:
		{
			printEntryState(CALIB_VGR);
			sound.info2();
			moveRef();
			break;
		}
		//-----------------------------------------------------------------
		case START_DELIVERY:
		{
			printEntryState(START_DELIVERY);
			dps.setErrorDSI(false);
			dps.setActiveDSI(true);
			break;
		}
		//-----------------------------------------------------------------
		default:
			break;
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
			reqHBWFault = false;
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
		FSM_TRANSITION( INIT, color=blue, label='wait' );
#endif
		printState(INIT);
		moveRef();
		FSM_TRANSITION( IDLE, color=green, label='initialized' );
		break;
	}
	//-----------------------------------------------------------------
	case IDLE:
	{
		//printState(IDLE);
		
		//NFC requests
		if (reqNfcDelete)
		{
			if (dps.nfcDelete())
			{
				dps.publishNfc();
				reqNfcDelete = false;
			}
		}

		if (reqNfcRead)
		{
			if (dps.nfcRead().empty())
			{
				dps.publishNfc();
				reqNfcRead = false;
			}
		}

		if (reqSLDFault || reqMPOFault) {
			assert(mqttclient);
			order_state.state = PROCESS_FAULT;
			mqttclient->publishStateOrder(order_state, TIMEOUT_MS_PUBLISH);
			FSM_TRANSITION( FAULT, color=red, label='nfc error' );

			// TODO: Put order back into orders queue or restart order process with the same order_state (here available: order_state)

			reqSLDFault = false;
			reqMPOFault = false;
			break;
		}

		if(wp_waiting_for_pickup || !wps_waiting_for_pickup.empty())
		{
			std::cout << "acknowledge wp_waiting_for_pickup" << std::endl;
			FSM_TRANSITION( PICKUP2DELIVERY, color=blue, label='next' );
			wp_waiting_for_pickup = false;
			wps_waiting_for_pickup.pop();
		}
		// activities
		else if (reqSLDsorted || !reqWP_sorted.empty())
		{
			setTarget("dso");
			order_state = reqWP_sorted.front();
			reqWP_sorted.pop();
			order_state.wp.printDebug();
			assert(mqttclient);
			order_state.state = ORDER_PROCESSED;
			mqttclient->publishStateOrder(order_state, TIMEOUT_MS_PUBLISH);
			if (order_state.wp.type == WP_TYPE_WHITE)
			{
				moveSSD1();
			}
			else if (order_state.wp.type == WP_TYPE_RED)
			{
				moveSSD2();
			}
			else if (order_state.wp.type == WP_TYPE_BLUE)
			{
				moveSSD3();
			} else {
				FSM_TRANSITION( FAULT, color=red, label='nfc error' );
				break;
			}
			if (order_state.wp.type != WP_TYPE_NONE)
			{
				FSM_TRANSITION( NFC_PRODUCED, color=blue, label='req sorted' );
			}
			reqSLDsorted = false;
			break;
		}
		else if (!dps.is_DIN())
		{
			storeProcessedWorkpiece = false;
			FSM_TRANSITION( DSI_PICKUP_DELIVERY, color=blue, label='dsi' );
			break;
		}
		else if (!reqWP_orders.empty())
		{
			order_state = reqWP_orders.front();
			reqWP_orders.pop();
			order_state.state = FETCHING_FROM_STORAGE;
			assert(mqttclient);
			mqttclient->publishStateOrder(order_state, TIMEOUT_MS_PUBLISH);
			FSM_TRANSITION( FETCH_WP_VGR_ORDER, color=blue, label='req order' );
			break;
		}
		else if (!reqWP_pickups.empty() && !reqPickupError )
		{
			pickup_state = reqWP_pickups.front();

			if (!dps.is_DOUT()) {
				std::cout << "Pickup: DSO blocked" << std::endl;
				reqPickupError = true;
				pickup_state.state = DSO_BLOCKED;
				assert(mqttclient);
				mqttclient->publishStatePickup(pickup_state, TIMEOUT_MS_PUBLISH);
				sound.warn();
			}
			else {
				reqStoreError = false;
				reqWP_pickups.pop();
				pickup_state.state = TxtPickupState_t::PICKUP_ACKNOWLEDGED;
				assert(mqttclient);
				mqttclient->publishStatePickup(pickup_state, TIMEOUT_MS_PUBLISH);

				FSM_TRANSITION( FETCH_WP_VGR_PICKUP, color=blue, label='req order' );
			}
			break;
		}
		else if (!reqWP_stores.empty() && !reqStoreError)
		{
			store_state = reqWP_stores.front();
			if (dps.is_DOUT()) {
				std::cout << "Store: DSO empty" << std::endl;
				reqStoreError = true;
				store_state.state = DSO_EMPTY;
				assert(mqttclient);
				mqttclient->publishStateStore(store_state, TIMEOUT_MS_PUBLISH);
				sound.warn();
			}
			else {
				reqPickupError = false;
				reqWP_stores.pop();
				store_state.state = TxtStoreState_t::STORE_ACKNOWLEDGED;
				assert(mqttclient);
				mqttclient->publishStateStore(store_state, TIMEOUT_MS_PUBLISH);

				FSM_TRANSITION( STORE_FROM_DSO, color=blue, label='req order' );
			}
			break;
		}
		// Edge-case scenario, where Store and Pickup Request block eachother, can be artificially forced, shouldnt happen in endless mode.
		else if ((reqStoreError || reqPickupError) && reqQuit.resume) {
			reqQuit.resume = false;
			reqStoreError = false;
			reqPickupError = false;
			break;
		}
		else if (reqStoreError && reqPickupError) { // Error Handling to prevent Deadlock
			std::cout << "\nPickup and Store Deadlock\nTo resolve either place wp in DSO or empty DSO\n" << std::endl;
			reqPickupError = false;
			reqStoreError = false;
			FSM_TRANSITION( FAULT, color=red, label='prevent deadlock');
		}
		else if (joyData.aX2 > 500)
		{
			if (order_state.state == WAITING_FOR_ORDER)
			{
				requestOrder(WP_TYPE_WHITE);
			}
		}
		else if (joyData.aY2 > 500)
		{
			if (order_state.state == WAITING_FOR_ORDER)
			{
				requestOrder(WP_TYPE_RED);
			}
		}
		else if (joyData.aX2 < -500)
		{
			if (order_state.state == WAITING_FOR_ORDER)
			{
				requestOrder(WP_TYPE_BLUE);
			}
		}
		else
		{
			std::string uid = dps.nfcReadUID();
			SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "uid: {}",uid);
			
			if (uid.length()==8) //command uid has length=8
			{
				SPDLOG_LOGGER_TRACE(spdlog::get("console"), "tag_uid: {}", uid);
				//set action uids if calibData is empty
				if (joyData.aX1 > 500 || dps.getUIDResetHBW().empty())
				{
					dps.saveUIDResetHBW(uid);
				}
				else if (((joyData.aX1 < -500) || dps.getUIDResetHBW()!=uid) && (dps.getUIDCalibMode().empty()))
				{
					dps.saveUIDCalibMode(uid);
				}
				//check uids
				if (uid == dps.getUIDResetHBW())
				{
					assert(mqttclient);
					mqttclient->publishVGR_Do(VGR_HBW_RESETSTORAGE, 0, TIMEOUT_MS_PUBLISH);
					mqttclient->publishStateVGR(ft::LEDS_WAIT_READY, "", TIMEOUT_MS_PUBLISH, 0, "");
					std::this_thread::sleep_for(std::chrono::milliseconds(2000));
					mqttclient->publishStateVGR(ft::LEDS_READY, "", TIMEOUT_MS_PUBLISH, 0, "");
				}
				else if (uid == dps.getUIDCalibMode())
				{
					FSM_TRANSITION( CALIB_VGR, color=orange, label='cmd calib' );
				}
				/*else if (uid == dps.getUIDOrderWHITE())*/
				/*else if (uid == dps.getUIDOrderRED())*/
				/*else if (uid == dps.getUIDOrderBLUE())*/
			}
		}
#ifdef __DOCFSM__
		FSM_TRANSITION( IDLE, color=green, label='wait' );
#endif
		break;
	}
	//-----------------------------------------------------------------
	case FETCH_WP_VGR_ORDER:
	{
		printState(FETCH_WP_VGR_ORDER);

		assert(mqttclient);
		order_state.wp.printDebug();
		mqttclient->publishVGR_Do(VGR_HBW_FETCH_WP, &order_state, TIMEOUT_MS_PUBLISH);

		setTarget("hbw");
		moveFromHBW1();

		FSM_TRANSITION( VGR_WAIT_FETCHED, color=green, label='fetched' );
		break;
	}
	case FETCH_WP_VGR_PICKUP:
		{
			printState(FETCH_WP_VGR_PICKUP);

			assert(mqttclient);
			pickup_state.wp.printDebug();
			mqttclient->publishVGR_Do(VGR_HBW_FETCH_WP, &pickup_state, TIMEOUT_MS_PUBLISH);
			pickup_state.state = FETCHING_WP;
			mqttclient->publishStatePickup(pickup_state, TIMEOUT_MS_PUBLISH);

			setTarget("hbw");
			moveFromHBW1();

			FSM_TRANSITION( VGR_WAIT_FETCHED_PICKUP, color=green, label='fetched' );
			break;
		}
	//-----------------------------------------------------------------
	case VGR_WAIT_FETCHED:
	{
		printState(VGR_WAIT_FETCHED);
		if (reqHBWFault)
		{
			assert(mqttclient);
			order_state.state = ORDER_FAULT;
			mqttclient->publishStateOrder(order_state, TIMEOUT_MS_PUBLISH);
			FSM_TRANSITION( FAULT, color=red, label='error' );
			reqHBWFault = false;
		}
		if (reqHBWfetched)
		{
			order_state.wp = reqWP_HBW;
			order_state.state = FETCHED_FROM_STORAGE;
	
			moveFromHBW2();

			assert(mqttclient);
			mqttclient->publishVGR_Do(VGR_HBW_STORECONTAINER, &order_state, TIMEOUT_MS_PUBLISH);
			mqttclient->publishStateOrder(order_state, TIMEOUT_MS_PUBLISH);

			proStorage.setTimestampNow(order_state.wp.tag_uid, OUTSOURCING_INDEX);

			reqHBWfetched = false;
			FSM_TRANSITION( MOVE_VGR2MPO, color=green, label='transport to MPO' );
		}
#ifdef __DOCFSM__
		FSM_TRANSITION( VGR_WAIT_FETCHED, color=blue, label='wait' );
#endif
		break;
	}
	case VGR_WAIT_FETCHED_PICKUP:
		{
			printState(VGR_WAIT_FETCHED_PICKUP);
			if (reqHBWFault)
			{
				assert(mqttclient);
				pickup_state.state = PICKUP_FAULT;
				mqttclient->publishStatePickup(pickup_state, TIMEOUT_MS_PUBLISH);
				FSM_TRANSITION( FAULT, color=red, label='error' );
				reqHBWFault = false;
			}
			if (reqHBWfetched)
			{
				pickup_state.wp = reqWP_HBW;

				moveFromHBW2();

				assert(mqttclient);
				mqttclient->publishVGR_Do(VGR_HBW_STORECONTAINER, &pickup_state, TIMEOUT_MS_PUBLISH);
				mqttclient->publishStatePickup(pickup_state, TIMEOUT_MS_PUBLISH);

				reqHBWfetched = false;
				FSM_TRANSITION( HBW2PICKUP, color=green, label='transport to pickup_station' );
			}
	#ifdef __DOCFSM__
			FSM_TRANSITION( VGR_WAIT_FETCHED_PICKUP, color=blue, label='wait' );
	#endif
			break;
		}
	//-----------------------------------------------------------------
	case MOVE_VGR2MPO:
	{
		printState(MOVE_VGR2MPO);
		setTarget("mpo");
		moveMPO();

		order_state.wp.printDebug();

		//duplicated with commented code below in START_PRODUCE (remove this and keep on below)
		//order_state.wp.type = reqWP_MPO->type;
		order_state.state = TRANSPORT_TO_PROCESS_STATION;
		mqttclient->publishStateOrder(order_state, TIMEOUT_MS_PUBLISH);

		std::this_thread::sleep_for(std::chrono::milliseconds(100));

		assert(mqttclient);
		mqttclient->publishVGR_Do(VGR_MPO_PRODUCE, &order_state, TIMEOUT_MS_PUBLISH);
		proStorage.setTimestampNow(order_state.wp.tag_uid, PROCESSING_OVEN_INDEX);
		releaseMPO();
		moveRef();

		FSM_TRANSITION( START_PRODUCE, color=blue, label='produce' );
		break;
	}
	//-----------------------------------------------------------------
	case START_PRODUCE:
	{
		printState(START_PRODUCE);
		if (reqMPOFault) {
			assert(mqttclient);
			order_state.state = PROCESS_FAULT;
			mqttclient->publishStateOrder(order_state, TIMEOUT_MS_PUBLISH);
			FSM_TRANSITION( FAULT, color=red, label='error in mpo' );
		}
		else if (reqMPOstarted)
		{
			order_state.state = IN_PROCESS;
			mqttclient->publishStateOrder(order_state, TIMEOUT_MS_PUBLISH);

			FSM_TRANSITION( IDLE, color=green, label='transport to MPO' );
			reqMPOstarted = false;
		}
#ifdef __DOCFSM__
		FSM_TRANSITION( START_PRODUCE, color=blue, label='wait' );
#endif
		break;
	}
	//-----------------------------------------------------------------
	case DSI_PICKUP_DELIVERY:
	{
		printState(DSI_PICKUP_DELIVERY);
		moveDeliveryInAndGrip();
		storeProcessedWorkpiece = false;
		FSM_TRANSITION( START_DELIVERY, color=green, label="start delivery");
		break;
	}
	case START_DELIVERY:
	{
		printState(START_DELIVERY);
		store_state = TxtStoreState(TxtStoreState_t::STORE_ACKNOWLEDGED);
		mqttclient->publishStateStore(store_state, TIMEOUT_MS_PUBLISH);

		setTarget("hbw");
		moveNFC();
		std::string uid = dps.nfcReadUID();
		if (uid.empty())
		{
			trashItem = true;
			FSM_TRANSITION( WRONG_COLOR, color=red, label='empty tag' );
			assert(mqttclient);
			store_state.state = BAD_WORKPIECE;
			mqttclient->publishStateStore(store_state, TIMEOUT_MS_PUBLISH);
			break;
		} else {
			
			dps.nfcRead();
			store_state.wp = dps.getNfcData()->wp;
			if (storeProcessedWorkpiece && (store_state.wp.state != WP_STATE_NONE && store_state.wp.type != WP_TYPE_NONE)) {
				std::cout << "store Processed Workpiece" << std::endl;
				FSM_TRANSITION( STORE_WP_VGR, color=blue, label='store processed workpiece' );
			}
			else {
				std::cout << "store Raw Workpiece" << std::endl;
				proStorage.resetTagUidMaskTs(store_state.wp.tag_uid);
				proStorage.setTimestampNow(store_state.wp.tag_uid, DELIVERY_RAW_INDEX);
				FSM_TRANSITION( COLOR_DETECTION, color=blue, label='store raw workpiece' );
			}

			break;
		}
#ifdef __DOCFSM__
		FSM_TRANSITION( START_DELIVERY, color=blue, label='wait' );
#endif
		break;
	}
	//-----------------------------------------------------------------
	case COLOR_DETECTION:
	{
		printState(COLOR_DETECTION);
		assert(mqttclient);
		store_state.state = IDENTIFICATION;
		mqttclient->publishStateStore(store_state, TIMEOUT_MS_PUBLISH);
		moveColorSensor();
		dps.readColorValue();
		std::cout << "Detected Type: " << dps.getLastColor() << std::endl;
		store_state.wp.printDebug();
		proStorage.setTimestampNow(store_state.wp.tag_uid, INSPECTION_INDEX);
		if ((dps.getLastColor() != WP_TYPE_NONE))
		{
			store_state.wp.type = dps.getLastColor();
			store_state.wp.printDebug();
			FSM_TRANSITION( NFC_RAW, color=blue, label='color ok' );
		}
		else
		{
			FSM_TRANSITION( NFC_REJECTED, color=blue, label='color wrong' );
			break;
		}
#ifdef __DOCFSM__
		FSM_TRANSITION( COLOR_DETECTION, color=blue, label='wait' );
#endif
		break;
	}
	//-----------------------------------------------------------------
	case NFC_RAW:
	{
		printState(NFC_RAW);
		moveNFC();
		std::string uid = dps.nfcReadUID();
		if (uid.empty())
		{
			FSM_TRANSITION( WRONG_COLOR, color=red, label='nfc error' );
			break;
		}

		std::vector<int64_t> vts = proStorage.getTagUidVts(uid);
		uint8_t mask_ts = proStorage.getTagUidMaskTs(uid);
		
		std::string tag_uid = dps.nfcDeviceDeleteWriteRawRead(store_state.wp.type, vts, mask_ts);
		store_state.wp.state = WP_STATE_RAW;
		if (tag_uid.empty())
		{
			FSM_TRANSITION( FAULT, color=red, label='nfc error' );
			break;
		}
		
		FSM_TRANSITION( STORE_WP_VGR, color=blue, label='nfc write ok' );
		break;
	}
	//-----------------------------------------------------------------
	case NFC_REJECTED:
	{
		printState(NFC_REJECTED);
		moveRefYNFC();
		std::string uid = dps.nfcReadUID();
		if (!uid.empty())
		{
			proStorage.resetTagUidMaskTs(uid);
			std::vector<int64_t> vts = proStorage.getTagUidVts(uid);
			uint8_t mask_ts = proStorage.getTagUidMaskTs(uid);
			std::string tag_uid = dps.nfcDeviceWriteRejectedRead(ft::WP_TYPE_NONE, vts, mask_ts);
			if (tag_uid.empty())
			{
				FSM_TRANSITION( FAULT, color=red, label='nfc error' );
				break;
			}
		}

		FSM_TRANSITION( WRONG_COLOR, color=blue, label='wrong color');

		assert(mqttclient);
		store_state.state = BAD_WORKPIECE;
		mqttclient->publishStateStore(store_state, TIMEOUT_MS_PUBLISH);

		trashItem = true;
		break;
	}
	//-----------------------------------------------------------------
	case WRONG_COLOR:
	{
		printState(WRONG_COLOR);
		dps.setErrorDSI(true);
		sound.warn();
		if (!trashItem && workingMode == TxtVgrWorkingModes_t::ENDLESS) {
			FSM_TRANSITION( RECYCLE_TRASH, color=blue, label='recycle bad workpiece');
		} 
		else 
		{
			moveWrongRelease();
			trashItem = false;
			FSM_TRANSITION( IDLE, color=green, label='next' );
		}
		
		break;
	}
	case RECYCLE_TRASH:
	{
		printState(RECYCLE_TRASH);
		moveRef();
		
		FSM_TRANSITION( START_DELIVERY, color=blue, label='test' );
		break;
	}
	//-----------------------------------------------------------------
	case NFC_PRODUCED:
	{
		printState(NFC_PRODUCED);
		assert(mqttclient);
		order_state.state = WAITING_FOR_QA;
		mqttclient->publishStateOrder(order_state, TIMEOUT_MS_PUBLISH);
		proStorage.setTimestampNow(order_state.wp.tag_uid, SORTING_INDEX);
		grip();
		dps.setActiveDSO(true);
		moveRefYNFC();
		std::string uid = dps.nfcReadUID();
		if (uid.empty())
		{
			FSM_TRANSITION( FAULT, color=red, label='nfc error' );
			break;
		}
		order_state.wp.tag_uid = uid;
		order_state.wp.printDebug();
		proStorage.setTimestampNow(order_state.wp.tag_uid, SHIPPING_INDEX);
		std::vector<int64_t> vts = proStorage.getTagUidVts(order_state.wp.tag_uid);
		uint8_t mask_ts = proStorage.getTagUidMaskTs(order_state.wp.tag_uid);
		std::string tag_uid = dps.nfcDeviceWriteProducedRead(order_state.wp.type, vts,mask_ts);
		if (tag_uid.empty())
		{
			assert(mqttclient);
			order_state.state = QA_FAULT;
			mqttclient->publishStateOrder(order_state, TIMEOUT_MS_PUBLISH);
			FSM_TRANSITION( FAULT, color=red, label='nfc error');
			break;
		}

		FSM_TRANSITION( MOVE_PICKUP_WAIT, color=blue, label='nfc ok' );
		break;
	}
	//-----------------------------------------------------------------
	case MOVE_PICKUP_WAIT:
	{
		printState(MOVE_PICKUP_WAIT);
		if (dps.is_DOUT()) //dout empty
		{
			dps.setErrorDSO(false);

			assert(mqttclient);
			order_state.state = TRANSPORT_WP_TO_DSO;
			mqttclient->publishStateOrder(order_state, TIMEOUT_MS_PUBLISH);

			moveDeliveryOutAndRelease();
			std::this_thread::sleep_for(std::chrono::milliseconds(1000));

			FSM_TRANSITION( MOVE_PICKUP, color=blue, label='delivered' );
		}
		else //dout not empty
		{
			dps.setErrorDSO(true);
		}
#ifdef __DOCFSM__
		FSM_TRANSITION( MOVE_PICKUP_WAIT, color=blue, label='wait' );
#endif
		break;
	}
	//-----------------------------------------------------------------
	case MOVE_PICKUP:
	{
		printState(MOVE_PICKUP);
		dps.setActiveDSO(false);
		assert(mqttclient);
		order_state.state = SHIPPED;
		mqttclient->publishStateOrder(order_state, TIMEOUT_MS_PUBLISH);

		if (workingMode == TxtVgrWorkingModes_t::NORMAL)
		{
			FSM_TRANSITION( IDLE, color=blue, label='next' );
		}
		else if (workingMode == TxtVgrWorkingModes_t::STORE_AFTER_PRODUCE || workingMode == TxtVgrWorkingModes_t::ENDLESS)
		{
			FSM_TRANSITION( PICKUP2DELIVERY, color=blue, label='next' );
		}
		else
		{
			std::cout << "WorkingMode unknown. This is an internal error." << std::endl;
			spdlog::get("file_logger")->error("WorkingMode unknown.", 0);
			exit(1);
		}

		break;
	}
	//-----------------------------------------------------------------
	case STORE_WP_VGR:
	{
		printState(STORE_WP_VGR);

		assert(mqttclient);
		store_state.state = STORING;
		mqttclient->publishVGR_Do(VGR_HBW_FETCHCONTAINER, &store_state, TIMEOUT_MS_PUBLISH);
		mqttclient->publishStateStore(store_state, TIMEOUT_MS_PUBLISH);

		dps.setActiveDSI(false);
		if (store_state.wp.type == WP_TYPE_NONE)
		{
			moveWrongRelease();
			FSM_TRANSITION( FAULT, color=red, label='wrong color' );
			break;
		}
	
		store_state.wp.printDebug();
		store_state.wp.type = dps.getLastColor();
		
		moveToHBW();
		FSM_TRANSITION( STORE_WP, color=blue, label='transport to HBW' );
		break;
	}
	//-----------------------------------------------------------------
	case STORE_WP:
	{
		printState(STORE_WP);
		if (reqHBWFault)
		{
			assert(mqttclient);
			store_state.state = STORE_FAULT;
			mqttclient->publishStateStore(store_state, TIMEOUT_MS_PUBLISH);
			FSM_TRANSITION( FAULT, color=red, label='error' );
			reqHBWFault = false;
		}
		else if (reqHBWfetched)
		{
			release();
			store_state.wp = reqWP_HBW; //should be identical anyway

			store_state.wp.printDebug();
			proStorage.setTimestampNow(store_state.wp.tag_uid, WAREHOUSING_INDEX);

			assert(mqttclient);
			store_state.state = STORE_DONE;
			mqttclient->publishStateStore(store_state, TIMEOUT_MS_PUBLISH);
			mqttclient->publishVGR_Do(VGR_HBW_STORE_WP, &store_state, TIMEOUT_MS_PUBLISH);

			moveRef();

			if (workingMode == TxtVgrWorkingModes_t::ENDLESS && store_state.wp.state == WP_STATE_RAW)
			{
				FSM_TRANSITION( ORDER_WP, color=green, label='endless_mode: order wp' );
			}
			else 
			{
				FSM_TRANSITION( IDLE, color=green, label='fetched' );
			}
			
			reqHBWfetched = false;
		}
#ifdef __DOCFSM__
		FSM_TRANSITION( STORE_WP, color=blue, label='wait' );
#endif
		break;
	}

	case ORDER_WP:
	{
		printState(ORDER_WP);
		assert(mqttclient);
		mqttclient->publishVGR_Order(store_state.wp.type, TIMEOUT_MS_PUBLISH);
		FSM_TRANSITION( IDLE, color=green, label='ordered' );
		break;
	}
	//-----------------------------------------------------------------
	case CALIB_VGR:
	{
		printState(CALIB_VGR);
		setStatus(SM_CALIB);
		while(true)
		{
			if (joyData.aX2 > 500) {
				assert(mqttclient);
				mqttclient->publishVGR_Do(VGR_HBW_CALIB, 0, TIMEOUT_MS_PUBLISH);
				mqttclient->publishStateVGR(ft::LEDS_READY, "", TIMEOUT_MS_PUBLISH, 0, "");
				FSM_TRANSITION( CALIB_HBW, color=orange, label='init' );
				break;
			} else if (joyData.aX2 < -500) {
				calibPos = (TxtVgrCalibPos_t)-1;
				FSM_TRANSITION( CALIB_VGR_NAV, color=orange, label='init' );
				break;
			} else if (joyData.aY2 < -500) {
				FSM_TRANSITION( CALIB_DPS, color=orange, label='init' );
				break;
			} else if (joyData.aY2 > 500) {
				assert(mqttclient);
				mqttclient->publishVGR_Do(VGR_SLD_CALIB, 0, TIMEOUT_MS_PUBLISH);
				mqttclient->publishStateVGR(ft::LEDS_READY, "", TIMEOUT_MS_PUBLISH, 0, "");
				FSM_TRANSITION( CALIB_SLD, color=orange, label='init' );
				break;
			} else if (joyData.b1) {
				FSM_TRANSITION( IDLE, color=green, label='cancel' );
				break;
			} else if (joyData.b2) {
				FSM_TRANSITION( IDLE, color=green, label='cancel' );
				break;
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}
#ifdef __DOCFSM__
		FSM_TRANSITION( CALIB_VGR, color=orange, label='wait' );
#endif
		break;
	}
	//-----------------------------------------------------------------
	case CALIB_HBW:
	{
		printState(CALIB_HBW);
		if (reqHBWcalib_end)
		{
			FSM_TRANSITION( IDLE, color=green, label='HBW calibrated' );
			reqHBWcalib_end = true;
			break;
		}
#ifdef __DOCFSM__
		FSM_TRANSITION( CALIB_HBW, color=orange, label='wait' );
#endif
		break;
	}
	//-----------------------------------------------------------------
	case CALIB_SLD:
	{
		printState(CALIB_SLD);
		if (reqSLDcalib_end)
		{
			FSM_TRANSITION( IDLE, color=green, label='SLD calibrated' );
			reqSLDcalib_end = true;
			break;
		}
#ifdef __DOCFSM__
		FSM_TRANSITION( CALIB_SLD, color=orange, label='wait' );
#endif
		break;
	}
	//-----------------------------------------------------------------
	case CALIB_DPS:
	{
		printState(CALIB_DPS);
		setStatus(SM_CALIB);
		moveRef();
		moveColorSensor(true);
		calibColorValues[0] = -1;
		calibColorValues[1] = -1;
		calibColorValues[2] = -1;
		calibColor=ft::TxtWPType_t::WP_TYPE_WHITE;
		FSM_TRANSITION( CALIB_DPS_NEXT, color=orange, label='next' );
		break;
	}
	//-----------------------------------------------------------------
	case CALIB_DPS_NEXT:
	{
		printState(CALIB_DPS_NEXT);
		bool exit_next = false;
		while(!exit_next)
		{
			setStatus(SM_CALIB);
			if (joyData.b1) {
				sound.info1();
				switch(calibColor)
				{
				case ft::TxtWPType_t::WP_TYPE_WHITE:
					calibColorValues[0] = dps.readColorValue();
					SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "value white: {}",calibColorValues[0]);

					std::this_thread::sleep_for(std::chrono::milliseconds(2000));
					calibColor=ft::TxtWPType_t::WP_TYPE_RED;

					break;
				case ft::TxtWPType_t::WP_TYPE_RED:
					calibColorValues[1] = dps.readColorValue();
					SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "value red: {}",calibColorValues[1]);

					std::this_thread::sleep_for(std::chrono::milliseconds(2000));
					calibColor=ft::TxtWPType_t::WP_TYPE_BLUE;

					break;
				case ft::TxtWPType_t::WP_TYPE_BLUE:
					calibColorValues[2] = dps.readColorValue();
					SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "value blue: {}",calibColorValues[2]);

					SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "w:{} r:{} b:{}",calibColorValues[0],calibColorValues[1],calibColorValues[2]);
					if ((calibColorValues[0] > 0)&&
						(calibColorValues[1] > 0)&&
						(calibColorValues[2] > 0))
					{
						dps.calibData.color_th[0] = (calibColorValues[0] + calibColorValues[1]) / 2;
						dps.calibData.color_th[1] = (calibColorValues[1] + calibColorValues[2]) / 2;
						SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "th1:{} th2:{}",dps.calibData.color_th[0],dps.calibData.color_th[1]);

						//check
						if ((calibColorValues[0] < calibColorValues[1])&&
							(calibColorValues[1] < calibColorValues[2])&&
							(calibColorValues[0] < calibColorValues[2])&&
							(dps.calibData.color_th[0] >= 200)&&
							(dps.calibData.color_th[1] < 2000))
						{
							dps.calibData.save();
						} else {
							sound.error();
						}
					} else {
						sound.error();
					}
					FSM_TRANSITION( IDLE, color=green, label='DPS calibrated' );
					exit_next = true;

					break;
				default:
					break;
				}
			} else if (joyData.b2) {
				sound.warn();
				FSM_TRANSITION( IDLE, color=green, label='cancel' );
				exit_next = true;
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}
#ifdef __DOCFSM__
		FSM_TRANSITION( CALIB_DPS_NEXT, color=orange, label='next color' );
#endif
		break;
	}
	//-----------------------------------------------------------------
	case CALIB_VGR_NAV:
	{
		printState(CALIB_VGR_NAV);
		bool reqmove = true;
		while(true)
		{
			if (joyData.b1) {
				break;
			} else if (joyData.b2) {
				FSM_TRANSITION( IDLE, color=green, label='cancel' );
				break;
			} else if (joyData.aX2 > 500) {
				calibPos=(TxtVgrCalibPos_t)(calibPos-1);
				std::cout << ft::toString(calibPos) << std::endl;
				reqmove = true;
			} else if (joyData.aX2 < -500) {
				calibPos=(TxtVgrCalibPos_t)(calibPos+1);
				std::cout << ft::toString(calibPos) << std::endl;
				reqmove = true;
			}
			//check pos valid
			if (calibPos < 0)
			{
				calibPos = (TxtVgrCalibPos_t)(VGRCALIB_END-1);
			} else if (calibPos >= VGRCALIB_END)
			{
				calibPos = (TxtVgrCalibPos_t)0;
			}
			//move pos
			if (reqmove)
			{
				moveCalibPos();
				setStatus(SM_CALIB);
				FSM_TRANSITION( CALIB_VGR_MOVE, color=orange, label='calib pos' );
				reqmove = false;
				break;
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}
#ifdef __DOCFSM__
		FSM_TRANSITION( CALIB_VGR_NAV, color=orange, label='select pos' );
#endif
		break;
	}
	//-----------------------------------------------------------------
	case CALIB_VGR_MOVE:
	{
		printState(CALIB_VGR_MOVE);
		while(true)
		{
			if (joyData.b2) {
				break; //-> NAV
			} else if (joyData.b1) {
				EncPos3 p3 = getPos3();
				switch(calibPos)
				{
				case VGRCALIB_DSI:
					calibData.setPos3("DIN", p3);
					calibData.copyPos3X("DIN","DIN0");
					calibData.copyPos3Z("DIN","DIN0");
					calibData.save();
					break;
				case VGRCALIB_DCS:
					calibData.setPos3("DCS", p3);
					calibData.copyPos3X("DCS","DCS0");
					calibData.copyPos3Z("DCS","DCS0");
					calibData.save();
					break;
				case VGRCALIB_NFC:
					calibData.setPos3("DNFC", p3);
					calibData.copyPos3X("DNFC","DNFC0");
					calibData.copyPos3Z("DNFC","DNFC0");
					calibData.save();
					break;
				case VGRCALIB_WDC:
					calibData.setPos3("WDC", p3);
					calibData.copyPos3X("WDC","WDC0");
					calibData.save();
					break;
				case VGRCALIB_DSO:
					calibData.setPos3("DOUT", p3);
					calibData.copyPos3X("DOUT","DOUT0");
					calibData.save();
					break;
				case VGRCALIB_HBW:
					calibData.setPos3("HBW1", p3);
					calibData.copyPos3X("HBW1","HBW");
					calibData.copyPos3Z("HBW1","HBW");
					calibData.copyPos3X("HBW1","HBW0");
					calibData.save();
					break;
				case VGRCALIB_MPO:
					calibData.setPos3("MPO", p3);
					calibData.copyPos3X("MPO","MPO0");
					calibData.copyPos3Z("MPO","MPO0");
					calibData.save();
					break;
				case VGRCALIB_SL1:
					calibData.setPos3("SSD1", p3);
					calibData.copyPos3X("SSD1","SSD10");
					calibData.save();
					break;
				case VGRCALIB_SL2:
					calibData.setPos3("SSD2", p3);
					calibData.copyPos3X("SSD2","SSD20");
					calibData.save();
					break;
				case VGRCALIB_SL3:
					calibData.setPos3("SSD3", p3);
					calibData.copyPos3X("SSD3","SSD30");
					calibData.save();
					break;
				default:
					break;
				}
				//same pos again
				break; //-> NAV
			} else if ((joyData.aX2 > 500)||(joyData.aX2 < -500)) {
				break; //-> NAV
			}
			moveJoystick();
#ifdef __DOCFSM__
			FSM_TRANSITION( CALIB_VGR_MOVE, color=orange, label='move' );
#endif
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}
		FSM_TRANSITION( CALIB_VGR_NAV, color=orange, label='ok' );
		break;
	}
	case PICKUP2DELIVERY:
	{
		printState(PICKUP2DELIVERY);
		
		if (!dps.is_DOUT()) {
			moveRef();
			move("DOUT0");
			move("DOUT");
			move("DOUT1");
			grip();
			move("DOUT");
			move("DOUT0");

			storeProcessedWorkpiece = (workingMode == TxtVgrWorkingModes_t::STORE_AFTER_PRODUCE);

			FSM_TRANSITION( START_DELIVERY, color=blue, label='start delivery' );
		}
		
		break;
	}
	case HBW2PICKUP:
	{
		printState(HBW2PICKUP);
		

		if (dps.is_DOUT())
		{
			dps.setErrorDSO(false);

			assert(mqttclient);
			pickup_state.state = MOVE_WP_TO_DSO;
			mqttclient->publishStatePickup(pickup_state, TIMEOUT_MS_PUBLISH);

			moveDeliveryOutAndRelease();
			
			pickup_state.state = WORKPIECE_READY;
			mqttclient->publishStatePickup(pickup_state, TIMEOUT_MS_PUBLISH);
			
			moveRef();

			FSM_TRANSITION( IDLE, color=blue, label='hbw2pickup' );
		}
		else
		{
			dps.setErrorDSO(true); //not sure if needed
		}
		break;
	}
	case STORE_FROM_DSO:
		printState(STORE_FROM_DSO);
		if (!dps.is_DOUT())
		{
			dps.setErrorDSO(false);
			moveRef();
			move("DOUT0");
			move("DOUT");
			move("DOUT1");
			grip();
			moveRef();
			storeProcessedWorkpiece = true;
			FSM_TRANSITION (START_DELIVERY, color=blue, label='store');
			// moveNFC();
			// dps.nfcRead();
			// store_state.wp = new TxtWorkpiece(dps.getNfcData()->wp);
			// if (store_state.wp)
			// {
			// 	// if.wp.processed store, else delivery
			// 	storeWorkpiece = true;
			// 	FSM_TRANSITION( STORE_WP_VGR, color=blue, label='test' );
			// }
			// else
			// {
			// 	sound.error();
			// 	store_state.state = TxtStoreState_t::STORE_FAULT;
			// 	assert(mqttclient);
			// 	mqttclient->publishStateStore(store_state, TIMEOUT_MS_PUBLISH);
			// 	FSM_TRANSITION( IDLE, color=blue, label='test' );
			// 	delete store_state.wp;
			// }
		}
		else 
		{
			store_state.state = DSO_EMPTY;
			assert(mqttclient);
			mqttclient->publishStateStore(store_state, TIMEOUT_MS_PUBLISH);

			sound.warn();
			FSM_TRANSITION( IDLE, color=blue, label='idle' );
		}
		break;
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

void TxtVacuumGripperRobot::moveCalibPos()
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "moveCalibPos",0);
	switch(calibPos)
	{
	case VGRCALIB_DSI:
		moveRef();
		move("DIN");
		break;
	case VGRCALIB_DCS:
		moveRef();
		move("DCS");
		break;
	case VGRCALIB_NFC:
		moveRef();
		move("DNFC");
		break;
	case VGRCALIB_WDC:
		moveRef();
		move("WDC");
		break;
	case VGRCALIB_DSO:
		moveRef();
		move("DOUT");
		break;
	case VGRCALIB_HBW:
		moveRef();
		move("HBW0");
		move("HBW");
		move("HBW1");
		break;
	case VGRCALIB_MPO:
		moveRef();
		move("HBW");
		move("HBW0");
		move("MPO0");
		move("MPO");
		break;
	case VGRCALIB_SL1:
		moveYRef();
		moveRef();
		move("SSD10");
		move("SSD1");
		break;
	case VGRCALIB_SL2:
		moveYRef();
		moveRef();
		move("SSD20");
		move("SSD2");
		break;
	case VGRCALIB_SL3:
		moveYRef();
		moveRef();
		move("SSD30");
		move("SSD3");
		break;
	default:
		assert(0);
		break;
	}
}

void TxtVacuumGripperRobot::initDashboard()
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "initDashboard", 0);
	assert(mqttclient);
	mqttclient->publishStateHBW(ft::LEDS_OFF, "", TIMEOUT_MS_PUBLISH, 0, "");
	mqttclient->publishStateMPO(ft::LEDS_OFF, "", TIMEOUT_MS_PUBLISH, 0, "");
	mqttclient->publishStateSLD(ft::LEDS_OFF, "", TIMEOUT_MS_PUBLISH, 0, "");
	mqttclient->publishStateVGR(ft::LEDS_OFF, "", TIMEOUT_MS_PUBLISH, 0, "hbw");
	mqttclient->publishStateVGR(ft::LEDS_OFF, "", TIMEOUT_MS_PUBLISH, 0, "mpo");
	mqttclient->publishStateVGR(ft::LEDS_OFF, "", TIMEOUT_MS_PUBLISH, 0, "dso");
	mqttclient->publishStateDSI(ft::LEDS_OFF, "", TIMEOUT_MS_PUBLISH, 0, "");
	mqttclient->publishStateDSO(ft::LEDS_OFF, "", TIMEOUT_MS_PUBLISH, 0, "");
	order_state = TxtOrderState();
	order_state.state = WAITING_FOR_ORDER;
	pickup_state = TxtPickupState();
	pickup_state.state = WAITING_FOR_PICKUP;
	store_state = TxtStoreState();
	store_state.state = WAITING_FOR_STORE;
	mqttclient->publishStateOrder(order_state, TIMEOUT_MS_PUBLISH);
	mqttclient->publishStateStore(store_state, TIMEOUT_MS_PUBLISH);
	mqttclient->publishStatePickup(pickup_state, TIMEOUT_MS_PUBLISH);
	TxtWorkpiece wp_empty("",WP_TYPE_NONE, WP_STATE_RAW);
	HistoryCode_map_t map_hist;
	mqttclient->publishNfcDS(&wp_empty, map_hist, TIMEOUT_MS_PUBLISH);
}

void TxtVacuumGripperRobot::run()
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "run",0);

	TxtNfcDevice* nfc = dps.getNfc();
	assert(nfc);
	obs_vgr = new TxtVacuumGripperRobotObserver(this, mqttclient);
	obs_nfc = new TxtNfcDeviceObserver(nfc, mqttclient);
	obs_dps = new TxtDeliveryPickupStationObserver(&dps, mqttclient);

	//nfc
	std::cout << "open nfc device ... ";
	bool suc = nfc->open();
	std::cout << (suc?"OK":"FAILED") << std::endl;
	if (!suc) {
		std::cout << "exit NFC failed" << std::endl;
		spdlog::get("file_logger")->error("exit NFC failed",0);
		exit(1);
	}

	//dps update thread
	dps.startThread();

	initDashboard();

	//move seq if program started first time
	moveYRef();
	moveZRef();
	moveXRef();

	FSM_INIT_FSM( INIT, color=black, label='init' );
	while (!m_stoprequested)
	{
		fsmStep();
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}

	assert(mqttclient);
	mqttclient->publishVGR_Do(VGR_EXIT, 0, TIMEOUT_MS_PUBLISH);
	initDashboard();
}


} /* namespace ft */
