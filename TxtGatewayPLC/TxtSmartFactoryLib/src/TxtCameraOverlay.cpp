/*
 * TxtCameraOverlay.cpp
 *
 *  Created on: 15.01.2020
 *      Author: steiger-a
 */

#include "TxtCameraOverlay.h"

#include "base64.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <thread>	// For sleep
#include <chrono>


namespace ft {


TxtCameraOverlay::TxtCameraOverlay(double w, double h) :
	w(w), h(h)
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "TxtCameraOverlay w:{} h:{}", w, h);
}

TxtCameraOverlay::~TxtCameraOverlay() {
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "~TxtCameraOverlay");
}

void TxtCameraOverlay::draw(cv::Mat& im)
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "draw");
	if (!textTL.empty())
	{
		int fontface = cv::FONT_HERSHEY_DUPLEX;
		double scale = 0.4;
		int thickness = 1;
		int baseline = 0;
		cv::Point p0(10,20);
		cv::Size text = cv::getTextSize(textTL, fontface, scale, thickness, &baseline);
		cv::rectangle(im, p0 + cv::Point(0, baseline), p0 + cv::Point(text.width, -text.height), cv::Scalar(0,0,0,128), cv::FILLED);
		cv::putText(im, textTL, p0, fontface, scale, cv::Scalar(255,255,255,255), thickness, cv::LINE_AA);
	}
	//TODO
}


} /* namespace ft */
