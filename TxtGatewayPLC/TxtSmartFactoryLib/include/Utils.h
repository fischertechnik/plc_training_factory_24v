/*
 * Utils.h
 *
 *  Created on: 22.01.2018
 *      Author: steiger-a
 */

#ifndef UTILS_H_
#define UTILS_H_

#include <iostream>
#include <sstream>
#include <string>
#include <time.h>
#include <iomanip>

#include "spdlog/spdlog.h"


namespace ft {


long time_offset();
bool trycheckTimestampTTL(const std::string& str, double diff_max = 10.0);

void gettimestampstr(int64_t timestamp, char* sts);
void gettimestr(time_t rawtime, int ms, char* sts);
void getnowstr(char* sts);
double getnowtimestamp_s();

std::chrono::system_clock::time_point trygettimepoint(const std::string& utcstr);

#ifndef NO_MQTT
std::string ftos(float f, int nd);
#endif


} /* namespace ft */


#endif /* UTILS_H_ */
