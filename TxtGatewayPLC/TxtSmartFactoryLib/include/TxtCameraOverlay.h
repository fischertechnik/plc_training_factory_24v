/*
 * TxtCameraOverlay.h
 *
 *  Created on: 15.01.2020
 *      Author: steiger-a
 */

#ifndef TxtCameraOverlay_H_
#define TxtCameraOverlay_H_

#include <chrono>
#include <thread>

#include "opencv2/opencv.hpp"

#include "spdlog/spdlog.h"


namespace ft {


class TxtCameraOverlay {
public:
	TxtCameraOverlay(double w=320, double h=240);
	virtual ~TxtCameraOverlay();

	void setTextTL(std::string t) { textTL = t; }
	void setTextTR(std::string t) { textTR = t; }
	void setTextBL(std::string t) { textBL = t; }
	void setTextBR(std::string t) { textBR = t; }

	void draw(cv::Mat& im);

private:
	double w;
	double h;
	std::string textTL;
	std::string textTR;
	std::string textBL;
	std::string textBR;
};


} /* namespace ft */


#endif /* TxtCameraOverlay_H_ */
