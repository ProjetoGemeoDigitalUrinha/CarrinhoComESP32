/**
 * @author Jose R. Padron
 * @author Used HMC6352 library  developed by Aaron Berk as template
 * @section LICENSE
 
 * Copyright (c) 2010 ARM Limited
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * @section DESCRIPTION
 *
 * Honeywell HMC5883Ldigital compass.
 *
 * Datasheet:
 *
 * http://www.ssec.honeywell.com/magnetic/datasheets/HMC5883L.pdf
 */
 
/**
 * Includes
 */
#include "HMC5883L.h"
 
HMC5883L::HMC5883L(PinName sda, PinName scl) {
 
    i2c_ = new I2C(sda, scl);
    //100KHz, as specified by the datasheet.
    i2c_->frequency(100000);
 
 
}
 
 
void HMC5883L::write(int address, int data) {
   
    char tx[2];
   
    tx[0]=address;
    tx[1]=data;
 
    i2c_->write(HMC5883L_I2C_WRITE,tx,2);
   
    wait_ms(100);
 
}
 
 
void HMC5883L::setSleepMode() {
    
    write(HMC5883L_MODE, HMC5883L_SLEEP);
}
 
void HMC5883L::setDefault(void) {
   
   write(HMC5883L_CONFIG_A,HMC5883L_10HZ_NORMAL);
   write(HMC5883L_CONFIG_B,HMC5883L_1_0GA);
   write(HMC5883L_MODE,HMC5883L_CONTINUOUS);
   wait_ms(100);
}
 
 
void HMC5883L::getAddress(char *buffer) {
    
   char rx[3];
   char tx[1];
   tx[0]=HMC5883L_IDENT_A;
    
       
    i2c_->write(HMC5883L_I2C_WRITE, tx,1);
   
    wait_ms(1);
    
    i2c_->read(HMC5883L_I2C_READ,rx,3);
    
    buffer[0]=rx[0];
    buffer[1]=rx[1];
    buffer[2]=rx[2];
}
 
 
 
void HMC5883L::setOpMode(int mode, int ConfigA, int ConfigB) {
    
    
    write(HMC5883L_CONFIG_A,ConfigA);
    write(HMC5883L_CONFIG_B,ConfigB);
    write(HMC5883L_MODE,mode);
    
 
}
 
 
 
 
void HMC5883L::readData(int* readings) {
 
  
    char tx[1];
    char rx[2];
    
    
    tx[0]=HMC5883L_X_MSB;
    i2c_->write(HMC5883L_I2C_READ,tx,1);
    i2c_->read(HMC5883L_I2C_READ,rx,2);
    readings[0]= (int)rx[0]<<8|(int)rx[1];
 
     
    tx[0]=HMC5883L_Y_MSB;
    i2c_->write(HMC5883L_I2C_READ,tx,1);
    i2c_->read(HMC5883L_I2C_READ,rx,2);
    readings[1]= (int)rx[0]<<8|(int)rx[1];
     
    tx[0]=HMC5883L_Z_MSB;
    i2c_->write(HMC5883L_I2C_READ,tx,1);
    i2c_->read(HMC5883L_I2C_READ,rx,2);
    readings[2]= (int)rx[0]<<8|(int)rx[1];
    
}
 
int HMC5883L::getMx() {
 
    char tx[1];
    char rx[2];
    
    
    tx[0]=HMC5883L_X_MSB;
    i2c_->write(HMC5883L_I2C_READ,tx,1);
    i2c_->read(HMC5883L_I2C_READ,rx,2);
    return ((int)rx[0]<<8|(int)rx[1]);
 
}
 
int HMC5883L::getMy() {
 
    char tx[1];
    char rx[2];
    
    
    tx[0]=HMC5883L_Y_MSB;
    i2c_->write(HMC5883L_I2C_READ,tx,1);
    i2c_->read(HMC5883L_I2C_READ,rx,2);
    return ((int)rx[0]<<8|(int)rx[1]);
 
}
 
 
int HMC5883L::getMz(){
 
    char tx[1];
    char rx[2];
    
    
    tx[0]=HMC5883L_Z_MSB;
    i2c_->write(HMC5883L_I2C_READ,tx,1);
    i2c_->read(HMC5883L_I2C_READ,rx,2);
    return ((int)rx[0]<<8|(int)rx[1]);
 
}
