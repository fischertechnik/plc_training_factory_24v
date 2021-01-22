/*
 * Observer.cpp
 *
 *  Created on: 20.01.2018
 *      Author: steiger-a
 */

#include "Observer.h"

#include <chrono>
#include <ctime>

#include "spdlog/spdlog.h"


namespace ft {


void SubjectObserver::Attach (Observer* o)
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "",0);
	_observers.push_back(o);
}

void SubjectObserver::Detach (Observer* o)
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "",0);
	int count = _observers.size();
	int i;

	for (i = 0; i < count; i++) {
		if(_observers[i] == o)
			break;
	}
	if(i < count)
		_observers.erase(_observers.begin() + i);

}

void SubjectObserver::Notify ()
{
	//SPDLOG_LOGGER_TRACE(spdlog::get("console"), "Notify 1");
#ifdef DEBUG
	auto start = std::chrono::system_clock::now();
#endif
	int count = _observers.size();
	Observer* obs;
	for (int i = 0; i < count; i++) {
		obs = _observers[i];
		if(obs != nullptr) {
			obs->Update(this);
		}
	}
#ifdef DEBUG
	auto end = std::chrono::system_clock::now();
	auto dur = end-start;
	auto secs = std::chrono::duration_cast< std::chrono::duration<float> >(dur);
	//double diff_ms = secs.count()*1000.;
	//spdlog::get("console")->info("Notify diff_ms:{}",diff_ms);
#endif
	//SPDLOG_LOGGER_TRACE(spdlog::get("console"), "Notify 2");
}


} /* namespace ft */
