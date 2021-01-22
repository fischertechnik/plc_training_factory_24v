/*
 * Utils.cpp
 *
 *  Created on: 22.01.2018
 *      Author: steiger-a
 */

#include "Utils.h"

#include <algorithm>

#ifndef NO_MQTT
#include "opencv2/opencv.hpp"
#endif


namespace ft {


// returns difference in seconds from UTC at current time if not specified
long time_offset()
{
	time_t when = std::time(nullptr);
    auto const tm = *std::localtime(&when);
    std::ostringstream os;
    os << std::put_time(&tm, "%z");
    std::string s = os.str();
    // s is in ISO 8601 format: "±HHMM"
	SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "±HHMM {}", s);
    int h = std::stoi(s.substr(0,3), nullptr, 10);
    int m = std::stoi(s[0]+s.substr(3), nullptr, 10);
    return h * 3600 + m * 60;
}

bool trycheckTimestampTTL(const std::string& str, double diff_max)
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "checkTimestamp: {} offset secs:{}",str, std::chrono::seconds(time_offset()).count());
	auto startlocal = ft::trygettimepoint(str) + std::chrono::seconds(time_offset());
	auto nowlocal = std::chrono::system_clock::now();

	auto itt_start = std::chrono::system_clock::to_time_t(startlocal);
	std::ostringstream ss_start;
	ss_start << std::put_time(std::gmtime(&itt_start), "%FT%TZ");
	SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "ss_start.str():{}", ss_start.str());

	auto itt_now = std::chrono::system_clock::to_time_t(nowlocal);
	std::ostringstream ss_now;
	ss_now << std::put_time(std::gmtime(&itt_now), "%FT%TZ");
	SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "ss_now.str():{}", ss_now.str());

	auto dur = nowlocal-startlocal;
	auto diff_s = std::abs(std::chrono::duration_cast< std::chrono::duration<float> >(dur).count());

	SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "str:{} diff_s:{} diff_max:{}", str.c_str(), diff_s, diff_max);

	bool r = (diff_s < diff_max);
	if (!r)
	{
		spdlog::get("file_logger")->info("str:{} diff_s:{} diff_max:{}", str.c_str(), diff_s, diff_max);
		//spdlog::get("file_logger")->info("Wrong date and time! try to execute: sudo ntpdate -u pool.ntp.org");
		//int r = system("sudo ntpdate -u pool.ntp.org");
		spdlog::get("file_logger")->info("  RETURN {}", r);
	}
	return r;
}

void gettimestampstr(int64_t timestamp, char* sts) {
	//SPDLOG_LOGGER_TRACE(spdlog::get("console"), "gettimestampstr");
	time_t epch = timestamp/1000000000.;
	double temp_modf_integer_part;
	int ms = modf(timestamp/1000000000., &temp_modf_integer_part) * 1000.;

	struct tm * timeinfo;
	timeinfo = gmtime ( &epch ); //UTC
	sprintf(sts, "%04d-%02d-%02dT%02d:%02d:%02d.%03dZ",
			timeinfo->tm_year + 1900, timeinfo->tm_mon + 1, timeinfo->tm_mday,
			timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec, ms);
}

void gettimestr(time_t rawtime, int ms, char* sts) {
	//SPDLOG_LOGGER_TRACE(spdlog::get("console"), "gettimestr");
	struct tm * timeinfo;
	//time ( &rawtime );
	timeinfo = gmtime ( &rawtime ); //UTC
	sprintf(sts, "%04d-%02d-%02dT%02d:%02d:%02d.%03dZ",
			timeinfo->tm_year + 1900, timeinfo->tm_mon + 1, timeinfo->tm_mday,
			timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec, ms);
}

void getnowstr(char* sts) {
	//SPDLOG_LOGGER_TRACE(spdlog::get("console"), "getnowstr");
	struct timespec ts;
	timespec_get(&ts, TIME_UTC);
	struct timespec {
		time_t   tv_sec;        /* seconds */
		long     tv_nsec;       /* nanoseconds */
	};
	time_t epch = ts.tv_sec;
	gettimestr(epch, (int)(ts.tv_nsec/1000000), sts);
}

double getnowtimestamp_s() {
	//SPDLOG_LOGGER_TRACE(spdlog::get("console"), "getnowtimestamp");
	struct timespec ts;
	timespec_get(&ts, TIME_UTC);
	struct timespec {
		time_t   tv_sec;        /* seconds */
		long     tv_nsec;       /* nanoseconds */
	};
	time_t epch = ts.tv_sec;
	double ms = ts.tv_nsec/1000000000.;
	//SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "epch:{} ms:{}", epch, ms);
	return (double)epch+ms;
}

std::chrono::system_clock::time_point trygettimepoint(const std::string& utcstr)
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "gettimepoint {}",utcstr);
	std::string _str = utcstr;
	if (utcstr.back() != 'Z') {
		throw std::invalid_argument("utc string should contain 'Z' at the end!");
	}
	replace(_str.begin(), _str.end(), 'T', ' ');
	_str = _str.substr(0,_str.length()-1); // ...Z
	//_str = _str.substr(0,_str.find(".")); // .XXX
	SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "replace {}",_str);
    std::istringstream iss{_str};
    std::tm tm = {0};
    tm.tm_isdst = -1; //initialize!
    if (!(iss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S")))//"%F %T")))
    {
    	std::cout << "te1" << std::endl;
        throw std::invalid_argument("get_time str:"+_str);
    }
    time_t t = std::mktime(&tm); //FIXME statt 15Uhr, 14Uhr -1h Unterschied, std::mktime falsch
	char sts[25];
    gettimestr(t, 0, sts);
    SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "sts {}", sts);

    std::chrono::system_clock::time_point tp = std::chrono::system_clock::from_time_t(t);

	auto itt_tp = std::chrono::system_clock::to_time_t(tp);
	std::ostringstream ss_tp;
	ss_tp << std::put_time(std::gmtime(&itt_tp), "%FT%TZ");
	SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "ss_tp.str():{}", ss_tp.str());

    if (iss.eof())
    {
        return tp;
    }
    double zz;
    if (iss.peek() != '.' || !(iss >> zz))
    {
        throw std::invalid_argument("decimal str:"+_str);
    }
    using hr_clock = std::chrono::high_resolution_clock;
    std::size_t nanoseconds = zz * hr_clock::period::den / hr_clock::period::num;
    SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "frac seconds {}",(double)(nanoseconds)/1000000000.);
    return tp += hr_clock::duration(nanoseconds);
}

#ifndef NO_MQTT
std::string ftos(float f, int nd) {
	//SPDLOG_LOGGER_TRACE(spdlog::get("console"), "");
    std::ostringstream ostr;
    int tens = std::stoi("1" + std::string(nd, '0'));
    ostr << round(f*tens)/tens;
    return ostr.str();
}
#endif


} /* namespace ft */
