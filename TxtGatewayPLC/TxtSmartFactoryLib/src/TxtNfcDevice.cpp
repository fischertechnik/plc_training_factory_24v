/*
 * TxtNfcDevice.cpp
 *
 *  Created on: 09.02.2019
 *      Author: steiger-a
 */

#include "TxtNfcDevice.h"

#include "Utils.h"

#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"

#include <iostream>


namespace ft {


TxtNfcDevice::TxtNfcDevice()
	: lastCmd(NFC_NONE),
	  opened(false), pnd(NULL), context(NULL), nfcData(NULL)
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "TxtNfcDevice",0);
}

TxtNfcDevice::~TxtNfcDevice()
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "~TxtNfcDevice",0);
	close();
}

bool TxtNfcDevice::open()
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "open",0);
	bool verbose = true;
	int res = 0;
	int mask = 0x1ff;

	lastCmd = NFC_NONE;

	const char *acLibnfcVersion;
	acLibnfcVersion = nfc_version();
	printf("libnfc %s\n", acLibnfcVersion);

	nfc_context *context;
	nfc_init(&context);
	if (context == NULL) {
		std::cout << "Unable to init libnfc (malloc)" << std::endl;
		return false;
	}

	nfc_connstring connstrings[MAX_DEVICE_COUNT];
	size_t szDeviceFound = nfc_list_devices(context, connstrings, MAX_DEVICE_COUNT);

	if (szDeviceFound == 0) {
		std::cout << "No NFC device found." << std::endl;
	}

	for (unsigned int i = 0; i < szDeviceFound; i++) {
		nfc_target ant[MAX_TARGET_COUNT];
		pnd = nfc_open(context, connstrings[i]);

		if (pnd == NULL) {
			std::cout << "Unable to open NFC device" << connstrings[i] << std::endl;
			continue;
		}
		if (nfc_initiator_init(pnd) < 0) {
			nfc_perror(pnd, "nfc_initiator_init");
			nfc_exit(context);
			return false;
		}

		std::cout << "NFC device: " << nfc_device_get_name(pnd) << " opened" << std::endl;

		nfc_modulation nm;

		if (mask & 0x1) {
			nm.nmt = NMT_ISO14443A;
			nm.nbr = NBR_106;
			// List ISO14443A targets
			if ((res = nfc_initiator_list_passive_targets(pnd, nm, ant, MAX_TARGET_COUNT)) >= 0) {
				int n;
				if (verbose || (res > 0)) {
					printf("%d ISO14443A passive target(s) found%s\n", res, (res == 0) ? ".\n" : ":");
				}
				for (n = 0; n < res; n++) {
					print_nfc_target(&ant[n], verbose);
					printf("\n");
				}
			}
		}
	}
	opened = true;
	return opened;
}

void TxtNfcDevice::close()
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "close",0);
	nfc_close(pnd);
	nfc_exit(context);
	pnd = nullptr;
	opened = false;
}

std::string TxtNfcDevice::readTagsGetUID()
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "readTagsGetUID",0);
	if (nfcData) {
		delete nfcData;
		nfcData = NULL;
	}

	std::string uid;
	FreefareTag* tags;

	if (!opened)
	{
		open();
	}
	if (!opened)
	{
		SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "Open nfc device first!",0);
		return "";
	}

	if (!(tags = freefare_get_tags(pnd))) {
		SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "Error listing tags.",0);
	} else {
		for (int i = 0; tags[i]; i++) {
			SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "tags i: {}",i);
			SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "freefare_get_tag_type: {}",(int)freefare_get_tag_type(tags[i]));
			switch (freefare_get_tag_type(tags[i])) {
			case NTAG_21x:
				SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "NTAG_21x",0);
				break;
			case MIFARE_ULTRALIGHT:
				SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "MIFARE_ULTRALIGHT",0);
				break;
			case MIFARE_CLASSIC_1K:
				SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "MIFARE_CLASSIC_1K",0);
				break;
			default:
				SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "continue",0);
				continue;
			}
			FreefareTag tag = tags[i];

			char *tag_uid = freefare_get_tag_uid(tag);

			nfcData = new TxtNfcData();
			if (nfcData)
			{
				nfcData->wp.tag_uid = std::string(tag_uid);
				nfcData->wp.state = TxtWPState_t::WP_STATE_NONE;
				nfcData->wp.type = TxtWPType_t::WP_TYPE_NONE;
			}

			uid = std::string(tag_uid);
			SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "Tag with UID {}",tag_uid);
			free(tag_uid);
		}
	}
	freefare_free_tags(tags);
	if (!uid.empty()) {
		lastCmd = NFC_READ_UID;
	} else {
		lastCmd = NFC_ERROR;
	}
	return uid;
}

bool TxtNfcDevice::eraseTags()
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "eraseTags",0);
	if (nfcData) {
		delete nfcData;
		nfcData = NULL;
	}

	//see https://github.com/nfc-tools/libfreefare/blob/master/examples/ntag-write.c
	bool suc = false;
	FreefareTag* tags;

	if (!opened)
	{
		open();
	}
	if (!opened)
	{
		SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "Open nfc device first!",0);
		return false;
	}

	if (!(tags = freefare_get_tags(pnd))) {
		SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "Error listing tags.",0);
	} else {
		for (int i = 0; tags[i]; i++) {
			switch (freefare_get_tag_type(tags[i])) {
			case NTAG_21x:
			case MIFARE_ULTRALIGHT:
				break;
			default:
				continue;
			}
			FreefareTag tag = tags[i];

			char *tag_uid = freefare_get_tag_uid(tag);
			printf("Tag with UID %s is a %s\n", tag_uid, freefare_get_tag_friendly_name(tags[i]));
			if (ntag21x_connect(tag) < 0)
				SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "Error connecting to tag.",0);

			/* Get information about tag
			 * MUST do, because here we are recognizing tag subtype (NTAG213,NTAG215,NTAG216), and gathering all parameters */
			int res = ntag21x_get_info(tag);
			if (res < 0) {
				printf("Error getting info from tag\n");
				break;
			}

			//---
			SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "...",0);
			uint8_t data [4] = {0x00, 0x00, 0x00, 0x00}; // Data to write on tag
			// writing to tag 4 bytes on page 0x27 (check specs for NTAG21x before changing page number !!!)

			for(unsigned int page = 0x4; page <= 0x27; page++)
			{
				res = ntag21x_write(tag, page, data);
				if (res < 0) {
					printf("Error writing to tag\n");
					suc = false;
					break;
				}
			}

			nfcData = new TxtNfcData();
			if (nfcData)
			{
				nfcData->wp.tag_uid = std::string(tag_uid);
				nfcData->wp.state = TxtWPState_t::WP_STATE_RAW;
				nfcData->wp.type = TxtWPType_t::WP_TYPE_NONE;
			}

			suc = true;
			//---

			ntag21x_disconnect(tag);
			free(tag_uid);
		}
	}

	freefare_free_tags(tags);
	if (suc) {
		lastCmd = NFC_DELETE;
	} else {
		lastCmd = NFC_ERROR;
	}
	return suc;
}

std::string TxtNfcDevice::readTags()
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "readTags",0);
	if (nfcData) {
		delete nfcData;
		nfcData = NULL;
	}

	//see https://github.com/nfc-tools/libfreefare/blob/master/examples/ntag-write.c
	bool suc = false;
	std::string tuid;
	FreefareTag* tags;

	if (!opened)
	{
		open();
	}
	if (!opened)
	{
		SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "Open nfc device first!",0);
		return "";
	}

	if (!(tags = freefare_get_tags(pnd))) {
		SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "Error listing tags.",0);
	} else {
		for (int i = 0; tags[i]; i++) {
			SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "tags i: {}",i);
			SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "freefare_get_tag_type: {}",(int)freefare_get_tag_type(tags[i]));
			switch (freefare_get_tag_type(tags[i])) {
			case NTAG_21x:
			case MIFARE_ULTRALIGHT:
				SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "break",0);
				break;
			default:
				SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "continue",0);
				continue;
			}
			FreefareTag tag = tags[i];

			char *tag_uid = freefare_get_tag_uid(tag);
			tuid = tag_uid;
			//printf("Tag with UID %s is a %s\n", tag_uid, freefare_get_tag_friendly_name(tags[i]));
			SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "Tag with UID {} is a {}",tag_uid, freefare_get_tag_friendly_name(tags[i]));
			if (ntag21x_connect(tag) < 0)
				SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "Error connecting to tag.",0);

			/* Get information about tag
			 * MUST do, because here we are recognizing tag subtype (NTAG213,NTAG215,NTAG216), and gathering all parameters */
			int res = ntag21x_get_info(tag);
			if (res < 0) {
				printf("Error getting info from tag\n");
				break;
			}

			//---
			SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "...",0);
			uint8_t buffer[36*4]; // Buffer for reading data from tag
			res = ntag21x_fast_read(tag, 0x4, 0x27, buffer);
			if (res < 0) {
				printf("Error reading tag pages\n");
				break;
			}

#ifdef DEBUG
			printRawData(buffer);
#endif
			nfcData = new TxtNfcData();
			if (nfcData)
			{
				nfcData->wp.tag_uid = tuid;
				nfcData->wp.state = (TxtWPState_t)buffer[0];
				nfcData->wp.type = (TxtWPType_t)buffer[1];
				nfcData->mask_ts = buffer[2];
				//buffer[3]
				for(unsigned int i=0; i < NUM_INDEX_MAX; i++)
				{
					uTS ts;
					if (((nfcData->mask_ts >> i) & 0x1) == 1)
					{
						TxtHistoryCode_t code = (TxtHistoryCode_t)toCode((TxtHistoryIndex_t)i);
						for(unsigned int j=0; j < 8; j++)
						{
							ts.u8[j] = buffer[4+ i*8 +j];
						}
						nfcData->map_hist[code] = ts.s64;
					}
				}

				suc = true;
				printNfcData();
			}
			//---

			ntag21x_disconnect(tag);
			free(tag_uid);
		}
	}
	freefare_free_tags(tags);
	if (suc) {
		lastCmd = NFC_READ;
	} else {
		lastCmd = NFC_ERROR;
	}
	return suc?tuid:"";
}

bool TxtNfcDevice::writeTags(TxtWorkpiece wp, HistoryCode_map_t map_ts, uint8_t mask_ts)
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "writeTags {} {} {}",(int)wp.state, (int)wp.type, map_ts.size());
	if (nfcData) {
		delete nfcData;
		nfcData = NULL;
	}

	//see https://github.com/nfc-tools/libfreefare/blob/master/examples/ntag-write.c
	bool suc = false;
	FreefareTag* tags = nullptr;

	if (!opened)
	{
		open();
	}
	if (!opened)
	{
		SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "Open nfc device first!",0);
		return false;
	}

	if (!pnd || !(tags = freefare_get_tags(pnd))) {
		std::cerr << "tags failed" << std::endl;
		SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "Error listing tags.",0);
	} else {
		for (int i = 0; tags[i]; i++) {
			switch (freefare_get_tag_type(tags[i])) {
			case NTAG_21x:
			case MIFARE_ULTRALIGHT:
				break;
			default:
				continue;
			}
			FreefareTag tag = tags[i];
			char *tag_uid = freefare_get_tag_uid(tag);

			/*if (wp.tag_uid != std::string(tag_uid)) //TODO comparison wrong
			{
				std::cerr << "tag uid mismatch" << std::endl;
				lastCmd = NFC_WRITE_ERROR;
				return false;
			}*/

			printf("Tag with UID %s is a %s\n", tag_uid, freefare_get_tag_friendly_name(tags[i]));
			if (ntag21x_connect(tag) < 0)
				SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "Error connecting to tag.",0);

			/* Get information about tag
			 * MUST do, because here we are recognizing tag subtype (NTAG213,NTAG215,NTAG216), and gathering all parameters */
			int res = ntag21x_get_info(tag);
			if (res < 0) {
				printf("Error getting info from tag\n");
				break;
			}

			//---
			SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "write header",0);
			uint8_t data4[4]; // Data to write on tag
			data4[0] = (TxtWPState_t)wp.state;
			data4[1] = (TxtWPType_t)wp.type;
			data4[2] = mask_ts;
			//data4[3] = 0xff;
			// writing to tag 4 bytes on page 0x4
			res = ntag21x_write(tag, 0x4, data4);
			if (res < 0) {
				printf("Error writing to tag\n");
				suc = false;
				break;
			}
			//---
			SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "write vts",0);
			for(unsigned int i = 0; i < NUM_INDEX_MAX; i++)
			{
				//TODO code
				TxtHistoryCode_t code = (TxtHistoryCode_t)toCode((TxtHistoryIndex_t)i);

				if (((mask_ts >> i) & 0x1) == 1)
				{
					SPDLOG_LOGGER_DEBUG(spdlog::get("console"), "write {}",i);

					uTS uts;
					uts.s64 = map_ts[code];
	#ifdef DEBUG
					char sts[25];
					gettimestampstr(map_ts[code], sts);
					std::cout << "  uts: " << sts << std::endl;
	#endif

					uint8_t data[4]; // Data to write on tag
					data[0] = uts.u8[0];
					data[1] = uts.u8[1];
					data[2] = uts.u8[2];
					data[3] = uts.u8[3];
					// writing to tag 4 bytes on page 0x5...
					res = ntag21x_write(tag, 0x5+i*2, data);
					if (res < 0) {
						printf("Error writing to tag\n");
						suc = false;
						break;
					}
					uint8_t data2[4]; // Data to write on tag
					data2[0] = uts.u8[4];
					data2[1] = uts.u8[5];
					data2[2] = uts.u8[6];
					data2[3] = uts.u8[7];
					// writing to tag 4 bytes on page 0x6...
					res = ntag21x_write(tag, 0x5+i*2+1, data2);
					if (res < 0) {
						printf("Error writing to tag\n");
						suc = false;
						break;
					}
				}
			}

			nfcData = new TxtNfcData();
			if (nfcData)
			{
				nfcData->wp.tag_uid = std::string(tag_uid);
				nfcData->wp.state = wp.state;
				nfcData->wp.type = wp.type;
				nfcData->mask_ts = mask_ts;
				nfcData->map_hist = map_ts;
			}

			suc = true;
			//---
			ntag21x_disconnect(tag);
			free(tag_uid);
		}
	}
	freefare_free_tags(tags);
	if (suc) {
		lastCmd = NFC_WRITE;
	} else {
		lastCmd = NFC_WRITE_ERROR;
	}
	return suc;
}

void TxtNfcDevice::printNfcData()
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "printNfcDataLast", 0);
	if (nfcData)
	{
		std::cout << "NFC Data Last" << std::endl;
		std::cout << "  tag_uid: " << nfcData->wp.tag_uid << std::endl;
		std::cout << "  state: " << toString(nfcData->wp.state) << std::endl;
		std::cout << "  type: " << toString(nfcData->wp.type) << std::endl;
		std::cout << "  mask: " << "0x" << std::setfill('0') /*<< std::setw(2)*/ << std::hex << (unsigned int)(nfcData->mask_ts) << std::endl;
	    for (HistoryCode_map_t::const_iterator it=nfcData->map_hist.begin(); it!=nfcData->map_hist.end(); ++it)
	    {
	    	TxtHistoryCode_t hcode = it->first;
	    	int64_t hts = it->second;
			char sts[25];
			gettimestampstr(nfcData->map_hist[hcode], sts);
			std::cout << "  ts[" << hcode << "]: " << sts << std::endl;
		}
	}
}

void TxtNfcDevice::printRawData(uint8_t* buffer)
{
	SPDLOG_LOGGER_TRACE(spdlog::get("console"), "printData",0);
	//print data
	printf("data pages 0x4...0x27:\n");
	printf("---\n");
	for(int i = 0; i < 36*4; i++)
	{
		if ((i % 4) == 0) {
			printf("0x%02X: ", i/4 + 0x4);
		}
		printf("0x%02X ", buffer[i]);
		if (((i+1) % 4) == 0) {
			printf("\n");
		}
	}
	//printf("---\n");
	//printf("%36s\n", buffer);
	printf("---\n");
}


} /* namespace ft */
