/*
 * TxtSound.h
 *
 *  Created on: 10.10.2019
 *      Author: steiger-a
 */

#ifndef TXTSOUND_H_
#define TXTSOUND_H_

#include "KeLibTxtDl.h"     // TXT Lib
#include "FtShmem.h"        // TXT Transfer Area

#include "TxtAxis.h"		// TxtTransfer


namespace ft {


class TxtSound {
public:
	static void play(FISH_X1_TRANSFER* pTArea, unsigned int r=1, unsigned int num=6);

	TxtSound(TxtTransfer* pT);
	virtual ~TxtSound();

	void play(unsigned int num, unsigned int r=1);
	void enable(bool e) { mute = !e; }

	void info1(unsigned int r=1) { play(6, r); }
	void info2() { play(26); }
	void warn() { play(3); }
	void error() { play(2); }

private:
	TxtTransfer* pT;
	bool mute;
};


} /* namespace ft */


#endif /* TXTSOUND_H_ */
