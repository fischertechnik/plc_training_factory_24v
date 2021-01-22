/*
 * TxtSimulationModel.cpp
 *
 *  Created on: 08.11.2018
 *      Author: steiger-a
 */

#include "TxtSimulationModel.h"

#include "TxtMqttFactoryClient.h"

#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"


namespace ft {


TxtSimulationModel::TxtSimulationModel(TxtTransfer* pT, ft::TxtMqttFactoryClient* mqttclient)
	: pT(pT), mqttclient(mqttclient), sound(pT), status(SM_NONE), active(false),
	  m_stoprequested(false), m_running(false), m_mutex(), m_thread()
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "TxtSimulationModel",0);
	pthread_mutexattr_t attr;
	pthread_mutexattr_init(&attr);
	pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
	pthread_mutex_init(&m_mutex, &attr);
}

TxtSimulationModel::~TxtSimulationModel()
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "~TxtSimulationModel",0);
	if (m_running) stopThread();
	pthread_mutex_destroy(&m_mutex);
}

bool TxtSimulationModel::startThread() {
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "start",0);
	//go
	assert(m_running == false);
	m_running = true;
	return pthread_create(&m_thread, 0, start_thread, this) == 0;
}

bool TxtSimulationModel::stopThread() {
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "stop",0);
	//stop
	assert(m_running == true);
	m_running = false;
	m_stoprequested = true;
	return pthread_join(m_thread, 0) == 0;
}


} /* namespace ft */
