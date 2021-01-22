#ifndef TXTBME680BSECLIB_H_
#define TXTBME680BSECLIB_H_

#include <stdio.h>          // for printf()
#include <string.h>         // for memset()
#include <unistd.h>         // for sleep()
#include <cmath>			// for pow()

#include "KeLibTxtDl.h"     // TXT Lib
#include "FtShmem.h"        // TXT Transfer Area

#include <sys/time.h>
#include <stdlib.h>
#include <thread>

#include "bsec_integration.h"
#include "bsec_serialized_configurations_iaq.h"


int8_t bus_write(uint8_t dev_addr, uint8_t reg_addr, uint8_t *reg_data_ptr, uint16_t data_len);
int8_t bus_read(uint8_t dev_addr, uint8_t reg_addr, uint8_t *reg_data_ptr, uint16_t data_len);
void bsecsleep(uint32_t t_ms);
int64_t get_timestamp_us();
uint32_t state_load(uint8_t *state_buffer, uint32_t n_buffer);
void state_save(const uint8_t *state_buffer, uint32_t length);
uint32_t config_load(uint8_t *config_buffer, uint32_t n_buffer);


#endif
