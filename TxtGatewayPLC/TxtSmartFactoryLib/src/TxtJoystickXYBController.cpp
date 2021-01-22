/*
 * TxtJoystickXYBController.cpp
 *
 *  Created on: 08.11.2018
 *      Author: steiger-a
 */

#include "TxtJoystickXYBController.h"


namespace ft {


TxtJoystickXYBController::TxtJoystickXYBController(TxtTransfer* pT, uint8_t chX1, uint8_t chY1, uint8_t chB1, uint8_t chX2, uint8_t chY2, uint8_t chB2)
	: SubjectObserver(), pT(pT),
	chX1(chX1), chY1(chY1), chB1(chB1), chX2(chX2), chY2(chY2), chB2(chB2),
	offx1(center0), offy1(center0), offx2(center0), offy2(center0),
	jd(), m_stoprequested(false), m_running(false), m_mutex(), m_thread()
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "TxtJoystickXYBController chX1,chY1,B1,chX2,chY2,B2:{} {} {} {} {} {}", chX1, chY1, chB1, chX2, chY2, chB2);
	pthread_mutexattr_t attr;
	pthread_mutexattr_init(&attr);
	pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
	pthread_mutex_init(&m_mutex, &attr);
}

TxtJoystickXYBController::~TxtJoystickXYBController()
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console"),"~TxtJoystickXYBController",0);
	if (m_running) stopThread();
	pthread_mutex_destroy(&m_mutex);
}

bool TxtJoystickXYBController::startThread() {
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "start",0);
	//go
	assert(m_running == false);
	m_running = true;
	return pthread_create(&m_thread, 0, start_thread, this) == 0;
}

bool TxtJoystickXYBController::stopThread() {
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "stop",0);
	//stop
	assert(m_running == true);
	m_running = false;
	m_stoprequested = true;
	return pthread_join(m_thread, 0) == 0;
}

void TxtJoystickXYBController::configInputs()
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "configInputs", 0);
	assert(pT->pTArea);
	pT->pTArea->ftX1config.uni[chX1].mode = MODE_U;
	pT->pTArea->ftX1config.uni[chX1].digital = 0;
	pT->pTArea->ftX1config.uni[chY1].mode = MODE_U;
	pT->pTArea->ftX1config.uni[chY1].digital = 0;
	if (chB1 > 7)
	{
		pT->pTArea->ftX1config.cnt[chB1-8].mode = MODE_R; // CX
	} else {
		pT->pTArea->ftX1config.uni[chB1].mode = MODE_R;   // IX
		pT->pTArea->ftX1config.uni[chB1].digital = 1;
	}
	pT->pTArea->ftX1config.uni[chX2].mode = MODE_U;
	pT->pTArea->ftX1config.uni[chX2].digital = 0;
	pT->pTArea->ftX1config.uni[chY2].mode = MODE_U;
	pT->pTArea->ftX1config.uni[chY2].digital = 0;
	if (chB2 > 7)
	{
		pT->pTArea->ftX1config.cnt[chB2-8].mode = MODE_R; // CX
	} else {
		pT->pTArea->ftX1config.uni[chB2].mode = MODE_R;   // IX
		pT->pTArea->ftX1config.uni[chB2].digital = 1;
	}
	pT->pTArea->ftX1state.config_id ++; // Save the new Setup
}

//TODO
/*
double curve_func(double x) {
	if (x < -1.000) return 0.;
	else if (x <= -0.600) {return -0.500 + x*(0.5000);}
	else if (x <= -0.500) {return  1.000 + x*(3.0000);}
	else if (x <= -0.100) {return  0.119 + x*(1.2375);}
	else if (x <=  0.100) {return  0.000 + x*(0.0500);}
	else if (x <=  0.500) {return -0.119 + x*(1.2375);}
	else if (x <=  0.600) {return -1.000 + x*(3.0000);}
	else if (x <=  1.000) {return  0.500 + x*(0.5000);}
	else return 0.;
}
*/

void TxtJoystickXYBController::run() {
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "run",0);
	configInputs();

	const int min_u = center0-delta0;
	const int max_u = center0+delta0;
	//calib zero pos, wait
	for(unsigned int i=0; i<5; i++)
	{
		uint16_t rx10 = pT->pTArea->ftX1in.uni[chX1];
		uint16_t ry10 = pT->pTArea->ftX1in.uni[chY1];
		if ((rx10>=min_u)&&(rx10<=max_u)) {
			offx1 = rx10;
		}
		if ((ry10>=min_u)&&(ry10<=max_u)) {
			offy1 = ry10;
		}
		uint16_t rx20 = pT->pTArea->ftX1in.uni[chX2];
		uint16_t ry20 = pT->pTArea->ftX1in.uni[chY2];
		if ((rx20>=min_u)&&(rx20<=max_u)) {
			offx2 = rx20;
		}
		if ((ry20>=min_u)&&(ry20<=max_u)) {
			offy2 = ry20;
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
		SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "off x1 y1 x2 y2: {} {} {} {}", offx1, offy1, offx2, offy2);
	}

	TxtJoysticksData jdLast;
	while (!m_stoprequested)
	{
		uint16_t rx1 = pT->pTArea->ftX1in.uni[chX1];
		uint16_t ry1 = pT->pTArea->ftX1in.uni[chY1];
		//jd.aX1 = curve_func((double)(rx1*512/offx1-512)/512)*512;
		//jd.aY1 = curve_func((double)(ry1*512/offy1-512)/512)*512;
		jd.aX1 = rx1*512/offx1-512;
		jd.aY1 = ry1*512/offy1-512;
		jd.b1 = (chB1>7 ? pT->pTArea->ftX1in.cnt_in[chB1-8] : pT->pTArea->ftX1in.uni[chB1]) == 1;
		uint16_t rx2 = pT->pTArea->ftX1in.uni[chX2];
		uint16_t ry2 = pT->pTArea->ftX1in.uni[chY2];
		//jd.aX2 = curve_func((double)(rx2*512/offx2-512)/512)*512;
		//jd.aY2 = curve_func((double)(ry2*512/offy2-512)/512)*512;
		jd.aX2 = rx2*512/offx2-512;
		jd.aY2 = ry2*512/offy2-512;
		jd.b2 = (chB2>7 ? pT->pTArea->ftX1in.cnt_in[chB2-8] : pT->pTArea->ftX1in.uni[chB2]) == 1;

		if ((jd.aX1>jdLast.aX1?(jd.aX1-jdLast.aX1>JOY_MIN_TH):(jdLast.aX1-jd.aX1>JOY_MIN_TH))||
			(jd.aY1>jdLast.aY1?(jd.aY1-jdLast.aY1>JOY_MIN_TH):(jdLast.aY1-jd.aY1>JOY_MIN_TH))||
			(jdLast.b1!=jd.b1)||
			(jd.aX2>jdLast.aX2?(jd.aX2-jdLast.aX2>JOY_MIN_TH):(jdLast.aX2-jd.aX2>JOY_MIN_TH))||
			(jd.aY2>jdLast.aY2?(jd.aY2-jdLast.aY2>JOY_MIN_TH):(jdLast.aY2-jd.aY2>JOY_MIN_TH))||
			(jdLast.b2!=jd.b2))
		{
			/*SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "1:{}[{}] {}[{}] {} 2:{}[{}] {}[{}] {}",
					jd.aX1,rx1,jd.aY1,ry1,jd.b1,jd.aX2,rx2,jd.aY2,ry2,jd.b2);*/
			std::cout<<jd.aX1<<"["<<rx1<<"],"<<jd.aY1<<"["<<ry1<<"],"<< jd.b1 << " " <<
					jd.aX2<<"["<<rx2<<"],"<<jd.aY2<<"["<<ry2<<"],"<< jd.b2 << std::endl;
			Notify();
		}

		jdLast = jd;
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}
}


} /* namespace ft */
