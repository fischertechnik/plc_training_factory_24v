/*
 * TxtMultiProcessingStation.cpp
 *
 *  Created on: 03.04.2019
 *      Author: steiger-a
 */

#include "TxtMultiProcessingStation.h"

#include "TxtMqttFactoryClient.h"

#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"


namespace ft {


TxtMultiProcessingStation::TxtMultiProcessingStation(TxtTransfer* pT, ft::TxtMqttFactoryClient* mqttclient)
	: TxtSimulationModel(pT, mqttclient),
	  currentState(__NO_STATE), newState(__NO_STATE),
	  chMsaw(1),
	  vgripper(pT,7,8+4),
	  axisGripper("gripper",pT,8+1,4,8+2),
	  axisOvenInOut("ovenInOut",pT,8+0,8+1,8+0),
	  axisRotTable("rotTable",pT,0,0,1,2),
	  convBelt(pT,2),
	  reqQuit(false), reqVGRproduce(false), reqSLDstarted(false)
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "TxtMultiProcessingStation",0);
	if (!calibData.existCalibFilename()) calibData.saveDefault();
	calibData.load();
    configInputs();
}

TxtMultiProcessingStation::~TxtMultiProcessingStation()
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "~TxtMultiProcessingStation",0);
}

bool TxtMultiProcessingStation::isEndConveyorBeltTriggered()
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "isEndConveyorBeltTriggered", 0);
	assert(pT->pTArea);
	bool ret = (pT->pTArea->ftX1in.uni[3] != 1);
	return ret;
}

void TxtMultiProcessingStation::setSawOff() {
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "setSawLeft",0);
	assert(pT->pTArea);
	pT->pTArea->ftX1out.duty[chMsaw*2] = 0;
	pT->pTArea->ftX1out.duty[chMsaw*2+1] = 0;
}

void TxtMultiProcessingStation::setSawLeft() {
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "setSawLeft",0);
	assert(pT->pTArea);
	pT->pTArea->ftX1out.duty[chMsaw*2] = 512;
	pT->pTArea->ftX1out.duty[chMsaw*2+1] = 0;
}

void TxtMultiProcessingStation::setSawRight()
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "setSawRight",0);
	assert(pT->pTArea);
	pT->pTArea->ftX1out.duty[chMsaw*2] = 0;
	pT->pTArea->ftX1out.duty[chMsaw*2+1] = 512;
}

void TxtMultiProcessingStation::setValveEjection(bool on)
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "setValveEjection {}", on);
	assert(pT->pTArea);
	pT->pTArea->ftX1out.duty[6] = on ? 512 : 0; // Switch on with PWM Value 512 (= max speed)
}

void TxtMultiProcessingStation::setCompressor(bool on)
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "setCompressor {}", on);
	assert(pT->pTArea);
	pT->pTArea->ftX1out.duty[7] = on ? 512 : 0; // Switch on with PWM Value 512 (= max speed)
}

bool TxtMultiProcessingStation::isOvenTriggered()
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "isOvenTriggered", 0);
	assert(pT->pTArea+1);
	bool ret = ((pT->pTArea+1)->ftX1in.uni[4] != 1);
	return ret;
}

void TxtMultiProcessingStation::setValveVacuum(bool on)
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "setValveVacuum {}", on);
	assert(pT->pTArea+1);
	(pT->pTArea+1)->ftX1out.duty[4] = on ? 512 : 0; // Switch on with PWM Value 512 (= max speed)
}

void TxtMultiProcessingStation::setValveLowering(bool on)
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "setValveLowering {}", on);
	assert(pT->pTArea+1);
	(pT->pTArea+1)->ftX1out.duty[5] = on ? 512 : 0; // Switch on with PWM Value 512 (= max speed)
}

void TxtMultiProcessingStation::setValveOvenDoor(bool on)
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "setValveOvenDoor {}", on);
	assert(pT->pTArea+1);
	(pT->pTArea+1)->ftX1out.duty[6] = on ? 512 : 0; // Switch on with PWM Value 512 (= max speed)
}

void TxtMultiProcessingStation::setLightOven(bool on)
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "setLightOven {}", on);
	assert(pT->pTArea+1);
	(pT->pTArea+1)->ftX1out.duty[7] = on ? 512 : 0;
}

void TxtMultiProcessingStation::configInputs()
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "configInputs", 0);
	//master
	assert(pT->pTArea);
	//End Conveyor Belt
	pT->pTArea->ftX1config.uni[3].mode = MODE_R; // Digital Switch with PullUp resistor
	pT->pTArea->ftX1config.uni[3].digital = 1;
	//save
	pT->pTArea->ftX1state.config_id ++; // Save the new Setup

	//extension
	assert(pT->pTArea+1);
	//Oven Phototransistor
	(pT->pTArea+1)->ftX1config.uni[4].mode = MODE_R; // Digital Switch with PullUp resistor
	(pT->pTArea+1)->ftX1config.uni[4].digital = 1;
	//save
	(pT->pTArea+1)->ftX1state.config_id ++; // Save the new Setup
}


} /* namespace ft */
