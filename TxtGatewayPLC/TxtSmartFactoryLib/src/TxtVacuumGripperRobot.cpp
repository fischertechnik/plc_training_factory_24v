/*
 * TxtVacuumGripperRobot.cpp
 *
 *  Created on: 08.11.2018
 *      Author: steiger-a
 *		Edited: Mark-Oliver Masur
 */

#include "TxtVacuumGripperRobot.h"

#include "Utils.h"


namespace ft {


TxtVacuumGripperRobot::TxtVacuumGripperRobot(TxtTransfer* pT, ft::TxtMqttFactoryClient* mqttclient)
	: TxtSimulationModel(pT, mqttclient),
	currentState(__NO_STATE), newState(__NO_STATE),
	calibPos(VGRCALIB_DSI), calibColor(ft::WP_TYPE_NONE),
	axisX("VGR_X", pT, 0, 0, 1500),
	axisY("VGR_Y", pT, 1, 1, 900),
	axisZ("VGR_Z", pT, 2, 2, 1100),
	vgripper(pT, 6, 7), target(""), dps(pT, mqttclient),
	reqQuit(false), 
	wp_waiting_for_pickup(false),
	reqOrder(false), reqWP_order(),
	reqNfcRead(false), reqNfcDelete(false),
	joyData(), reqJoyData(false),
	reqMPOstarted(false),
	reqHBWstored(false), reqHBWfetched(false),
	reqHBWcalib_nav(false), reqHBWcalib_end(false), reqSLDcalib_end(false),
	reqSLDsorted(false),
	obs_vgr(0), obs_nfc(0), temp(0.0f), humidity(0.0f), trashItem(false), 
	workingMode(TxtVgrWorkingModes_t::NORMAL), workingModeNew(TxtVgrWorkingModes_t::UNDEFINED),
	reqStoreError(false), reqPickupError(false)
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "TxtVacuumGripperRobot",0);
	if (!calibData.existCalibFilename()) calibData.saveDefault();
	calibData.load();
    configInputs();
}

TxtVacuumGripperRobot::~TxtVacuumGripperRobot()
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "~TxtVacuumGripperRobot",0);
	delete obs_vgr;
	delete obs_nfc;
}

void TxtVacuumGripperRobot::stop()
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "stop",0);
	axisX.stop();
	axisY.stop();
	axisZ.stop();
}

void TxtVacuumGripperRobot::moveRef()
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "moveRef",0);
	setActStatus(true, SM_BUSY);
	axisY.moveRef();
	std::thread tx = axisX.moveRefThread();
	std::thread tz = axisZ.moveRefThread();
	tz.join();
	tx.join();
	setActStatus(false, SM_READY);
}

void TxtVacuumGripperRobot::moveXRef()
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "moveXRef",0);
	axisX.moveRef();
}

void TxtVacuumGripperRobot::moveYRef()
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "moveYRef",0);
	axisY.moveRef();
}

void TxtVacuumGripperRobot::moveZRef()
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "moveZRef",0);
	axisZ.moveRef();
}

void TxtVacuumGripperRobot::moveXEnd()
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "moveXEnd",0);
	axisX.moveAbs(axisX.getPosEnd());
}

void TxtVacuumGripperRobot::moveYEnd()
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "moveYEnd",0);
	axisY.moveAbs(axisY.getPosEnd());
}

void TxtVacuumGripperRobot::moveZEnd()
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "moveZEnd",0);
	axisZ.moveAbs(axisZ.getPosEnd());
}

void TxtVacuumGripperRobot::move(const std::string pos3name, TxtVgrPosOrder_t order)
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "move pos:{} o:{}",pos3name,(int)order);
	setActStatus(true, SM_BUSY);
	std::map<std::string, EncPos3>::iterator it;
	it = calibData.map_pos3.find(pos3name);
	if (it != calibData.map_pos3.end())
	{
	    EncPos3 pos3 = calibData.map_pos3[pos3name];
	    move(pos3, order);
	    setActStatus(false, SM_READY);
	} else {
		setActStatus(false, SM_ERROR);
	}
}

void TxtVacuumGripperRobot::move(uint16_t x, uint16_t y, uint16_t z, TxtVgrPosOrder_t order)
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "move x:{} y:{} z:{} o:{}",x,y,z,(int)order);
	switch(order)
	{
	case VGRMOV_PTP:
		{
			std::thread tx = axisX.moveAbsThread(x);
			std::thread ty = axisY.moveAbsThread(y);
			std::thread tz = axisZ.moveAbsThread(z);
			tz.join();
			ty.join();
			tx.join();
		}
		break;
	case VGRMOV_XYZ:
		axisX.moveAbs(x);
		axisY.moveAbs(y);
		axisZ.moveAbs(z);
		break;
	case VGRMOV_XZY:
		axisX.moveAbs(x);
		axisZ.moveAbs(z);
		axisY.moveAbs(y);
		break;
	case VGRMOV_YXZ:
		axisY.moveAbs(y);
		axisX.moveAbs(x);
		axisZ.moveAbs(z);
		break;
	case VGRMOV_YZX:
		axisY.moveAbs(y);
		axisZ.moveAbs(z);
		axisX.moveAbs(x);
		break;
	case VGRMOV_ZXY:
		axisZ.moveAbs(z);
		axisX.moveAbs(x);
		axisY.moveAbs(y);
		break;
	case VGRMOV_ZYX:
		axisZ.moveAbs(z);
		axisY.moveAbs(y);
		axisX.moveAbs(x);
		break;
	case VGRMOV_X_PTP:
		{
			axisX.moveAbs(x);
			std::thread ty = axisY.moveAbsThread(y);
			std::thread tz = axisZ.moveAbsThread(z);
			tz.join();
			ty.join();
		}
		break;
	case VGRMOV_Y_PTP:
		{
			axisY.moveAbs(y);
			std::thread tx = axisX.moveAbsThread(x);
			std::thread tz = axisZ.moveAbsThread(z);
			tz.join();
			tx.join();
		}
		break;
	case VGRMOV_Z_PTP:
		{
			axisZ.moveAbs(z);
			std::thread ty = axisY.moveAbsThread(y);
			std::thread tx = axisX.moveAbsThread(x);
			tx.join();
			ty.join();
		}
		break;
	default:
		break;
	}
}

void TxtVacuumGripperRobot::moveJoystick()
{
	if (reqJoyData)
	{
		TxtJoysticksData jd = joyData;
		SPDLOG_LOGGER_TRACE(spdlog::get("console"), "moveJoystick 1:{} {} {} 2:{} {} {}", jd.aX1,jd.aY1,jd.b1,jd.aX2,jd.aY2,jd.b2);

		int abs_X1 = abs(jd.aX1);
		int abs_Y1 = abs(jd.aY1);
		int abs_X2 = abs(jd.aX2);
		int abs_Y2 = abs(jd.aY2);

		if (jd.b1 || jd.b2) {
			//do not move
		}
		else if (abs_X1>abs_Y1)
		{
			//uint16_t sX1 = jd.aX1>=0?jd.aX1:-jd.aX1;
			//axisX.setSpeed(sX1>512?512:sX1);
			int relMove = abs_X1/50;
			if (jd.aX1 > 0) {
				//axisX.setMotorRight();
				axisX.moveRel(relMove);
			} else if (jd.aX1 < 0) {
				//axisX.setMotorLeft();
				axisX.moveRel(-relMove);
			} else {
				//axisX.setMotorOff();
				//axisX.stop();
			}
		}
		else if (abs_Y1>abs_X1)
		{
			//uint16_t sY1 = jd.aY1>=0?jd.aY1:-jd.aY1;
			//axisZ.setSpeed(sY1>512?512:sY1);
			int relMove = abs_Y1/50;
			if (jd.aY1 > 0) {
				//axisZ.setMotorRight();
				axisZ.moveRel(relMove);
			} else if (jd.aY1 < 0) {
				//axisZ.setMotorLeft();
				axisZ.moveRel(-relMove);
			} else {
				//axisZ.setMotorOff();
				//axisZ.stop();
			}
		}
		else if (abs_Y2>abs_X2)
		{
			//uint16_t sY2 = jd.aY2>=0?jd.aY2:-jd.aY2;
			//axisY.setSpeed(sY2>512?512:sY2);
			int relMove = abs_Y2/50;
			if (jd.aY2 > 0) {
				//axisY.setMotorRight();
				axisY.moveRel(-relMove);
			} else if (jd.aY2 < 0) {
				//axisY.setMotorLeft();
				axisY.moveRel(relMove);
			} else {
				//axisY.setMotorOff();
				//axisY.stop();
			}
		}

		EncPos3 p3 = getPos3();
		std::cout << "EncPos3: " << p3.x << ", " << p3.y << ", " << p3.z << std::endl;

		reqJoyData = false;
	}
}

void TxtVacuumGripperRobot::moveDeliveryInAndGrip()
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "moveDeliveryInAndGrip", 0);
	move("DIN0", ft::VGRMOV_PTP);
	move("DIN", ft::VGRMOV_PTP);
	vgripper.grip();
	move("DIN0", ft::VGRMOV_PTP);
}

void TxtVacuumGripperRobot::moveDeliveryOutAndRelease()
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "moveDeliveryOutAndRelease", 0);
	axisY.moveRef();
	move("DOUT0", ft::VGRMOV_PTP);
	move("DOUT", ft::VGRMOV_PTP);
	vgripper.release();
	moveRef();
}

void TxtVacuumGripperRobot::moveColorSensor(bool half)
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "moveColorSensor", 0);
	move("DCS0", ft::VGRMOV_PTP);
	if (!half) {
		move("DCS", ft::VGRMOV_PTP);
	}
}

void TxtVacuumGripperRobot::moveRefYNFC()
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "moveRefNFC", 0);
	axisY.moveRef();
	move("DNFC0", ft::VGRMOV_PTP);
	move("DNFC", ft::VGRMOV_PTP);
}

void TxtVacuumGripperRobot::moveNFC()
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "moveNFC", 0);
	move("DNFC0", ft::VGRMOV_PTP);
	move("DNFC", ft::VGRMOV_PTP);
}

void TxtVacuumGripperRobot::moveWrongRelease()
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "moveWrongRelease", 0);
	move("WDC0", ft::VGRMOV_PTP);
	move("WDC", ft::VGRMOV_PTP);
	vgripper.release();
	moveRef();
}


void TxtVacuumGripperRobot::moveToHBW()
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "moveToHBW",0);
	move("HBW0", ft::VGRMOV_PTP);
	move("HBW", ft::VGRMOV_PTP);
}

void TxtVacuumGripperRobot::moveFromHBW1()
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "moveFromHBW1",0);
	move("HBW0", ft::VGRMOV_PTP);
	move("HBW", ft::VGRMOV_PTP);
}

void TxtVacuumGripperRobot::moveFromHBW2()
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "moveFromHBW2",0);
	move("HBW1", ft::VGRMOV_PTP);
	grip();
	move("HBW", ft::VGRMOV_PTP);
	move("HBW0", ft::VGRMOV_PTP);
}

void TxtVacuumGripperRobot::moveMPO()
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "moveMPO",0);
	move("MPO0", ft::VGRMOV_PTP);
	move("MPO", ft::VGRMOV_PTP);
}

void TxtVacuumGripperRobot::releaseMPO() {
	vgripper.release();
	std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	axisY.moveRef();
	axisZ.moveRef();
}

void TxtVacuumGripperRobot::moveSSD1()
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "moveSSD1",0);
	move("SSD10", ft::VGRMOV_PTP);
	move("SSD1", ft::VGRMOV_PTP);
}

void TxtVacuumGripperRobot::moveSSD2()
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "moveSSD2",0);
	move("SSD20", ft::VGRMOV_PTP);
	move("SSD2", ft::VGRMOV_PTP);
}

void TxtVacuumGripperRobot::moveSSD3()
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "moveSSD3",0);
	move("SSD30", ft::VGRMOV_PTP);
	move("SSD3", ft::VGRMOV_PTP);
}

void TxtVacuumGripperRobot::setSpeed(int16_t s)
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "setSpeed {}", s);
	axisX.setSpeed(s);
	axisY.setSpeed(s);
	axisZ.setSpeed(s);
}

void TxtVacuumGripperRobot::configInputs()
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "configInputs", 0);
	assert(pT->pTArea);
	//SSD_1
	pT->pTArea->ftX1config.uni[3].mode = MODE_R; // Digital Switch with PullUp resistor
	pT->pTArea->ftX1config.uni[3].digital = 1;
	//SSD_2
	pT->pTArea->ftX1config.uni[4].mode = MODE_R; // Digital Switch with PullUp resistor
	pT->pTArea->ftX1config.uni[4].digital = 1;
	//SSD_3
	pT->pTArea->ftX1config.uni[5].mode = MODE_R; // Digital Switch with PullUp resistor
	pT->pTArea->ftX1config.uni[5].digital = 1;

	//save
	pT->pTArea->ftX1state.config_id ++; // Save the new Setup
}


} /* namespace ft */
