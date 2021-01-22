/*
 * TxtAxis1RefSwitch.h
 *
 *  Created on: 08.11.2018
 *      Author: steiger-a
 */

#ifndef TXTAXIS1REFSWITCH_H_
#define TXTAXIS1REFSWITCH_H_


#include <stdio.h>          // for printf()
#include <unistd.h>         // for sleep()
#include <iostream>
#include <thread>

#include "KeLibTxtDl.h"     // TXT Lib
#include "FtShmem.h"        // TXT Transfer Area

#include "TxtAxis.h"


namespace ft {


class TxtVacuumGripperRobot;
class TxtHighBayWarehouse;
class TxtAxis1RefSwitch : public TxtAxis, public SubjectObserver {
	friend class TxtVacuumGripperRobot;
	friend class TxtHighBayWarehouse;
public:
	TxtAxis1RefSwitch(std::string name, TxtTransfer* pT, uint8_t chM, uint8_t chS1, uint16_t posEnd);
	virtual ~TxtAxis1RefSwitch();

	void moveRef();
	std::thread moveRefThread() {
		return std::thread(&TxtAxis1RefSwitch::moveRef, this);
	}

	bool moveAbs(uint16_t p);
	std::thread moveAbsThread(uint16_t p) {
		return std::thread(&TxtAxis1RefSwitch::moveAbs, this, p);
	}

	bool moveRel(int rp) {
		if (status == AXIS_READY)
		{
			if (((rp>0)&&((int)pos+rp<65535)) || //negative
				((rp<0)&&(pos>=-rp))) //positive
			{
				return moveAbs(pos+rp);
			}
		}
		return false;
	}

	uint16_t getPosAbs() { return pos; }
	uint16_t getPosEnd() { return posEnd; }

	virtual void setMotorRight(); //override and check posEnd

protected:
	void reset();
	void resetCounter();

	void moveLeft(uint16_t steps, uint16_t* pPos);
	void moveRight(uint16_t steps, uint16_t* pPos);

	uint16_t pos;
	uint16_t posEnd;
};


} /* namespace ft */


#endif /* TXTAXIS1REFSWITCH_H_ */
