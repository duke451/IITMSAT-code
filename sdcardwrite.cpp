#include "mbed.h"
#include "mbed_debug.h"
 
#define SD_COMMAND_TIMEOUT 5000
 
#define SD_DBG             0
 
#define R1_IDLE_STATE           (1 << 0)
#define R1_ERASE_RESET          (1 << 1)
#define R1_ILLEGAL_COMMAND      (1 << 2)
#define R1_COM_CRC_ERROR        (1 << 3)
#define R1_ERASE_SEQUENCE_ERROR (1 << 4)
#define R1_ADDRESS_ERROR        (1 << 5)
#define R1_PARAMETER_ERROR      (1 << 6)
 
int initialise_card();
int initialise_card_v1();
int initialise_card_v2();
int disk_initialize();
int disk_write(const uint8_t *, uint64_t);
 
uint64_t sd_sectors();
uint64_t sectors;
 
int cmd(int, int);
int cmd58();
int cmdx(int, int);
int cmd8();
int read(uint8_t*, uint32_t );
int write(const uint8_t*, uint32_t );
static uint32_t ext_bits(unsigned char *, int , int );
 
int cdv;
 
#define SDCARD_FAIL 0
#define SDCARD_V1   1
#define SDCARD_V2   2
#define SDCARD_V2HC 3
 
 
 
SPI spi(PTD6, PTD7, PTD5); // mosi, miso, sclk
DigitalOut cs(PTD2);
int count_bro;
 
 
 
int main()
{
    
     initialise_card();
      int result= initialise_card();
      printf("initialise card result=%d\n",result);
      disk_initialize();
       uint8_t buffer[512];
      for(count_bro=0;count_bro<512;count_bro++)
      {buffer[count_bro]='a';}
      disk_write(buffer, 2);
      
      
    
}
 
 
 
 
int initialise_card() {
    // Set to 100kHz for initialisation, and clock card with cs = 1
    spi.frequency(100000);
    cs = 1;
    for (int i = 0; i < 16; i++) {
        spi.write(0xFF);
    }
    
    // send CMD0, should return with all zeros except IDLE STATE set (bit 0)
    if (cmd(0, 0) != R1_IDLE_STATE) {
        debug("No disk, or could not put SD card in to SPI idle state\n");
        return SDCARD_FAIL;
    }
 
 // send CMD8 to determine whther it is ver 2.x
    int r = cmd8();
    if (r == R1_IDLE_STATE) {
        printf("Entering v2 bro\n");
        return initialise_card_v2();
        
    } else if (r == (R1_IDLE_STATE | R1_ILLEGAL_COMMAND)) {
        printf("Entering v1 bro\n");
        return initialise_card_v1();
        
    } else {
        debug("Not in idle state after sending CMD8 (not an SD card?)\n");
        return SDCARD_FAIL;
    }
}
 
int initialise_card_v1() {
    for (int i = 0; i < SD_COMMAND_TIMEOUT; i++) {
        cmd(55, 0);
        if (cmd(41, 0) == 0) {
            printf("Yuppie v1 successful\n");
            cdv = 512;
            debug_if(SD_DBG, "\n\rInit: SEDCARD_V1\n\r");
            
            return SDCARD_V1;
        }
    }
    
    debug("Timeout waiting for v1.x card\n");
    return SDCARD_FAIL;
}
 
 
int initialise_card_v2() {
    for (int i = 0; i < SD_COMMAND_TIMEOUT; i++) {
        wait_ms(50);
        cmd58();
        cmd(55, 0);
        if (cmd(41, 0x40000000) == 0) {
            printf("Yuppie,v2 successful\n");
            cmd58();
            debug_if(SD_DBG, "\n\rInit: SDCARD_V2\n\r");
            cdv = 1;
        
            return SDCARD_V2;
        }
    }
    
    debug("Timeout waiting for v2.x card\n");
    return SDCARD_FAIL;
}
 
int cmd(int cmd, int arg) {
    cs = 0;
    
    // send a command
    spi.write(0x40 | cmd);
    spi.write(arg >> 24);
    spi.write(arg >> 16);
    spi.write(arg >> 8);
    spi.write(arg >> 0);
    spi.write(0x95);
    
    // wait for the repsonse (response[7] == 0)
for (int i = 0; i < SD_COMMAND_TIMEOUT; i++) {
        int response = spi.write(0xFF);
        if (!(response & 0x80)) {
            cs = 1;
            spi.write(0xFF);
            return response;
        }
    }
    cs = 1;
    spi.write(0xFF);
    return -1; // timeout
}
 
 
int cmd58() {
    cs = 0;
    int arg = 0;
    
    // send a command
    spi.write(0x40 | 58);
    spi.write(arg >> 24);
    spi.write(arg >> 16);
    spi.write(arg >> 8);
    spi.write(arg >> 0);
    spi.write(0x95);
    
    // wait for the repsonse (response[7] == 0)
    for (int i = 0; i < SD_COMMAND_TIMEOUT; i++) {
        int response = spi.write(0xFF);
        if (!(response & 0x80)) {
            int ocr = spi.write(0xFF) << 24;
            ocr |= spi.write(0xFF) << 16;
            ocr |= spi.write(0xFF) << 8;
            ocr |= spi.write(0xFF) << 0;
            cs = 1;
            spi.write(0xFF);
            return response;
        }
    }
    cs = 1;
    spi.write(0xFF);
    return -1; // timeout
}
 
 
int cmd8() {
    cs = 0;
    
    // send a command
    spi.write(0x40 | 8); // CMD8
    spi.write(0x00);     // reserved
    spi.write(0x00);     // reserved
    spi.write(0x01);     // 3.3v
    spi.write(0xAA);     // check pattern
    spi.write(0x87);     // crc
    
    // wait for the repsonse (response[7] == 0)
    for (int i = 0; i < SD_COMMAND_TIMEOUT * 1000; i++) {
        char response[5];
        response[0] = spi.write(0xFF);
        if (!(response[0] & 0x80)) {
            for (int j = 1; j < 5; j++) {
                response[i] = spi.write(0xFF);
            }
            cs = 1;
            spi.write(0xFF);
            return response[0];
        }
    }
    cs = 1;
    spi.write(0xFF);
    return -1; // timeout
}
 
uint64_t sd_sectors() {
    uint32_t c_size, c_size_mult, read_bl_len;
    uint32_t block_len, mult, blocknr, capacity;
    uint32_t hc_c_size;
    uint64_t blocks;
    
    // CMD9, Response R2 (R1 byte + 16-byte block read)
    if (cmdx(9, 0) != 0) {
        debug("Didn't get a response from the disk\n");
        return 0;
    }
    
    uint8_t csd[16];
    if (read(csd, 16) != 0) {
        debug("Couldn't read csd response from disk\n");
        return 0;
    }
    
    // csd_structure : csd[127:126]
    // c_size        : csd[73:62]
    // c_size_mult   : csd[49:47]
    // read_bl_len   : csd[83:80] - the *maximum* read block length
    
    int csd_structure = ext_bits(csd, 127, 126);
    
    switch (csd_structure) {
        case 0:
            cdv = 512;
            c_size = ext_bits(csd, 73, 62);
            c_size_mult = ext_bits(csd, 49, 47);
            read_bl_len = ext_bits(csd, 83, 80);
            
            block_len = 1 << read_bl_len;
            mult = 1 << (c_size_mult + 2);
            blocknr = (c_size + 1) * mult;
            capacity = blocknr * block_len;
            blocks = capacity / 512;
            debug_if(SD_DBG, "\n\rSDCard\n\rc_size: %d \n\rcapacity: %ld \n\rsectors: %lld\n\r", c_size, capacity, blocks);
            break;
        
        case 1:
            cdv = 1;
            hc_c_size = ext_bits(csd, 63, 48);
            blocks = (hc_c_size+1)*1024;
            debug_if(SD_DBG, "\n\rSDHC Card \n\rhc_c_size: %d\n\rcapacity: %lld \n\rsectors: %lld\n\r", hc_c_size, blocks*512, blocks);
            break;
        
        default:
            debug("CSD struct unsupported\r\n");
            return 0;
    };
    return blocks;
}
 
int cmdx(int cmd, int arg) {
    cs = 0;
    
    // send a command
    spi.write(0x40 | cmd);
    spi.write(arg >> 24);
    spi.write(arg >> 16);
    spi.write(arg >> 8);
    spi.write(arg >> 0);
    spi.write(0x95);
    
    // wait for the repsonse (response[7] == 0)
    for (int i = 0; i < SD_COMMAND_TIMEOUT; i++) {
        int response = spi.write(0xFF);
        if (!(response & 0x80)) {
            return response;
        }
    }
    cs = 1;
    spi.write(0xFF);
    return -1; // timeout
}
 
 
int read(uint8_t *buffer, uint32_t length) {
    cs = 0;
    
    // read until start byte (0xFF)
    while (spi.write(0xFF) != 0xFE);
    
    // read data
    for (int i = 0; i < length; i++) {
        buffer[i] = spi.write(0xFF);
    }
    spi.write(0xFF); // checksum
    spi.write(0xFF);
    
    cs = 1;
    spi.write(0xFF);
    return 0;
}
 
static uint32_t ext_bits(unsigned char *data, int msb, int lsb) {
    uint32_t bits = 0;
    uint32_t size = 1 + msb - lsb;
    for (int i = 0; i < size; i++) {
        uint32_t position = lsb + i;
        uint32_t byte = 15 - (position >> 3);
        uint32_t bit = position & 0x7;
        uint32_t value = (data[byte] >> bit) & 1;
        bits |= value << i;
    }
    return bits;
}
 
int disk_initialize() {
    int i = initialise_card();
    debug_if(SD_DBG, "init card = %d\n", i);
    sectors = sd_sectors();
    
    // Set block length to 512 (CMD16)
    if (cmd(16, 512) != 0) {
        debug("Set 512-byte block timed out\n");
        return 1;
    }
    else
    {
        printf("Hey,block init succesful\n");
    }
    
    spi.frequency(1000000); // Set to 1MHz for data transfer
    return 0;
}
 
int disk_write(const uint8_t *buffer, uint64_t block_number)
 
{
    // set write address for single block (CMD24)
    if (cmd(24, block_number * cdv) != 0) {
        return 1;
    }
    
    // send the data block
    write(buffer, 512);
    printf("Written Successfully bro \n");
    return 0;
}
 
int write(const uint8_t*buffer, uint32_t length) {
    cs = 0;
    
    // indicate start of block
    spi.write(0xFE);
    
    // write the data
    for (int i = 0; i < length; i++) {
        spi.write(buffer[i]);
    }
    
    // write the checksum
    spi.write(0xFF);
    spi.write(0xFF);
    
    // check the response token
    if ((spi.write(0xFF) & 0x1F) != 0x05) {
        cs = 1;
        spi.write(0xFF);
        return 1;
    }
    
    // wait for write to finish
    while (spi.write(0xFF) == 0);
    
    cs = 1;
    spi.write(0xFF);
    return 0;
}
 
 
            
