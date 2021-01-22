/*
 * TxtHighBayWarehouse.cpp
 *
 *  Created on: 07.02.2019
 *      Author: steiger-a
 *		Edited: Mark-Oliver Masur
 */

#include "TxtHighBayWarehouse.h"

#include "TxtMqttFactoryClient.h"
#include "Utils.h"


namespace ft {


TxtHighBayWarehouse::TxtHighBayWarehouse(TxtTransfer* pT, ft::TxtMqttFactoryClient* mqttclient)
	: TxtSimulationModel(pT, mqttclient),
	currentState(__NO_STATE), newState(__NO_STATE),
	calibPos(HBWCALIB_CV),
	axisX("HBW_X", pT, 1, 4, 2050),
	axisY("HBW_Y", pT, 3, 7, 1050),
	axisZ("HBW_Z", pT, 2, 5, 6),
	convBelt(pT, 0, 0, 3),
	reqQuit(false),
	reqVGRfetchContainer(false), reqVGRstore(false),
	reqVGRfetch(false), reqVGRstoreContainer(false), reqVGRcalib(false), reqVGRresetStorage(false),
	joyData(), reqJoyData(false),
	obs_hbw(0), obs_storage(0)
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "TxtHighBayWarehouse",0);
	if (!calibData.existCalibFilename()) calibData.saveDefault();
	calibData.load();
}

TxtHighBayWarehouse::~TxtHighBayWarehouse()
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "~TxtHighBayWarehouse",0);
	delete obs_hbw;
	delete obs_storage;
}

void TxtHighBayWarehouse::stop()
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "stop",0);
	axisX.stop();
	axisY.stop();
	axisZ.stop();
}

void TxtHighBayWarehouse::moveRef()
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "moveRef",0);
	setActStatus(true, SM_BUSY);
	axisZ.moveS1();
	std::thread tx = axisX.moveRefThread();
	std::thread ty = axisY.moveRefThread();
	tx.join();
	ty.join();
	setActStatus(false, SM_READY);
}

void TxtHighBayWarehouse::moveJoystick()
{
	if (reqJoyData)
	{
		TxtJoysticksData jd = joyData;
		SPDLOG_LOGGER_TRACE(spdlog::get("console"), "moveJoystick 1:{} {} {} 2:{} {} {}", jd.aX1,jd.aY1,jd.b1,jd.aX2,jd.aY2,jd.b2);

		int abs_X1 = abs(jd.aX1);
		int abs_Y1 = abs(jd.aY1);

		if (jd.b1 || jd.b2) {
			//do not move
		}
		else if (abs_X1>abs_Y1)
		{
			//uint16_t sX1 = jd.aX1>=0?jd.aX1:-jd.aX1;
			//axisY.setSpeed(sX1>512?512:sX1);
			int relMove = abs_X1/50;
			if (jd.aX1 > 0) {
				//axisY.setMotorRight();
				axisY.moveRel(relMove);
			} else if (jd.aX1 < 0) {
				//axisY.setMotorLeft();
				axisY.moveRel(-relMove);
			} else {
				//axisY.stop();
				//axisY.setMotorOff();l
			}
		}
		else if (abs_Y1>abs_X1)
		{
			//uint16_t sY1 = jd.aY1>=0?jd.aY1:-jd.aY1;
			//axisX.setSpeed(sY1>512?512:sY1);
			int relMove = abs_Y1/50;
			if (jd.aY1 > 0) {
				//axisX.setMotorRight();
				axisX.moveRel(relMove);
			} else if (jd.aY1 < 0) {
				//axisX.setMotorLeft();
				axisX.moveRel(-relMove);
			} else {
				//axisX.setMotorOff();
				//axisX.stop();
			}
		}

		EncPos2 p2 = getPos2();
		std::cout << "EncPos2: " << p2.x << ", " << p2.y << std::endl;

		reqJoyData = false;
	}
}

EncPos2 TxtHighBayWarehouse::moveConv(bool stop)
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "moveConv",0);
	if (!stop)
	{
		axisZ.moveS1();
	}
	EncPos2 pos2 = calibData.conv;
	SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "pos:{} {}", pos2.x, pos2.y);
	std::thread tx = axisX.moveAbsThread(pos2.x);
	std::thread ty = axisY.moveAbsThread(pos2.y);
	tx.join();
	ty.join();
	return pos2;
}

EncPos2 TxtHighBayWarehouse::moveCR(int i, int j)
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "moveCR idx:{} {}", i, j);
	axisZ.moveS1();
	EncPos2 pos2;
	pos2.x = calibData.hbx[i];
	pos2.y = calibData.hby[j];
	SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "pos:{} {}", pos2.x, pos2.y);
	std::thread tx = axisX.moveAbsThread(pos2.x);
	std::thread ty = axisY.moveAbsThread(pos2.y);
	tx.join();
	ty.join();
	return pos2;
}

bool TxtHighBayWarehouse::getCR(int iCol, int iRow)
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "getCR idx:{} {}", iCol, iRow);
	EncPos2 p2 = moveCR(iCol,iRow);
	axisZ.moveS2();
	bool r = axisY.moveAbs(p2.y - ydelta);
	axisZ.moveS1();
	return r;
}

bool TxtHighBayWarehouse::putCR(int iCol, int iRow)
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "putCR idx:{} {}", iCol, iRow);
	EncPos2 p2 = moveCR(iCol,iRow);
	//uint16_t posEndY = axisY.getPosEnd();
	axisY.moveAbs(p2.y - ydelta);
	axisZ.moveS2();
	bool r = axisY.moveAbs(p2.y + ydelta);
	axisZ.moveS1();
	return r;
}

bool TxtHighBayWarehouse::getConv(bool stop)
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "getConv",0);
	EncPos2 p2 = moveConv(stop);
	axisZ.moveS2();
	convBelt.moveIn();
	bool r = axisY.moveAbs(p2.y - ydelta);
	if (!stop)
	{
		axisZ.moveS1();
	}
	return r;
}

bool TxtHighBayWarehouse::putConv(bool stop)
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "putConv",0);
	EncPos2 p2 = moveConv();
	axisZ.moveS2();
	bool r = axisY.moveAbs(p2.y + ydelta);
	convBelt.moveOut();
	if (!stop)
	{
		axisZ.moveS1();
	}
	return r;
}

bool TxtHighBayWarehouse::store(TxtWorkpiece wp)
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "store {}", wp.type);
	setActStatus(true, SM_BUSY);
	if (storage.store(wp))
	{
		StoragePos2 p = storage.getNextStorePos();
		if (p.x<0 || p.y<0) return false;
		bool r = getConv(true);
		if (!r) return false;
		r = putCR(p.x,p.y);
		if (!r) return false;
		setActStatus(false, SM_READY);
		storage.saveStorageState();
		moveRef();
		return true;
	}
	setActStatus(false, SM_ERROR);
	return false;
}

bool TxtHighBayWarehouse::storeContainer()
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "storeContainer",0);
	setActStatus(true, SM_BUSY);
	if (storage.storeContainer())
	{
		StoragePos2 p = storage.getNextStorePos();
		if (p.x<0 || p.y<0) return false;
		bool r = getConv(true);
		if (!r) return false;
		r = putCR(p.x,p.y);
		if (!r) return false;
		setActStatus(false, SM_READY);
		storage.saveStorageState();
		moveRef();
		return true;
	}
	setActStatus(false, SM_ERROR);
	return false;
}

HbwFetchResult TxtHighBayWarehouse::fetchByType(TxtWorkpiece txt_wp)
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "fetch wp by type {}", txt_wp.type);
	setActStatus(true, SM_BUSY);
	HbwFetchResult res = storage.fetchByType(txt_wp);
	if (res.result)
	{
		StoragePos2 p = storage.getNextFetchPos();
		if (p.x<0 || p.y<0) return HbwFetchResult(false);
		bool r = getCR(p.x, p.y);
		if (!r) return HbwFetchResult(false);
		r = putConv(true);
		if (!r) return HbwFetchResult(false);
		setActStatus(false, SM_READY);
		storage.saveStorageState();
		return res;
	}
	else {
		setActStatus(false, SM_ERROR);
		return res;
	}
}

HbwFetchResult TxtHighBayWarehouse::fetch(TxtWorkpiece wp)
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "fetch wp with tag_uid{}", wp.tag_uid);
	setActStatus(true, SM_BUSY);
	HbwFetchResult res = storage.fetch(wp);
	if (res.result)
	{
		StoragePos2 p = storage.getNextFetchPos();
		if (p.x<0 || p.y<0) return HbwFetchResult(false);
		bool r = getCR(p.x, p.y);
		if (!r) return HbwFetchResult(false);
		r = putConv(true);
		if (!r) return HbwFetchResult(false);
		setActStatus(false, SM_READY);
		storage.saveStorageState();
		return res;
	}
	else {
		setActStatus(false, SM_ERROR);
		return res;
	}
}

bool TxtHighBayWarehouse::fetchContainer()
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "fetchContainer", 0);
	setActStatus(true, SM_BUSY);
	if (storage.fetchContainer())
	{
		StoragePos2 p = storage.getNextFetchPos();
		if (p.x<0 || p.y<0) {
			setActStatus(false, SM_ERROR);
			return false;
		}
		bool r = getCR(p.x, p.y);
		if (!r) {
			setActStatus(false, SM_ERROR);
			return false;
		}
		r = putConv(true);
		if (!r) {
			setActStatus(false, SM_ERROR);
			return false;
		}
		setActStatus(false, SM_READY);
		return true;
	}
	setActStatus(false, SM_ERROR);
	return false;
}

bool TxtHighBayWarehouse::canColorBeStored(TxtWPType_t c)
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "canColorBeStored {}", c);
	return storage.canColorBeStored(c);
}

void TxtHighBayWarehouse::setSpeed(int16_t s)
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "setSpeed {}", s);
	axisX.setSpeed(s);
	axisY.setSpeed(s);
	axisZ.setSpeed(s);
}


} /* namespace ft */
