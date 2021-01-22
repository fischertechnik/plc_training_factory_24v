/*
 * TxtMotionDetection.cpp
 *
 *  Created on: 15.02.2018
 *      Author: steiger-a
 */

#include "TxtMotionDetection.h"


namespace ft {


TxtMotionDetection::TxtMotionDetection(ft::TxtCamera* cam, double max_limit_Area)
	: cam(cam), tsLastDetected(), max_limit_Area(max_limit_Area),
	  m_stoprequested(false), m_running(false), m_mutex(), m_thread()
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "max_limit_Area:{}", max_limit_Area);
	pthread_mutexattr_t attr;
	pthread_mutexattr_init(&attr);
	pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
	pthread_mutex_init(&m_mutex, &attr);
}

TxtMotionDetection::~TxtMotionDetection()
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "");
	if (m_running) {
		stopThread();
	}
	pthread_mutex_destroy(&m_mutex);
}

bool TxtMotionDetection::init() {
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "");
	return true;
}

bool TxtMotionDetection::startThread() {
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "");
	if (m_running) return true; //already running
	/*if (!init()) {
    	return false;
	}*/
    //go
    assert(m_running == false);
    m_running = true;
    /*pthread_attr_t tattr;
    int ret;
    sched_param param;
    int newprio = 20;
    ret = pthread_attr_init(&tattr);
    ret = pthread_attr_getschedparam (&tattr, &param);
    LOG("old priority %d", param.sched_priority);
    param.sched_priority = newprio;
    ret = pthread_attr_setschedparam (&tattr, &param);
    LOG("new priority %d",newprio);*/
    //return pthread_create(&m_thread, &tattr, start_thread, this) == 0;
    return pthread_create(&m_thread, 0, start_thread, this) == 0;
}

bool TxtMotionDetection::stopThread() {
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "");
	if (!m_running) return true; //already stopped
	//stop
    assert(m_running == true);
    m_running = false;
    m_stoprequested = true;
    return pthread_join(m_thread, 0) == 0;
}

std::string TxtMotionDetection::getDataString() {
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "");
	assert(cam);
	return cam->getDataString();
}

void TxtMotionDetection::run() {
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "");
    while (!m_stoprequested)
    {
		//pthread_mutex_lock(&m_mutex);

		cv::Mat frame;
		assert(cam);
		cam->getFrame().copyTo(frame);

		if (!frame.empty()) {

			//convert to grayscale
			cv::Mat gray;
			cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);
			cv::GaussianBlur(gray, gray, cv::Size(21, 21), 0);
			SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "TxtMotionDetection cvtColor");

			if (!gray_last.empty()) {
				//compute difference between first frame and current frame
				cv::Mat frameDelta;
				cv::absdiff(gray_last, gray, frameDelta);
				cv::Mat thresh;
				cv::threshold(frameDelta, thresh, 25, 255, cv::THRESH_BINARY);

				cv::dilate(thresh, thresh, cv::Mat(), cv::Point(-1,-1), 2);
				std::vector<std::vector<cv::Point> > cnts;
				cv::findContours(thresh, cnts, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

				SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "TxtMotionDetection findContours {}", cnts.size());
				for(unsigned int i = 0; i< cnts.size(); i++) {
					std::vector<cv::Point> cnt = cnts[i];
					double cArea = contourArea(cnts[i]);
#ifdef DEBUG
					cv::Rect rectcnt = cv::boundingRect(cnt);
					// tl() directly equals to the desired min. values
					//cv::Point minVal = rectcnt.tl();
					// br() is exclusive so we need to subtract 1 to get the max. values
					//cv::Point maxVal = rectcnt.br() - cv::Point(1, 1);
					//SPDLOG_LOGGER_DEBUG(spdlog::get("console"), ".......... cnt size:{} min:{} {} max:{} {}", cnt.size(), minVal.x, minVal.y, maxVal.x, maxVal.y);
					SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "---------- cnt.size:{} cArea:{}({})", cnt.size(), cArea, max_limit_Area);
#endif
					if(cArea < max_limit_Area) { //default 500
						continue;
					}
					//cv::putText(frame, "Motion Detected", cv::Point(10, 20), cv::FONT_HERSHEY_SIMPLEX, 0.75, cv::Scalar(0,0,255),2);

					auto tsDetected = std::chrono::system_clock::now();
					auto dur = tsDetected-tsLastDetected;
					auto secs = std::chrono::duration_cast< std::chrono::duration<float> >(dur);
					SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "========== elapsed_seconds:{} ({})", secs.count(), TIMEWAIT_S_MAX);
					if (secs.count() > TIMEWAIT_S_MAX) {
						SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "XXXXXXXXXX TRIGGER");
						tsLastDetected = tsDetected;
						Notify(); //motion detectionl
					}
				}
			}

			gray_last = gray;
		}
		std::this_thread::sleep_for(std::chrono::milliseconds((int64_t)cam->getPeriod()));

		//pthread_mutex_unlock(&m_mutex);
	}
}


} /* namespace ft */
