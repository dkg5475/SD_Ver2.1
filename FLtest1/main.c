#include "system_samc21.h"
#include "sam.h"
#include "samc21e18a.h"
#include "Driver_Common.h"
#include "Driver_SPI.h"
#include "Driver_USART.h"
#include <stdlib.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>

#define READ_TIMEOUT_MS 1000

ARM_DRIVER_USART USART0; // instance of ARM_DRIVER_USART from Driver_USART.h
ARM_DRIVER_USART *driver0 = &USART0; // pointer to the instance

volatile bool buffer_ready;
volatile bool rx_timeout;

uint8_t txBuffer[128] = {0};
uint8_t rxBuffer[128] = {0};

volatile uint32_t tickCount = 0; // Global tick counter

void SysTick_Init(uint32_t ticks) {

	// Disable SysTick
    SysTick->CTRL = 0;
    
    // Set reload value (adjust for your clock speed)
    SysTick->LOAD = ticks - 1;
    
    // Set priority (optional, lower number = higher priority)
    NVIC_SetPriority(SysTick_IRQn, 3); 
    
    // Enable SysTick with core clock and interrupt
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | 
                    SysTick_CTRL_TICKINT_Msk | 
                    SysTick_CTRL_ENABLE_Msk;

}

void Delay_ms(uint32_t ms) {
    uint32_t start = tickCount;
    while ((tickCount - start) < ms);
}


void USART_Read_Callback(uint32_t event) {
  uint32_t mask;
 
  mask = ARM_USART_EVENT_RECEIVE_COMPLETE  | 
				 ARM_USART_EVENT_TRANSFER_COMPLETE |
         ARM_USART_EVENT_SEND_COMPLETE     |
         ARM_USART_EVENT_TX_COMPLETE;
	
	if (event & mask) {
		buffer_ready = true;
	}
	
	if (event & ARM_USART_EVENT_RX_TIMEOUT) {
		rx_timeout = true;
	}
}



void USART0_Initialize(void) {

	//Register the callback function
	driver0->Initialize(USART_Read_Callback);
	
	// Power USART Peripheral
	driver0->PowerControl(ARM_POWER_FULL);
	
	// Configure USART settings
	// Asynchronous
	// 8 bit payload
	// No parity
	// 1 stop bit
	// No flow control
	// Baud rate: 14400 
	driver0->Control(ARM_USART_MODE_ASYNCHRONOUS | 
	ARM_USART_DATA_BITS_8 |
	ARM_USART_PARITY_NONE | 
	ARM_USART_STOP_BITS_1 | 
	ARM_USART_FLOW_CONTROL_NONE, 14400);
	
	driver0->Control(ARM_USART_CONTROL_TX, 1); // enable the transmitter
	driver0->Control(ARM_USART_CONTROL_RX, 1); // enable the receiver
	
}

typedef enum {
    READ_IDLE,
    READ_IN_PROGRESS,
    READ_COMPLETE,
    READ_TIMEOUT
} read_state_t;

read_state_t usart_read_state = READ_IDLE;
uint32_t read_start_time;

int32_t USART0_Read_NonBlocking(void) {
	int32_t status = 0;
	
	switch(usart_read_state) {
        
				case READ_IDLE:
            // Start new read
            status = driver0->Receive(rxBuffer, sizeof(rxBuffer)/sizeof(rxBuffer[0]));
            if (status == ARM_DRIVER_OK) {
                usart_read_state = READ_IN_PROGRESS;
                read_start_time = tickCount;
            }
            return status;
            
        case READ_IN_PROGRESS:
            if (buffer_ready) {
                usart_read_state = READ_IDLE;
                return driver0->GetRxCount();
            }
            else if ((tickCount - read_start_time) > READ_TIMEOUT_MS) {
                usart_read_state = READ_IDLE;
                driver0->Control(ARM_USART_ABORT_RECEIVE, 0);
                return ARM_DRIVER_ERROR_TIMEOUT;
            }
            return ARM_DRIVER_ERROR_BUSY;  // Still waiting
            
        default:
            usart_read_state = READ_IDLE;
            return ARM_DRIVER_ERROR;
    }
}


void USART0_Send (const void *data, uint32_t num) {
	driver0->Send(data, num);
}







int main () {
	SystemInit();
	
	
	while(1) {
			
				
  }

	return (EXIT_FAILURE);
	
}