#include "TxtBME680BSECLib.h"


const char * calibfile = "bme680.cloud.state";

/**********************************************************************************************************************/
/* functions */
/**********************************************************************************************************************/

/*!
 * @brief           Write operation in either I2C or SPI
 *
 * param[in]        dev_addr        I2C or SPI device address
 * param[in]        reg_addr        register address
 * param[in]        reg_data_ptr    pointer to the data to be written
 * param[in]        data_len        number of bytes to be written
 *
 * @return          result of the bus communication function
 */
int8_t bus_write(uint8_t dev_addr, uint8_t reg_addr, uint8_t *reg_data_ptr, uint16_t data_len)
{
    // ...
    // Please insert system specific function to write to the bus where BME680 is connected
    // ...
    int8_t rslt = 0; /* Return 0 for Success, non-zero for failure */
    uint8_t u8WriteData[data_len+1];
    u8WriteData[0] = reg_addr;
    for(int i=0; i < data_len+1; i++) {
    	u8WriteData[i+1] = reg_data_ptr[i];
    }
    //Adr, Anz Wr, Wr-Data, Anz Read, Rd-Data, Speed
    uint32_t u32RetValue = KeLibI2cTransfer(dev_addr, data_len+1, u8WriteData, 0, 0, I2C_SPEED_400_KHZ);
    rslt = (u32RetValue==0?0:-1); //0=success, -1 error

    return rslt;
}

/*!
 * @brief           Read operation in either I2C or SPI
 *
 * param[in]        dev_addr        I2C or SPI device address
 * param[in]        reg_addr        register address
 * param[out]       reg_data_ptr    pointer to the memory to be used to store the read data
 * param[in]        data_len        number of bytes to be read
 *
 * @return          result of the bus communication function
 */
int8_t bus_read(uint8_t dev_addr, uint8_t reg_addr, uint8_t *reg_data_ptr, uint16_t data_len)
{
    // ...
    // Please insert system specific function to read from bus where BME680 is connected
    // ...
    int8_t rslt = 0; /* Return 0 for Success, non-zero for failure */

    //Adr, Anz Wr, Wr-Data, Anz Read, Rd-Data, Speed
    uint32_t u32RetValue = KeLibI2cTransfer(dev_addr, 1, &reg_addr, data_len, reg_data_ptr, I2C_SPEED_400_KHZ);
    rslt = (u32RetValue==0?0:-1); //0=success, -1 error

    return rslt;
}

/*!
 * @brief           System specific implementation of sleep function
 *
 * @param[in]       t_ms    time in milliseconds
 *
 * @return          none
 */
void bsecsleep(uint32_t t_ms)
{
    // ...
    // Please insert system specific function sleep or delay for t_ms milliseconds
    // ...
    //usleep(1000 * t_ms); // wait ms
	std::this_thread::sleep_for(std::chrono::milliseconds((int64_t)t_ms));
}

/*!
 * @brief           Capture the system time in microseconds
 *
 * @return          system_current_time    current system timestamp in microseconds
 */
int64_t get_timestamp_us()
{
    int64_t system_current_time;
    // ...
    // Please insert system specific function to retrieve a timestamp (in microseconds)
    // ...
	struct timeval currentTime;
	gettimeofday(&currentTime, NULL);
	system_current_time = currentTime.tv_sec * (long long int)1e6 + currentTime.tv_usec;

	return system_current_time;
}

/*!
 * @brief           Load previous library state from non-volatile memory
 *
 * @param[in,out]   state_buffer    buffer to hold the loaded state string
 * @param[in]       n_buffer        size of the allocated state buffer
 *
 * @return          number of bytes copied to state_buffer
 */
uint32_t state_load(uint8_t *state_buffer, uint32_t n_buffer)
{
    // ...
    // Load a previous library state from non-volatile memory, if available.
    //
    // Return zero if loading was unsuccessful or no state was available,
    // otherwise return length of loaded state string.
    // ...
	printf("state_load %s... ", calibfile);
	if (state_buffer == NULL) {
		printf("state_buffer is NULL.\r\n");
		return 0;
	}
	printf("n_buffer:%d ", n_buffer);
	FILE* fp;
	uint8_t c;
	uint32_t i = 0;
	fp = fopen(calibfile, "rb");
	if (fp == 0) {
		printf("Cannot open the file %s\r\n", calibfile);
		return 0;
	}
	int prev=ftell(fp);
	fseek(fp, 0L, SEEK_END);
	size_t sz=ftell(fp);
	/*TODO sz is 65, slould be 304!
	 *if (sz != BSEC_MAX_PROPERTY_BLOB_SIZE)
	{
		remove("bme680.state");
		printf("remove %s... (wrong size: %d instead of %d)\r\n", calibfile, sz, BSEC_MAX_PROPERTY_BLOB_SIZE);
		return 0;
	}*/
	fseek(fp,prev,SEEK_SET); //go back to where we were
	printf("buffer size:%d ", sz);
	n_buffer = sz;
	while(1) {
		c = fgetc(fp);
		if( feof(fp) ) {
			break;
		}
		state_buffer[i++] = c;
		//printf("%i ", i);
    }
	fflush(fp);
    fclose(fp);
	printf("OK.\r\n");
	return n_buffer;
}

/*!
 * @brief           Save library state to non-volatile memory
 *
 * @param[in]       state_buffer    buffer holding the state to be stored
 * @param[in]       length          length of the state string to be stored
 *
 * @return          none
 */
void state_save(const uint8_t *state_buffer, uint32_t length)
{
    // ...
    // Save the string some form of non-volatile memory, if possible.
    // ...
	printf("state_save %s... ", calibfile);
	FILE* fp;
	fp = fopen(calibfile, "w+b");
	if (fp == 0) {
		printf("Cannot open the file %s\r\n", calibfile);
		return;
	}
	for(uint32_t i = 0; i < length; i++) {
	  fputc(state_buffer[i], fp);
    }
	fflush(fp);
    fclose(fp);
	printf("OK.\r\n");
}

/*!
 * @brief           Load library config from non-volatile memory
 *
 * @param[in,out]   config_buffer    buffer to hold the loaded state string
 * @param[in]       n_buffer        size of the allocated state buffer
 *
 * @return          number of bytes copied to config_buffer
 */
uint32_t config_load(uint8_t *config_buffer, uint32_t n_buffer)
{
    // ...
    // Load a library config from non-volatile memory, if available.
    //
    // Return zero if loading was unsuccessful or no config was available,
    // otherwise return length of loaded config string.
    // ...
	printf("config_load bme680... ");
	for(uint32_t i = 0; i < sizeof(bsec_config_iaq); i++) {
		config_buffer[i] = bsec_config_iaq[i];
    }
	printf("n:%d ", sizeof(bsec_config_iaq));
	printf("OK.\r\n");
    return sizeof(bsec_config_iaq);
}
