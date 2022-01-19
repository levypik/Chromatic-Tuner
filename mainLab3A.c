/*
 * Copyright (c) 2009-2012 Xilinx, Inc.  All rights reserved.
 *
 * Xilinx, Inc.
 * XILINX IS PROVIDING THIS DESIGN, CODE, OR INFORMATION "AS IS" AS A
 * COURTESY TO YOU.  BY PROVIDING THIS DESIGN, CODE, OR INFORMATION AS
 * ONE POSSIBLE   IMPLEMENTATION OF THIS FEATURE, APPLICATION OR
 * STANDARD, XILINX IS MAKING NO REPRESENTATION THAT THIS IMPLEMENTATION
 * IS FREE FROM ANY CLAIMS OF INFRINGEMENT, AND YOU ARE RESPONSIBLE
 * FOR OBTAINING ANY RIGHTS YOU MAY REQUIRE FOR YOUR IMPLEMENTATION.
 * XILINX EXPRESSLY DISCLAIMS ANY WARRANTY WHATSOEVER WITH RESPECT TO
 * THE ADEQUACY OF THE IMPLEMENTATION, INCLUDING BUT NOT LIMITED TO
 * ANY WARRANTIES OR REPRESENTATIONS THAT THIS IMPLEMENTATION IS FREE
 * FROM CLAIMS OF INFRINGEMENT, IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 */

/*
 * helloworld.c: simple test application
 *
 * This application configures UART 16550 to baud rate 9600.
 * PS7 UART (Zynq) is not initialized by this application, since
 * bootrom/bsp configures it to baud rate 115200
 *
 * ------------------------------------------------
 * | UART TYPE   BAUD RATE                        |
 * ------------------------------------------------
 *   uartns550   9600
 *   uartlite    Configurable only in HW design
 *   ps7_uart    115200 (configured by bootrom/bsp)
 */
#include <unistd.h>
#include <stdlib.h>

#include <stdio.h>
#include <pthread.h>
#include "xil_cache.h"
#include <mb_interface.h>

#include "xparameters.h"
#include <xil_types.h>
#include <xil_assert.h>

#include <xio.h>
#include "xtmrctr.h"
#include "fft.h"
#include "note.h"
#include "stream_grabber.h"
#include "trig.h"

#define M 7 //2^m=samples

//effective range 3-6kHz  --> 5k, 3.6k, 2k
#define SAMPLES 512 // AXI4 Streaming Data FIFO has size 512
#define D 4
#define DD 1

//////effective range 375-750Hz  --> 440, 200
//#define SAMPLES 4096 // AXI4 Streaming Data FIFO has size 512
//#define D 32
//#define DD 4

#define CLOCK 100000000.0 //clock speed

XTmrCtr timer;
float tot_time; //time to run program
int ticks; //used for timer
//
//float cos[500]={0};
//float sin[500]={0};

int int_buffer[SAMPLES];
static float q[SAMPLES/D];
static float w[SAMPLES/D];

//void read_fsl_values(float* q, int n) {
//   int i;
//   unsigned int x;
//   stream_grabber_wait_enough_samples(SAMPLES/DD);
//
//   for(i = 0; i < n; i++) {
//	 x = 0;
//	 for(int j=0; j<(D/DD); j++){
//		 x += stream_grabber_read_sample(D*i+j);
//	 }
//	 x = x / (D/DD);
//	 q[i] = 3.3*x/67108864.0;
//   }
//
//   stream_grabber_start();
//}


int mainLab3A() {
   float sample_f;
   int l;
   uint32_t Control;
   float frequency; 

   Xil_ICacheInvalidate();
   Xil_ICacheEnable();
   Xil_DCacheInvalidate();
   Xil_DCacheEnable();

   //set up timer
   XTmrCtr_Initialize(&timer, XPAR_AXI_TIMER_0_DEVICE_ID);
   Control = XTmrCtr_GetOptions(&timer, 0) | XTC_CAPTURE_MODE_OPTION | XTC_INT_MODE_OPTION;
   XTmrCtr_SetOptions(&timer, 0, Control);
//
//   int z=0, k=0, n=SAMPLES/D, m=M, b=1;
//   for (int j=0; j<m; j++){
//   		for(int i=0; i<n; i+=2){
//   			if (i%(n/b)==0 && i!=0)
//   				k++;
//   			cos[z] = cosine(-PI*k/b);
//   			sin[z] = sine(-PI*k/b);
//   			z++;
//   		}
//
//   		b*=2;
//   		k=0;
//   	}


   print("Hello World\n\r");
   stream_grabber_start();
   while(1) {
	  XTmrCtr_Start(&timer, 0);

      //Read Values from Microblaze buffer, which is continuously populated by AXI4 Streaming Data FIFO.
//      read_fsl_values(q, SAMPLES/D);

      sample_f = (100*1000*1000/2048.0)/D;
      //xil_printf("sample frequency: %d \r\n",(int)sample_f);

      //zero w array
      for(l=0;l<SAMPLES/D;l++)
         w[l]=0;

      frequency=fft_og(q,w,(SAMPLES/D),M,sample_f); //Sample = 512
//      //ignore noise below set frequency
//      if(frequency > 200.0)
//         xil_printf("frequency: %d Hz\r\n", (int)(frequency+.5));
//      	  findNote(frequency);


      //get time to run program
	  ticks=XTmrCtr_GetValue(&timer, 0);
	  XTmrCtr_Stop(&timer, 0);
	  tot_time=ticks/CLOCK;
	  if(frequency > 190.0){
		  xil_printf("frequency: %d Hz\r\n", (int)(frequency+.5));
		  findNote(frequency);
	  }
	  xil_printf("program time: %dms \r\n",(int)(1000*tot_time));
   }


   return 0;
}
