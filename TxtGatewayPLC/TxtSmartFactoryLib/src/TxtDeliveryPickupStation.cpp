/*
 * TxtDeliveryPickupStation.cpp
 *
 *  Created on: 08.11.2018
 *      Author: steiger-a
 */

#include "TxtDeliveryPickupStation.h"

#include "Utils.h"


namespace ft {


// Callback Function.
bool reqUpdateDIN = true;
bool reqUpdateDOUT = true;

uint16_t u16LastStateDIN = 0;
uint16_t u16LastStateDOUT = 0;

// This is called between receiving inputs and sending outputs to the TXT hardware
bool DPSTransferAreaCallbackFunction(FISH_X1_TRANSFER *pTArea, int i32NrAreas)
{	// 10 ms cycle, debouncing inputs and count

	if (pTArea->IFStatus.ComErr != 0)
	{
		printf("pTArea->IFStatus.ComErr %d %d", pTArea->IFStatus.ComErr, pTArea->IFStatus.iostatus);
	}

	//DIN
	if (pTArea->ftX1in.uni[6] != u16LastStateDIN)
	{   // new State
		//SPDLOG_LOGGER_TRACE(spdlog::get("console"), "DPSTransferAreaCallbackFunction: new State DIN", 0);
		u16LastStateDIN = pTArea->ftX1in.uni[6];
		reqUpdateDIN = true;
	}
	//DOUT
	if (pTArea->ftX1in.cnt_in[3] != u16LastStateDOUT)
	{   // new State
		//SPDLOG_LOGGER_TRACE(spdlog::get("console"), "DPSTransferAreaCallbackFunction: new State DOUT", 0);
		u16LastStateDOUT = pTArea->ftX1in.cnt_in[3];
		reqUpdateDOUT = true;
	}
	return true; // if you return FALSE, then the hardware update is stopped !!!
}

TxtDeliveryPickupStation::TxtDeliveryPickupStation(TxtTransfer* pT, ft::TxtMqttFactoryClient* mqttclient)
	: TxtSimulationModel(pT, mqttclient),
	lastColorValue(0), activeDSI(false), activeDSO(false), errorDSI(false), errorDSO(false)
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "TxtDeliveryPickupStation",0);
	if (!calibData.existCalibFilename()) calibData.saveDefault();
	calibData.load();
    //sound.enable(calibData.sound_enable); //see VGR
    configInputs();
    SetTransferAreaCompleteCallback(DPSTransferAreaCallbackFunction);
}

TxtDeliveryPickupStation::~TxtDeliveryPickupStation()
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "~TxtDeliveryPickupStation",0);
}

std::string TxtDeliveryPickupStation::nfcDeviceDeleteWriteRawRead(ft::TxtWPType_t c, std::vector<int64_t> vts, uint8_t mask_ts)
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "moveNFCDeviceDeleteWriteRawRead", 0);
	nfcDelete();
	TxtWorkpiece wp("", c, WP_STATE_RAW);
	nfcWrite(wp, vts, mask_ts);
	std::string sid = nfcRead();
	return sid;
}

std::string TxtDeliveryPickupStation::nfcDeviceWriteProducedRead(ft::TxtWPType_t c, std::vector<int64_t> vts, uint8_t mask_ts)
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "moveNFCDeviceWriteProducedRead", 0);
	TxtWorkpiece wp("", c, WP_STATE_PROCESSED);
	nfcWrite(wp, vts, mask_ts);
	std::string sid = nfcRead();
	return sid;
}

std::string TxtDeliveryPickupStation::nfcDeviceWriteRejectedRead(ft::TxtWPType_t c, std::vector<int64_t> vts, uint8_t mask_ts)
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "moveNFCDeviceWriteRejectedRead", 0);
	nfcDelete();
	TxtWorkpiece wp("", c, WP_STATE_REJECTED);
	nfcWrite(wp, vts, mask_ts);
	std::string sid = nfcRead();
	return sid;
}

void TxtDeliveryPickupStation::configInputs()
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "configInputs", 0);
	assert(pT->pTArea);
	//DIN
	pT->pTArea->ftX1config.uni[6].mode = MODE_R; // Digital Switch with PullUp resistor
	pT->pTArea->ftX1config.uni[6].digital = 1;
	//trigger first read (workaround first value is wrong)
    u16LastStateDIN = pT->pTArea->ftX1in.uni[6];

	//DOUT
	pT->pTArea->ftX1config.cnt[3].mode = MODE_R; // C4 = Digital Switch with PullUp resistor
	//trigger first read (workaround first value is wrong)
    u16LastStateDOUT = pT->pTArea->ftX1in.cnt_in[3];

    //ColorSensor
	pT->pTArea->ftX1config.uni[7].mode = MODE_U; // Analog Voltage
	pT->pTArea->ftX1config.uni[7].digital = 0;
	//save
	pT->pTArea->ftX1state.config_id ++; // Save the new Setup
}

bool TxtDeliveryPickupStation::is_DIN()
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "is_DIN", 0);
	assert(pT->pTArea);
	return (pT->pTArea->ftX1in.uni[6] == 1);
}

bool TxtDeliveryPickupStation::is_DOUT()
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "is_DOUT", 0);
	assert(pT->pTArea);
	return (pT->pTArea->ftX1in.cnt_in[3] == 1);
}

int TxtDeliveryPickupStation::readColorValue()
{
	lastColorValue = 3000;
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "getColorValue", 0);
	assert(pT->pTArea);
	for (int i = 0; i < 100; i++) {
		if(lastColorValue > pT->pTArea->ftX1in.uni[7]) {
			lastColorValue = pT->pTArea->ftX1in.uni[7];
		}
	}
	return lastColorValue;
}

ft::TxtWPType_t TxtDeliveryPickupStation::getLastColor()
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "getColor", 0);
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

bool TxtDeliveryPickupStation::nfcDelete()
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "nfcDelete", 0);
	bool suc = nfc.eraseTags();
	nfc.Notify();
	//if (suc) sound.info1();
	if (!suc) return false;
	std::string tag_uid = nfc.readTags();
	return !tag_uid.empty();
}

std::string TxtDeliveryPickupStation::nfcRead()
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "nfcRead", 0);
	std::string sret = nfc.readTags();
	nfc.Notify();
	if (!sret.empty()) sound.info1();
	return sret;
}

std::string TxtDeliveryPickupStation::nfcReadUID()
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "nfcReadUID", 0);
	std::string sret = nfc.readTagsGetUID();
	nfc.Notify();
	//if (!sret.empty()) sound.info1();
	return sret;
}

//TODO change vts to HistoryCode_map_t
bool TxtDeliveryPickupStation::nfcWrite(TxtWorkpiece wp, std::vector<int64_t> vts, uint8_t mask_ts)
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "nfcWrite {} {} {}", wp.state, wp.type, vts.size());
	std::vector<uTS> vuTS;
	HistoryCode_map_t map_ts;
	for(unsigned int i = 0; i < vts.size(); i++)
	{
		uTS uts;
		uts.s64 = vts[i];
		vuTS.push_back(uts);
		//TODO code
		TxtHistoryCode_t code = (TxtHistoryCode_t)toCode((TxtHistoryIndex_t)i);
		map_ts[code] = uts.s64;
	}

	bool bret = nfc.writeTags(wp, map_ts, mask_ts);
	nfc.Notify();
	if (bret) sound.info1();
	return bret;
}

void TxtDeliveryPickupStation::run()
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "run",0);
	while (!m_stoprequested)
	{
		if (reqUpdateDIN)
		{
			Notify();
			reqUpdateDIN = false;
		}
		if (reqUpdateDOUT)
		{
			Notify();
			reqUpdateDOUT = false;
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	}
}


} /* namespace ft */
