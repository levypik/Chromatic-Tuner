/*****************************************************************************
* bsp.c for Lab2A of ECE 153a at UCSB
* Date of the Last Update:  October 27,2019
*****************************************************************************/

/**/
#include "qpn_port.h"
#include "bsp.h"
#include "lab2a.h"
#include "xintc.h"
#include "xil_exception.h"

#include <xio.h>
#include "xtmrctr.h"
#include "fft.h"
#include "note.h"
#include "stream_grabber.h"
#include "trig.h"
#include "math.h"

/*****************************/

/* Define all variables and Gpio objects here  */

XIntc sys_intc;
XGpio sys_encoder;
XTmrCtr sys_tmrctr;
XGpio sys_btn;
XGpio dc;
XSpi spi;

//FFT variables
float cosTable[500]={0};
float sinTable[500]={0};
float cosTable2[1024]={0};
float sinTable2[1024]={0};
float cosTable3[2500]={0};
float sinTable3[2500]={0};
float cosTable4[5200]={0};
float sinTable4[5200]={0};

int SAMPLES = 512;
int D = 4;
int DD = 1;
static float q[1024];
static float w[1024];
float sample_f = 0;
int l;
float frequency=0;
int octave = 2;
float total = 0;
int timer2 = 0;

//State Machine variables
int state = 0;
unsigned int last_press = 0;
unsigned int count = 0;
unsigned int timer = 100;
int button = 0;

#define GPIO_CHANNEL1 1

void debounceInterrupt(); // Write This function

/*..........................................................................*/
void BSP_init(void) {
	XStatus Status;
	XSpi_Config *spiConfig;

	Status = XST_SUCCESS;
	Status = XIntc_Initialize(&sys_intc, XPAR_MICROBLAZE_0_AXI_INTC_DEVICE_ID);
	if ( Status != XST_SUCCESS ){
		if( Status == XST_DEVICE_NOT_FOUND ){
			xil_printf("XST_DEVICE_NOT_FOUND...\r\n");
		}
		else{
			xil_printf("a different error from XST_DEVICE_NOT_FOUND...\r\n");
		}
		xil_printf("Interrupt controller driver failed to be initialized...\r\n");
	}
	xil_printf("Interrupt controller driver initialized!\r\n");

	Status = XTmrCtr_Initialize(&sys_tmrctr, XPAR_AXI_TIMER_0_DEVICE_ID);
	if ( Status != XST_SUCCESS ){
		xil_printf("Timer initialization failed...\r\n");
	}
	xil_printf("Initialized Timer!\r\n");

	Status = XGpio_Initialize(&sys_btn, XPAR_AXI_GPIO_BTN_DEVICE_ID);
	if ( Status != XST_SUCCESS ){
		xil_printf("Button initialization failed...\r\n");
	}
	xil_printf("Initialized Buttons!\r\n");

	Status = XGpio_Initialize(&sys_encoder, XPAR_AXI_GPIO_ENCODER_DEVICE_ID);
	if ( Status != XST_SUCCESS ){
		xil_printf("Encoder initialization failed...\r\n");
	}
	xil_printf("Initialized Encoder!\r\n");

	Status = XIntc_Connect(&sys_intc,XPAR_MICROBLAZE_0_AXI_INTC_AXI_TIMER_0_INTERRUPT_INTR,
			(XInterruptHandler)GpioHandler, &sys_tmrctr);
	if ( Status != XST_SUCCESS ){
		xil_printf("Failed to connect the timer handlers to the interrupt controller...\r\n");
	}
	xil_printf("Connected timer to Interrupt Controller!\r\n");

	Status = XIntc_Connect(&sys_intc,XPAR_MICROBLAZE_0_AXI_INTC_AXI_GPIO_BTN_IP2INTC_IRPT_INTR,
			(XInterruptHandler)ButtonHandler, &sys_btn);
	if ( Status != XST_SUCCESS ){
		xil_printf("Failed to connect the button handlers to the interrupt controller...\r\n");
	}
	xil_printf("Connected buttons to Interrupt Controller!\r\n");

	Status = XIntc_Connect(&sys_intc,XPAR_MICROBLAZE_0_AXI_INTC_AXI_GPIO_ENCODER_IP2INTC_IRPT_INTR,
			(XInterruptHandler)TwistHandler, &sys_encoder);
	if ( Status != XST_SUCCESS ){
		xil_printf("Failed to connect the encoder handlers to the interrupt controller...\r\n");
	}
	xil_printf("Connected Encoder to Interrupt Controller!\r\n");

	Status = XIntc_Start(&sys_intc, XIN_REAL_MODE);
	if ( Status != XST_SUCCESS ){
		xil_printf("Interrupt controller driver failed to start...\r\n");
	}
	xil_printf("Started Interrupt Controller!\r\n");

	XIntc_Enable(&sys_intc, XPAR_MICROBLAZE_0_AXI_INTC_AXI_TIMER_0_INTERRUPT_INTR);
	XIntc_Enable(&sys_intc, XPAR_MICROBLAZE_0_AXI_INTC_AXI_GPIO_BTN_IP2INTC_IRPT_INTR);
	XIntc_Enable(&sys_intc, XPAR_MICROBLAZE_0_AXI_INTC_AXI_GPIO_ENCODER_IP2INTC_IRPT_INTR);

	XTmrCtr_SetOptions(&sys_tmrctr, 0, XTC_INT_MODE_OPTION | XTC_AUTO_RELOAD_OPTION);
	XTmrCtr_SetResetValue(&sys_tmrctr, 0, 0xFFFFFFFF-0xF4240);		// 0x989680 = 10*10^6 clk cycles @ 100MHz = 100ms //RESET_VALUE=1000
	//989680 = 10*10^6 <<100ms we think    F4240 = 1*10^6 <<10ms we think      186A0 = 1*10^5 << 1ms    2710 = 10000 << .1ms
	XTmrCtr_Start(&sys_tmrctr, 0);

	XGpio_InterruptEnable(&sys_btn, 1);
	XGpio_InterruptGlobalEnable(&sys_btn);

	XGpio_InterruptEnable(&sys_encoder, 1);
	XGpio_InterruptGlobalEnable(&sys_encoder);

	u32 controlReg;
	Status = XGpio_Initialize(&dc, XPAR_SPI_DC_DEVICE_ID);
	if (Status != XST_SUCCESS) {
		xil_printf("Initialize GPIO dc fail!\n");
	}

	XGpio_SetDataDirection(&dc, 1, 0x0);

	spiConfig = XSpi_LookupConfig(XPAR_SPI_DEVICE_ID);
	if (spiConfig == NULL) {
		xil_printf("Can't find spi device!\n");
	}

	Status = XSpi_CfgInitialize(&spi, spiConfig, spiConfig->BaseAddress);
	if (Status != XST_SUCCESS) {
		xil_printf("Initialize spi fail!\n");
	}

	XSpi_Reset(&spi);

	controlReg = XSpi_GetControlReg(&spi);
	XSpi_SetControlReg(&spi,
			(controlReg | XSP_CR_ENABLE_MASK | XSP_CR_MASTER_MODE_MASK) &
			(~XSP_CR_TRANS_INHIBIT_MASK));

	XSpi_SetSlaveSelectReg(&spi, ~0x01);

	microblaze_register_handler((XInterruptHandler)XIntc_DeviceInterruptHandler,
			(void*)XPAR_MICROBLAZE_0_AXI_INTC_DEVICE_ID);
	microblaze_enable_interrupts();
	xil_printf("Interrupts enabled!\r\n");

	initLCD();
	clrScr();

   int z=0, k=0, n=128, m=7, b=1;
   for (int j=0; j<m; j++){
		for(int i=0; i<n; i+=2){
			if (i%(n/b)==0 && i!=0)
				k++;
			cosTable[z] = cosine(-PI*k/b);
			sinTable[z] = sine(-PI*k/b);
			z++;
		}

		b*=2;
		k=0;
   }

   z=0, k=0, n=256, m=8, b=1;
  for (int j=0; j<m; j++){
	for(int i=0; i<n; i+=2){
		if (i%(n/b)==0 && i!=0)
			k++;
		cosTable2[z] = cosine(-PI*k/b);
		sinTable2[z] = sine(-PI*k/b);
		z++;
	}

	b*=2;
	k=0;
  }

  	z=0, k=0, n=512, m=9, b=1;
    for (int j=0; j<m; j++){
		for(int i=0; i<n; i+=2){
			if (i%(n/b)==0 && i!=0)
				k++;
			cosTable3[z] = cosine(-PI*k/b);
			sinTable3[z] = sine(-PI*k/b);
			z++;
		}

		b*=2;
		k=0;
    }

    z=0, k=0, n=1024, m=10, b=1;
	for (int j=0; j<m; j++){
		for(int i=0; i<n; i+=2){
			if (i%(n/b)==0 && i!=0)
				k++;
			cosTable4[z] = cosine(-PI*k/b);
			sinTable4[z] = sine(-PI*k/b);
			z++;
		}

		b*=2;
		k=0;
	}
//    xil_printf("num=%d\r\n", z);


}
/*..........................................................................*/
void QF_onStartup(void) {                 /* entered with interrupts locked */
}

void read_fsl_values(float* q, int n) {
   int i;
   unsigned int x;
   int nSamp = SAMPLES;
   int num = 1;
   if(SAMPLES > 4096){
	   num = SAMPLES / 4096;
	   nSamp = 4096;
   }

   for(int k=0; k<num; k++){
	   stream_grabber_wait_enough_samples(nSamp);
	   stream_grabber_start();

	   for(i = 0; i<(n/num); i++) {
		 x = 0;
		 for(int j=0; j<D; j++){
			 x += stream_grabber_read_sample(D*i+j);
		 }
		 x = x / D;
		 q[i+((n/num)*k)] = 3.3*x/67108864.0;
	   }
   }
}

void runFFT(int samp, int m){
	  read_fsl_values(q, samp);

	  for(l=0;l<samp;l++)
		 w[l]=0;

	  frequency=fft_og(q, w, samp, m, sample_f);
}

void QF_onIdle(void) {        /* entered with interrupts locked */
    QF_INT_UNLOCK();                       /* unlock interrupts */

//    timer2 = count;
	  total = 0;
	  SAMPLES = 2048;
	  D = 4;
	  sample_f = (100*1000*1000/2048.0)/D;
	  runFFT(512, 9);
	  if(frequency > sample_f/4){//effective range 3000-6000Hz
		  total += frequency;
		  SAMPLES = 4096;
		  D = 4;
		  sample_f = (100*1000*1000/2048.0)/D;
		  runFFT(1024, 10);
		  total += frequency;
		  runFFT(1024, 10);
		  total += frequency;
		  frequency = total/3;
	  }else if(frequency > sample_f/8){//effective range 1500-3000Hz
		  SAMPLES = 4096;
		  D = 8;
		  sample_f = (100*1000*1000/2048.0)/D;
		  runFFT(512, 9);
		  total += frequency;
		  runFFT(512, 9);
		  total += frequency;
		  runFFT(512, 9);
		  total += frequency;
		  frequency = total/3;
	  }else if(frequency > sample_f/16){//effective range 750-1500Hz
		  SAMPLES = 8192;
		  D = 16;
		  sample_f = (100*1000*1000/2048.0)/D;
		  runFFT(512, 9);
		  total += frequency;
		  runFFT(512, 9);
		  total += frequency;
		  runFFT(512, 9);
		  total += frequency;
		  frequency = total/3;
	  }else if(frequency > sample_f/32){ //effective range 375-750Hz
		  SAMPLES = 16384;
		  D = 32;
		  sample_f = (100*1000*1000/2048.0)/D;
		  runFFT(512, 9);
	  }else if(frequency > sample_f/64){ //effective range 187.5-375Hz
		  SAMPLES = 32768;
		  D = 64;
		  sample_f = (100*1000*1000/2048.0)/D;
		  runFFT(512, 9);
	  }else{ //if(frequency > sample_f/128){//effective range 93.75-187.5Hz
		  SAMPLES = 32768;
		  D = 128;
		  sample_f = (100*1000*1000/2048.0)/D;
		  runFFT(256, 8);
	  }
//	  xil_printf("time=%d\r\n", (count-timer2));

	  if(octave == -1){
		  if(frequency > 20 && frequency < 6000){
//			  for(int f=1; f<((SAMPLES/D)/2); f++){
//				  xil_printf("bin %d: %d\r\n", f, (int)(new_2[f]));
//			  }
			  QActive_postISR((QActive *)&AO_Lab2A, FFT);
		  }
	  }else if(octave == 2){
		  if(frequency >= a4freq*(pow(2.0, ((float)(11.5-9)/12 + (float)(1-4))))
				  && frequency < a4freq*(pow(2.0, ((float)(11.5-9)/12 + (float)(2-4))))){
			  QActive_postISR((QActive *)&AO_Lab2A, FFT);
		  }
	  }else if(octave == 3){
		  if(frequency >= a4freq*(pow(2.0, ((float)(11.5-9)/12 + (float)(2-4))))
				  && frequency < a4freq*(pow(2.0, ((float)(11.5-9)/12 + (float)(3-4))))){
			  QActive_postISR((QActive *)&AO_Lab2A, FFT);
		  }
	  }else if(octave == 4){
		  if(frequency >= a4freq*(pow(2.0, ((float)(11.5-9)/12 + (float)(3-4))))
				  && frequency < a4freq*(pow(2.0, ((float)(11.5-9)/12 + (float)(4-4))))){
			  QActive_postISR((QActive *)&AO_Lab2A, FFT);
		  }
	  }else if(octave == 5){
		  if(frequency >= a4freq*(pow(2.0, ((float)(11.5-9)/12 + (float)(4-4))))
				  && frequency < a4freq*(pow(2.0, ((float)(11.5-9)/12 + (float)(5-4))))){
			  QActive_postISR((QActive *)&AO_Lab2A, FFT);
		  }
	  }else if(octave == 6){
		  if(frequency >= a4freq*(pow(2.0, ((float)(11.5-9)/12 + (float)(5-4))))
				  && frequency < a4freq*(pow(2.0, ((float)(11.5-9)/12 + (float)(6-4))))){
			  QActive_postISR((QActive *)&AO_Lab2A, FFT);
		  }
	  }else if(octave == 7){
		  if(frequency >= a4freq*(pow(2.0, ((float)(11.5-9)/12 + (float)(6-4))))
				  && frequency < a4freq*(pow(2.0, ((float)(11.5-9)/12 + (float)(7-4))))){
			  QActive_postISR((QActive *)&AO_Lab2A, FFT);
		  }
	  }

}

/* Do not touch Q_onAssert */
/*..........................................................................*/
void Q_onAssert(char const Q_ROM * const Q_ROM_VAR file, int line) {
    (void)file;                                   /* avoid compiler warning */
    (void)line;                                   /* avoid compiler warning */
    QF_INT_LOCK();
    for (;;) {
    }
}

// Interrupt handler functions here.  Do not forget to include them in lab2a.h!
void GpioHandler(void *CallbackRef) {
	Xuint32 ControlStatusReg;
	ControlStatusReg = XTimerCtr_ReadReg(sys_tmrctr.BaseAddress, 0, XTC_TCSR_OFFSET);
	count++;
	if(timer != -1 && timer + 200 <= count){
		timer = -1;
		QActive_postISR((QActive *)&AO_Lab2A, IDLE);
	}
	XTmrCtr_WriteReg(sys_tmrctr.BaseAddress, 0, XTC_TCSR_OFFSET, ControlStatusReg |XTC_CSR_INT_OCCURED_MASK);
}

void TwistHandler(void *CallbackRef) {
	if ((XGpio_InterruptGetStatus(&sys_encoder) & 1) != 1) {
		return;
	}
	int encoder = XGpio_DiscreteRead(&sys_encoder, 1);
	if(encoder == 7 && last_press < count-15){
		last_press = count;
		QActive_postISR((QActive *)&AO_Lab2A, ENCODER_CLICK);
	}else if(encoder == 0){
				if(state == 1 || state == 3){
					state = 2;
				}else if(state == 4 || state == 6){
					state = 5;
				}
	}else if(encoder == 1){
				if(state == 0 || state == 2){
					state = 1;
				}else if(state == 5){
					state = 6;
				}
	}else if(encoder == 2){
				if(state == 0 || state == 5){
					state = 4;
				}else if(state == 2){
					state = 3;
				}
	}else if(encoder == 3){
				if(state == 1 || state == 4){
					state = 0;
				}else if(state == 3){
					state = 0;
					timer = count;
					QActive_postISR((QActive *)&AO_Lab2A, ENCODER_UP);
				}else if(state == 6){
					state = 0;
					timer = count;
					QActive_postISR((QActive *)&AO_Lab2A, ENCODER_DOWN);
				}
	}else if(encoder == 4){
				if(state == 1 || state == 3){
					state = 2;
				}else if(state == 4 || state == 6){
					state = 5;
				}
	}else if(encoder == 5){
				if(state == 0 || state == 2){
					state = 1;
				}else if(state == 5){
					state = 6;
				}
	}else if(encoder == 6){
				if(state == 0 || state == 5){
					state = 4;
				}else if(state == 2){
					state = 3;
				}
	}
	XGpio_InterruptClear(&sys_encoder, 1);
}

void ButtonHandler(void *CallbackRef) {
	XGpio_InterruptDisable(&sys_btn, 1);
	if ((XGpio_InterruptGetStatus(&sys_btn) & 1) != 1) {
		return;
	}
	button = XGpio_DiscreteRead(&sys_btn, 1);
	if(button == 0){//UNPRESS
	}else if(button == 1){//UP
		timer = count;
		QActive_postISR((QActive *)&AO_Lab2A, TUNER);
	}else if(button == 2){//LEFT
		QActive_postISR((QActive *)&AO_Lab2A, HIST);
	}else if(button == 4){//Right
		QActive_postISR((QActive *)&AO_Lab2A, OCT);
	}else if(button == 8){//DOWN
		QActive_postISR((QActive *)&AO_Lab2A, SPEC);
	}else if(button == 16){//CENTER
		QActive_postISR((QActive *)&AO_Lab2A, MENU);
	}
	XGpio_InterruptClear(&sys_btn, 1);
	XGpio_InterruptEnable(&sys_btn, 1);
}

