/*
 * TxtSound.cpp
 *
 *  Created on: 10.10.2019
 *      Author: steiger-a
 */

#include "TxtSound.h"

#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"


static UINT16  u16SoundCmdId = 0;


namespace ft {


void TxtSound::play(FISH_X1_TRANSFER* pTArea, unsigned int r,  unsigned int num)
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "play num:{} repeat:{}",num,r);
	assert(pTArea);
	if (num < 0 || num > 29) {
		std::cout << "number " << num <<  " not valid!." << std::endl;
		return;
	}
	for (unsigned int i = 0; i < r; i++)
	{
		pTArea->sTxtOutputs.u16SoundIndex = num;
		pTArea->sTxtOutputs.u16SoundRepeat = 0;
		pTArea->sTxtOutputs.u16SoundCmdId++;
		while(pTArea->sTxtInputs.u16SoundCmdId == u16SoundCmdId)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}
}

TxtSound::TxtSound(TxtTransfer* pT) : pT(pT), mute(false)
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "TxtSound",0);
	assert(pT);
	assert(pT->pTArea);
	pT->pTArea->sTxtOutputs.u16SoundCmdId = u16SoundCmdId;
}

TxtSound::~TxtSound()
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "~TxtSound",0);
}

void TxtSound::play(unsigned int num, unsigned int r)
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "play num:{} repeat:{}",num,r);
	if (!mute)
	{
		assert(pT);
		assert(pT->pTArea);
		if (num < 0 || num > 29) {
			std::cout << "number " << num <<  " not valid!." << std::endl;
			return;
		}
		for (unsigned int i = 0; i < r; i++)
		{
			pT->pTArea->sTxtOutputs.u16SoundIndex = num;
			pT->pTArea->sTxtOutputs.u16SoundRepeat = 0;
			pT->pTArea->sTxtOutputs.u16SoundCmdId++;
			while(pT->pTArea->sTxtInputs.u16SoundCmdId == u16SoundCmdId)
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(10));
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}
	} else {
		std::cout << "sound is disabled." << std::endl;
	}
}


} /* namespace ft */
