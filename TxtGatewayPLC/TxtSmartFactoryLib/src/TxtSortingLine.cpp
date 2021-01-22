/*
 * TxtSortingLine.cpp
 *
 *  Created on: 03.04.2019
 *      Author: steiger-a
 */

#include "TxtSortingLine.h"

#include "TxtMqttFactoryClient.h"
#include "Utils.h"

#include <string>
#include <fstream>
#include <json/writer.h>
#include <json/reader.h>


#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"


namespace ft {


// Callback Function.
uint16_t u16LastState;
uint16_t u16Counter;
// This is called between receiving inputs and sending outputs to the TXT hardware
bool SLDTransferAreaCallbackFunction(FISH_X1_TRANSFER *pTArea, int i32NrAreas)
{	// 10 ms cycle, debouncing inputs and count
	if (pTArea->IFStatus.ComErr != 0)
	{
		printf("pT->pTArea->IFStatus.ComErr %d %d", pTArea->IFStatus.ComErr, pTArea->IFStatus.iostatus);
	}
	if (pTArea->ftX1in.cnt_in[0] != u16LastState)
	{   // new State
		//SPDLOG_LOGGER_TRACE(spdlog::get("console"), "SLDTransferAreaCallbackFunction: new State", 0);
		u16Counter++;
		SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "u16Counter {}",u16Counter);
		u16LastState = pTArea->ftX1in.cnt_in[0];
	}
	return true; // if you return FALSE, then the hardware update is stopped !!!
}


TxtSortingLine::TxtSortingLine(TxtTransfer* pT, ft::TxtMqttFactoryClient* mqttclient)
	: TxtSimulationModel(pT, mqttclient),
	currentState(__NO_STATE), newState(__NO_STATE),
	convBelt(pT, 0), chEW(4), chER(5), chEB(6), chComp(7),
	lastColorValue(-1), calibColor(ft::WP_TYPE_NONE), reqQuit(false), reqMPOproduced(false), reqVGRstart(false), reqVGRcalib(false),
	joyData(), reqJoyData(false),
	obs_sld(0)
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "TxtSortingLine",0);
	if (!calibData.existCalibFilename()) calibData.saveDefault();
	calibData.load();
    configInputs();
    SetTransferAreaCompleteCallback(SLDTransferAreaCallbackFunction);
}

TxtSortingLine::~TxtSortingLine()
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "~TxtSortingLine",0);
	delete obs_sld;
}

bool TxtSortingLine::isColorSensorTriggered()
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "isColorSensorTriggered", 0);
	assert(pT->pTArea);
	return (pT->pTArea->ftX1in.uni[0] != 1);
}

bool TxtSortingLine::isEjectionTriggered()
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "isEjectionTriggered", 0);
	assert(pT->pTArea);
	return (pT->pTArea->ftX1in.uni[2] != 1);
}

bool TxtSortingLine::isWhite()
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "isWhite", 0);
	assert(pT->pTArea);
	return (pT->pTArea->ftX1in.uni[5] != 1);
}

bool TxtSortingLine::isRed()
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "isRed", 0);
	assert(pT->pTArea);
	return (pT->pTArea->ftX1in.uni[6] != 1);
}

bool TxtSortingLine::isBlue()
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "isBlue", 0);
	assert(pT->pTArea);
	return (pT->pTArea->ftX1in.uni[7] != 1);
}

int TxtSortingLine::readColorValue()
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "readColorValue", 0);
	assert(pT->pTArea);
	lastColorValue = pT->pTArea->ftX1in.uni[1];
	return lastColorValue;
}

ft::TxtWPType_t TxtSortingLine::getLastColor()
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "getLastColor", 0);
	if ((lastColorValue >= 200)&&(lastColorValue < calibData.color_th[0]))
	{
		return WP_TYPE_WHITE;
	}
	else if ((lastColorValue >= calibData.color_th[0])&&(lastColorValue < calibData.color_th[1]))
	{
		return WP_TYPE_RED;
	}
	else if ((lastColorValue >= calibData.color_th[1])&&(lastColorValue < 2000))
	{
		return WP_TYPE_BLUE;
	}
	return WP_TYPE_NONE;
}

ft::TxtWPType_t TxtSortingLine::getDetectedColor()
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "getDetectedColor", 0);
	if ((detectedColorValue >= 200)&&(detectedColorValue < calibData.color_th[0]))
	{
		return WP_TYPE_WHITE;
	}
	else if ((detectedColorValue >= calibData.color_th[0])&&(detectedColorValue < calibData.color_th[1]))
	{
		return WP_TYPE_RED;
	}
	else if ((detectedColorValue >= calibData.color_th[1])&&(detectedColorValue < 2000))
	{
		return WP_TYPE_BLUE;
	}
	return WP_TYPE_NONE;
}

void TxtSortingLine::ejectWhite()
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "ejectWhite", 0);
	assert(pT->pTArea);
	pT->pTArea->ftX1out.duty[chEW] = 512;
	std::this_thread::sleep_for(std::chrono::milliseconds(500));
	pT->pTArea->ftX1out.duty[chEW] = 0;
	setCompressor(false);
}

void TxtSortingLine::ejectRed()
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "ejectRed", 0);
	assert(pT->pTArea);
	pT->pTArea->ftX1out.duty[chER] = 512;
	std::this_thread::sleep_for(std::chrono::milliseconds(500));
	pT->pTArea->ftX1out.duty[chER] = 0;
	setCompressor(false);
}

void TxtSortingLine::ejectBlue()
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "ejectBlue", 0);
	assert(pT->pTArea);
	pT->pTArea->ftX1out.duty[chEB] = 512;
	std::this_thread::sleep_for(std::chrono::milliseconds(500));
	pT->pTArea->ftX1out.duty[chEB] = 0;
	setCompressor(false);
}

void TxtSortingLine::setCompressor(bool on)
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "setCompressor {}", on);
	assert(pT->pTArea);
	pT->pTArea->ftX1out.duty[chComp] = on ? 512 : 0; // Switch on with PWM Value 512 (= max speed)
}

void TxtSortingLine::configInputs()
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "configInputs", 0);
	assert(pT->pTArea);
	//isStartColorSensor
	pT->pTArea->ftX1config.uni[0].mode = MODE_R; // Digital Switch with PullUp resistor
	pT->pTArea->ftX1config.uni[0].digital = 1;
	//isStartEjection
	pT->pTArea->ftX1config.uni[2].mode = MODE_R; // Digital Switch with PullUp resistor
	pT->pTArea->ftX1config.uni[2].digital = 1;
	//isWhite
	pT->pTArea->ftX1config.uni[5].mode = MODE_R; // Digital Switch with PullUp resistor
	pT->pTArea->ftX1config.uni[5].digital = 1;
	//isRed
	pT->pTArea->ftX1config.uni[6].mode = MODE_R; // Digital Switch with PullUp resistor
	pT->pTArea->ftX1config.uni[6].digital = 1;
	//isBlue
	pT->pTArea->ftX1config.uni[7].mode = MODE_R; // Digital Switch with PullUp resistor
	pT->pTArea->ftX1config.uni[7].digital = 1;
	//ColorSensor
	pT->pTArea->ftX1config.uni[1].mode = MODE_U; // Analog Voltage
	pT->pTArea->ftX1config.uni[1].digital = 0;

	pT->pTArea->ftX1config.cnt[0].mode = MODE_R; // C1 = Digital Switch with PullUp resistor

	//save
	pT->pTArea->ftX1state.config_id ++; // Save the new Setup
}


} /* namespace ft */
