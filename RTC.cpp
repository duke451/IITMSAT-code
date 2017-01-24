#include "mbed.h"
SPI spi(PTD6, PTD7, PTD5); // mosi, miso, sclk
DigitalOut cs(PTD2);
 
void spiwrite(int);
 
 
int main() 
{
 
 
 
    // Setup the spi for 8 bit data, high steady state clock,
    // second edge capture, with a 1MHz clock rate
    spi.format(8,3);
    spi.frequency(1000000);
 
    // Send the commands to write to test the seconds register
    /*cs = 0;
    spi.write(0x80);  // set write register to seconds
    spi.write(0x00);  // send value of one*/
    spiwrite(0x80);
    
    cs=1;
    cs=0;
     spi.write(0x81);  //write reg to minutes
    spi.write(0x00);
    spiwrite(0x81);
    
    cs=1;
    cs=0;
    
    spi.write(0x82);  //write reg to minutes
    spi.write(0x01);
    
    cs=1;
    
    cs = 0;
    spi.write(0x83);  // set write register to seconds
    spi.write(0x01);  // send value of one
    
    cs=1;
    
    cs = 0;
    spi.write(0x84);  // set write register to seconds
    spi.write(0x01);  // send value of one
    
    cs=1;
    
    cs = 0;
    spi.write(0x85);  // set write register to seconds
    spi.write(0x01);  // send value of one
    
    cs=1;
    
    cs = 0;
    spi.write(0x86);  // set write register to seconds
    spi.write(0x01);  // send value of one
    
    cs=1;
    // Receive the contents of the seconds register
   
   for(int i=0;i<1000000;i++){
    
        
    cs=0;
    spi.write(0x00);               // set read register to seconds
    int seconds = spi.write(0x00);   // read the value
    
    cs=1;
    cs=0;
    
    spi.write(0x01);
    int minutes =spi.write(0x00);
    
    
    cs=1;
    cs=0;
    
    spi.write(0x02);
    int hours =spi.write(0x01);
    cs = 1;  //This cs=1 is to make the chipselect line high to "deselect" the slave in our case RTC
    
    cs=0;
    
    spi.write(0x03);
    int day =spi.write(0x01);
    cs = 1;
    
    cs=0;
    
    spi.write(0x04);
    int date =spi.write(0x01);
    cs = 1;
    
    cs=0;
    
    spi.write(0x05);
    int month =spi.write(0x01);
    cs = 1;
    
    cs=0;
    
    spi.write(0x06);
    int year =spi.write(0x01);
    cs = 1;
    
    wait(1);
    //This printf function is to check the timestamp function in the terminal output
    printf("Seconds register = %2X : %2X : %2X : %2X : %2X : %2X : %2X \n\r",year,month,date,day, hours,minutes,seconds);
    }
    
    return 0;
    
    
    
    }
    
    void spiwrite(int a)
    {
        cs=1;
        cs=0;
        spi.write(a);
        spi.write(0x01);
    }
            
