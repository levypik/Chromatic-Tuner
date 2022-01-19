/*****************************************************************************
* lab2a.c for Lab2A of ECE 153a at UCSB
* Date of the Last Update:  October 23,2014
*****************************************************************************/

#define AO_LAB2A

#include "qpn_port.h"
#include "bsp.h"
#include "lab2a.h"
#include "lcd.h"
#include "fft.h"
#include "stdio.h"
#include "math.h"



typedef struct Lab2ATag  {               //Lab2A State machine
	QActive super;
}  Lab2A;

/* Setup state machines */
/**********************************************************************/
static QState Lab2A_initial (Lab2A *me);
static QState Lab2A_on      (Lab2A *me);
static QState Lab2A_stateM  (Lab2A *me);
static QState Lab2A_stateT  (Lab2A *me);
static QState Lab2A_stateH  (Lab2A *me);
static QState Lab2A_stateO  (Lab2A *me);
static QState Lab2A_stateS  (Lab2A *me);

/**********************************************************************/


Lab2A AO_Lab2A;

int volume = 0;
int idle = 0;
char freq[50] = "Frequency = " ;
char maxFreq[50] = "Max Frequency = ";
char bin[50] = "Bin Spacing = ";
char oct[20] = "Octave: ";
char a4[10] = "A4 = ";
float actual;
int a4freq = 440;
int o, n, o2;
float diff = 0;
float next, cent;
int cents;
char note[4] = {0};
int noteLen;
int bins[128] = {0};
int spect[100][256] = {0};

void draw_v_line(int x, int y, int l){
	setXY(x, y - l, x, y);
	for (int i = 0; i < l + 1; i++) {
		LCD_Write_DATA16(fch, fcl);
	}

}

void draw_diff(int x, int l){
	if(l > 220){
		l = 220;
	}

	int diff = l - bins[x];
	if(diff>0){
		setColor(0, 0, 255);
		int y = 250-bins[x];
		setXY(x+56, y - diff, x+56, y);
		for (int i = 0; i < diff + 1; i++){
			LCD_Write_DATA16(fch, fcl);
		}
	}else if(diff<0){
		setColor(0, 0, 0);
		int y = 250-(bins[x]+diff);
		setXY(x+56, y + diff, x+56, y);
		for (int i = 0; i < 1 - diff; i++){
			LCD_Write_DATA16(fch, fcl);
		}
	}
	bins[x] = l;
}

void draw_bin(int num, int size){
	int x = 56 + num;
	draw_diff(num, size);
}

void set_note(int r, int oct, char* note){
	switch(r) {
	   case 0:
		   note[0] = 'C';
		   note[1] = '0' + oct;
		   note[2] = ' ';
		   break;
	   case 1:
	  		note[0] = 'C';
	  		note[1] = '#';
	  		note[2] = '0' + oct;
	  		break;
	  	case 2:
	  		note[0] = 'D';
	  		note[1] = '0' + oct;
	  		note[2] = ' ';
	  		break;
	  	case 3:
	  		note[0] = 'D';
	  		note[1] = '#';
	  		note[2] = '0' + oct;
	  		break;
	  	case 4:
	  		note[0] = 'E';
	  		note[1] = '0' + oct;
	  		note[2] = ' ';
	  		break;
	  	case 5:
			note[0] = 'F';
			note[1] = '0' + oct;
			note[2] = ' ';
			break;
	  	case 6:
	  		note[0] = 'F';
	  		note[1] = '#';
	  		note[2] = '0' + oct;
	  		break;
	  	case 7:
	  		note[0] = 'G';
	  		note[1] = '0' + oct;
	  		note[2] = ' ';
	  		break;
	  	case 8:
	  		note[0] = 'G';
	  		note[1] = '#';
	  		note[2] = '0' + oct;
	  		break;
	  	case 9:
	  		note[0] = 'A';
	  		note[1] = '0' + oct;
	  		note[2] = ' ';
	  		break;
	  	case 10:
	  		note[0] = 'A';
	  		note[1] = '#';
	  		note[2] = '0' + oct;
	  		break;
	  	case 11:
	  		note[0] = 'B';
	  		note[1] = '0' + oct;
	  		note[2] = ' ';
	  		break;
	}
}

int get_note(float f, int oct){
	float new = f/a4freq;
	new = log2(new);
	new = new - (oct - 4);
	new = new * 12;
	new = new + 9;
	return (int)(new +.5);
}

int get_octave(float f){
	int oct = -1;
	if(f < a4freq*(pow(2.0, ((float)(11.5-9)/12 + (float)(0-4))))){
		oct = 0;
	}
	else if(f < a4freq*(pow(2.0, ((float)(11.5-9)/12 + (float)(1-4))))){
		oct = 1;
	}
	else if(f < a4freq*(pow(2.0, ((float)(11.5-9)/12 + (float)(2-4))))){  //octave 2
		oct = 2;
	}
	else if(f < a4freq*(pow(2.0, ((float)(11.5-9)/12 + (float)(3-4))))){ //octave 3
		oct = 3;
	}
	else if(f < a4freq*(pow(2.0, ((float)(11.5-9)/12 + (float)(4-4))))){ //octave 4
		oct = 4;
	}
	else if(f < a4freq*(pow(2.0, ((float)(11.5-9)/12 + (float)(5-4))))){ //octave 5
		oct = 5;
	}
	else if(f < a4freq*(pow(2.0, ((float)(11.5-9)/12 + (float)(6-4))))){ //octave 6
		oct = 6;
	}
	else if(f < a4freq*(pow(2.0, ((float)(11.5-9)/12 + (float)(7-4))))){ //octave 7
		oct = 7;
	}
	else if(f < a4freq*(pow(2.0, ((float)(11.5-9)/12 + (float)(8-4))))){
		oct = 8;
	}

	return oct;
}


void Lab2A_ctor(void)  {
	Lab2A *me = &AO_Lab2A;
	QActive_ctor(&me->super, (QStateHandler)&Lab2A_initial);
}


QState Lab2A_initial(Lab2A *me) {
	xil_printf("\n\rInitialization");
    return Q_TRAN(&Lab2A_on);
}

QState Lab2A_on(Lab2A *me) {
	switch (Q_SIG(me)) {
		case Q_ENTRY_SIG: {
			setColor(0, 0, 0);
			fillRect(0, 0, DISP_X_SIZE, DISP_Y_SIZE);
			return Q_HANDLED();
			}
			
		case Q_INIT_SIG: {
			return Q_TRAN(&Lab2A_stateM);
			}
	}
	
	return Q_SUPER(&QHsm_top);
}

QState Lab2A_stateM(Lab2A *me) { //MUTED
	switch (Q_SIG(me)) {
			case Q_ENTRY_SIG: {
				octave=-1;
				clrScr();
				setColor(0, 0, 0);
				fillRect(0, 0, DISP_X_SIZE, DISP_Y_SIZE);

				setColor(255, 255, 255);
				setFont(BigFont);
				lcdPrint("Menu Buttons", 20, 20);
				setColor(0, 0, 255);
				lcdPrint("2", 116-3, 50);
				lcdPrint("3", 74-3, 90-1);
				lcdPrint("4", 160-3, 90-1);
				lcdPrint("1", 114, 90-1);
				lcdPrint("5", 118-3, 130);
				setFont(various_symbols);
				lcdPrint("=", 116-3, 70);
				lcdPrint(">", 116-3, 110);
				lcdPrint("?", 94-3, 90-1);
				lcdPrint("T", 140-3, 90-1);

				setFont(BigFont);
				setColor(255, 255, 255);
				lcdPrint("1-Menu", 20, 200-30-10);
				lcdPrint("2-Chromatic", 20, 215-20-10);
				lcdPrint("Tuner", 53, 230-20-10);
				lcdPrint("3-FFT", 20, 245-10-10);
				lcdPrint("Histogram", 53, 260-10-10);
				lcdPrint("4-Octave", 20, 275-10);
				lcdPrint("Select", 53, 290-10);
				lcdPrint("5-Spectrogram", 20, 300);


				return Q_HANDLED();
			}

			case ENCODER_UP: {
				return Q_HANDLED();
			}
			case ENCODER_DOWN: {
				return Q_HANDLED();
			}
			case ENCODER_CLICK:  {
				return Q_HANDLED();
			}
			case MENU:  {
				return Q_HANDLED();
			}
			case TUNER:  {
				return Q_TRAN(&Lab2A_stateT);
			}
			case HIST:  {
				return Q_TRAN(&Lab2A_stateH);
			}
			case OCT:  {
				return Q_TRAN(&Lab2A_stateO);
			}
			case SPEC:  {
				return Q_TRAN(&Lab2A_stateS);
			}
			case FFT:  {
				return Q_HANDLED();
			}
			case IDLE:  {
				return Q_HANDLED();
			}
		}

		return Q_SUPER(&Lab2A_on);
}

QState Lab2A_stateT(Lab2A *me) { //UNMUTED
	switch (Q_SIG(me)) {
		case Q_ENTRY_SIG: {
			octave=-1;
			clrScr();
			setColor(255, 255, 255);
			setFont(BigFont);
			lcdPrint("Chromatic Tuner", 0, 10);
			snprintf(a4+5, 5, "%d", a4freq);
			lcdPrint(a4, 60, 250);

			lcdPrint("-50", 0, 182);
			lcdPrint("0", 112, 182);
			lcdPrint("50", 200, 182);

			setColor(255, 255, 255);
			fillRect(19, 170, 19, 180);
			setColor(255, 255, 255);
			fillRect(221, 170, 221, 180);
			return Q_HANDLED();
		}

		case ENCODER_UP: {
			if(a4freq < 460){
				a4freq++;
				snprintf(a4+5, 5, "%d", a4freq);
				setColor(255, 255, 255);
				setFont(BigFont);
				lcdPrint(a4, 60, 250);
			}
			return Q_HANDLED();
		}
		case ENCODER_DOWN: {
			if(a4freq > 420){
				a4freq--;
				snprintf(a4+5, 5, "%d", a4freq);
				setColor(255, 255, 255);
				setFont(BigFont);
				lcdPrint(a4, 60, 250);
			}
			return Q_HANDLED();
		}
		case ENCODER_CLICK:  {
			return Q_HANDLED();
		}
		case MENU:  {
			return Q_TRAN(&Lab2A_stateM);
		}
		case TUNER:  {
			return Q_HANDLED();
		}
		case HIST:  {
			return Q_TRAN(&Lab2A_stateH);
		}
		case OCT:  {
			return Q_TRAN(&Lab2A_stateO);
		}
		case SPEC:  {
			return Q_TRAN(&Lab2A_stateS);
		}
		case FFT:  {
			setFont(BigFont);
			setColor(255, 255, 255);
			snprintf(freq+12, 38, "%f", frequency);
			lcdPrint(freq+12, 30, 140);
			note[3] = '\0';
			o = get_octave(frequency);
			n = get_note(frequency, o);
			set_note(n, o, note);
			setFont(SuperBig);
			setColor(255, 255, 255);
			if(note[2] != ' '){
				lcdPrint(note, 72, 70);
				noteLen = 3;
			}else{
				if(noteLen == 3){
					setColor(0, 0, 0);
					fillRect(72, 70, 168, 134);
				}
				lcdPrint(note, 88, 70);
				noteLen = 2;
			}
			actual = a4freq*(pow(2.0, ((float)(n-9)/12 + (float)(o-4))));
			setColor(0, 0, 0);
			if(diff < 0){
				fillRect(20 + (100 - 2*cents), 170, 120, 180);
			}else{
				fillRect(120, 170, 120 + 2*cents, 180);
			}
			diff = frequency - actual;
			if(diff < 0){
				next = a4freq*(pow(2.0, ((float)(n-10)/12 + (float)(o-4))));
				cent = (next - actual)/100;
				cents = ((int)((diff/cent)+.5));
				if(cents < 0){
					cents = 0;
				}else if(cents > 50){
					cents = 50;
				}
				setColor(0, 0, 255);
				fillRect(20 + (100 - 2*cents), 170, 120, 180);
			}else{
				next = a4freq*(pow(2.0, ((float)(n-8)/12 + (float)(o-4))));
				cent = (next - actual)/100;
				cents = ((int)((diff/cent)+.5));
				if(cents < 0){
					cents = 0;
				}else if(cents > 50){
					cents = 50;
				}
				setColor(0, 0, 255);
				fillRect(120, 170, 120 + 2*cents, 180);
			}
			return Q_HANDLED();
		}
		case IDLE:  {
			setColor(0, 0, 0);
			fillRect(60, 250, 190, 265);
			return Q_HANDLED();
		}
	}

	return Q_SUPER(&Lab2A_on);

}

QState Lab2A_stateH(Lab2A *me) {
	switch (Q_SIG(me)) {
			case Q_ENTRY_SIG: {
				octave=-1;
				clrScr();
				setColor(255, 255, 255);
				setFont(BigFont);
				lcdPrint("FFT Histogram", 15, 10);
				drawHLine(56, 28, 128);
				drawHLine(56, 251, 128);
				draw_v_line(56, 251, 222);
				draw_v_line(184, 251, 222);
				return Q_HANDLED();
			}

			case ENCODER_UP: {
				return Q_HANDLED();
			}
			case ENCODER_DOWN: {
				return Q_HANDLED();
			}
			case ENCODER_CLICK:  {
				return Q_HANDLED();
			}
			case MENU:  {
				return Q_TRAN(&Lab2A_stateM);
			}
			case TUNER:  {
				return Q_TRAN(&Lab2A_stateT);
			}
			case HIST:  {
				return Q_HANDLED();
			}
			case OCT:  {
				return Q_TRAN(&Lab2A_stateO);
			}
			case SPEC:  {
				return Q_TRAN(&Lab2A_stateS);
			}
			case FFT:  {
				setColor(255, 255, 255);
				setFont(SmallFont);
				snprintf(freq+12, 38, "%f", frequency);
				lcdPrint(freq, 0, 260);
				snprintf(maxFreq+16, 38, "%f", sample_f/2);
				lcdPrint(maxFreq, 0, 280);
				snprintf(bin+14, 38, "%f", sample_f/(SAMPLES/D));
				lcdPrint(bin, 0, 300);
				if((SAMPLES/D) == 256){
					for(int i=1; i<128; i++){
						draw_bin(i,(int)(new_2[i]+.5));
					}
				}else if((SAMPLES/D) == 512){
					for(int i=1; i<256; i++){
						if(i%2 == 0){
							draw_bin(i/2,(int)(((new_2[i-1] + new_2[i])/2)+.5));
						}
					}
				}else{
					for(int i=1; i<512; i++){
						if(i%4 == 0){
							draw_bin(i/4,(int)(((new_2[i-3] + new_2[i-2] + new_2[i-1] + new_2[i])/4)+.5));
						}
					}
				}

				return Q_HANDLED();
			}
			case IDLE:  {
				return Q_HANDLED();
			}
		}

		return Q_SUPER(&Lab2A_on);
}

QState Lab2A_stateO(Lab2A *me) { //MUTED
	char num[1];
	switch (Q_SIG(me)) {
			case Q_ENTRY_SIG: {
				clrScr();
				setColor(255, 255, 255);
				setFont(BigFont);
				lcdPrint("Octave Select", 15, 10);
				octave = 2;
				o2 = 2;
				oct[8] = octave+'0';
				lcdPrint(oct, 45, 280);

				setColor(0, 0, 255);
				for(int i=2; i<8; i++){
					if(i == 3){
						setColor(255, 255, 255);
					}
					sprintf(num, "%d", i);
					lcdPrint(num, 5 + 30*(i-1), 300);
				}

				lcdPrint("-50", 0, 182);
				lcdPrint("0", 112, 182);
				lcdPrint("50", 200, 182);

				setColor(255, 255, 255);
				fillRect(19, 170, 19, 180);
				setColor(255, 255, 255);
				fillRect(221, 170, 221, 180);
				return Q_HANDLED();
			}

			case ENCODER_UP: {
				if(o2 < 7){
					setColor(255, 255, 255);
					setFont(BigFont);
					sprintf(num, "%d", o2);
					lcdPrint(num, 5 + 30*(o2-1), 300);

					o2++;
					setColor(0, 0, 255);
					sprintf(num, "%d", o2);
					lcdPrint(num, 5 + 30*(o2-1), 300);
				}
				return Q_HANDLED();
			}
			case ENCODER_DOWN: {
				if(o2 > 2){
					setColor(255, 255, 255);
					setFont(BigFont);
					sprintf(num, "%d", o2);
					lcdPrint(num, 5 + 30*(o2-1), 300);

					o2--;
					setColor(0, 0, 255);
					sprintf(num, "%d", o2);
					lcdPrint(num, 5 + 30*(o2-1), 300);
				}
				return Q_HANDLED();
			}
			case ENCODER_CLICK:  {
				octave = o2;
				setColor(255, 255, 255);
				setFont(BigFont);
				oct[8] = octave+'0';
				lcdPrint(oct, 45, 280);
				return Q_HANDLED();
			}
			case MENU:  {
				return Q_TRAN(&Lab2A_stateM);
			}
			case TUNER:  {
				return Q_TRAN(&Lab2A_stateT);
			}
			case HIST:  {
				return Q_TRAN(&Lab2A_stateH);
			}
			case OCT:  {
				return Q_HANDLED();
			}
			case SPEC:  {
				return Q_TRAN(&Lab2A_stateS);
			}
			case FFT:  {
				setFont(BigFont);
				setColor(255, 255, 255);
				snprintf(freq+12, 38, "%f", frequency);
				lcdPrint(freq+12, 30, 140);
				note[3] = '\0';
				o = get_octave(frequency);
				n = get_note(frequency, o);
				set_note(n, o, note);
				setFont(SuperBig);
				setColor(255, 255, 255);
				if(note[2] != ' '){
					lcdPrint(note, 72, 70);
					noteLen = 3;
				}else{
					if(noteLen == 3){
						setColor(0, 0, 0);
						fillRect(72, 70, 168, 134);
					}
					lcdPrint(note, 88, 70);
					noteLen = 2;
				}
				actual = a4freq*(pow(2.0, ((float)(n-9)/12 + (float)(o-4))));
				setColor(0, 0, 0);
				if(diff < 0){
					fillRect(20 + (100 - 2*((int)((diff/cent)+.5))), 170, 120, 180);
				}else{
					fillRect(120, 170, 120 + 2*((int)((diff/cent)+.5)), 180);
				}
				diff = frequency - actual;
				if(diff < 0){
					next = a4freq*(pow(2.0, ((float)(n-10)/12 + (float)(o-4))));
					cent = (next - actual)/100;
					setColor(0, 0, 255);
					fillRect(20 + (100 - 2*((int)((diff/cent)+.5))), 170, 120, 180);
				}else{
					next = a4freq*(pow(2.0, ((float)(n-8)/12 + (float)(o-4))));
					cent = (next - actual)/100;
					setColor(0, 0, 255);
					fillRect(120, 170, 120 + 2*((int)((diff/cent)+.5)), 180);
				}
				return Q_HANDLED();
			}
			case IDLE:  {
				return Q_HANDLED();
			}
		}

		return Q_SUPER(&Lab2A_on);
}

QState Lab2A_stateS(Lab2A *me) { //MUTED
	switch (Q_SIG(me)) {
			case Q_ENTRY_SIG: {
				clrScr();
				setColor(0, 0, 0);
				fillRect(0, 0, DISP_X_SIZE, DISP_Y_SIZE);

				setColor(255, 255, 255);
				setFont(BigFont);
				lcdPrint("Spectrogram", 20, 0);
				setFont(SmallFont);
				int xscale = 185;
				lcdPrint("-6000", xscale, 50);
				lcdPrint("-5000", xscale, 93);
				lcdPrint("-4000", xscale, 135);
				lcdPrint("-3000", xscale, 178);
				lcdPrint("-2000", xscale, 220);
				lcdPrint("-1000", xscale, 263);
				lcdPrint("-0", xscale, 305);

				return Q_HANDLED();
			}

			case ENCODER_UP: {
				return Q_HANDLED();
			}
			case ENCODER_DOWN: {
				return Q_HANDLED();
			}
			case ENCODER_CLICK:  {
				return Q_HANDLED();
			}
			case MENU:  {
				return Q_TRAN(&Lab2A_stateM);
			}
			case TUNER:  {
				return Q_TRAN(&Lab2A_stateT);
			}
			case HIST:  {
				return Q_TRAN(&Lab2A_stateH);
			}
			case OCT:  {
				return Q_TRAN(&Lab2A_stateO);
			}
			case SPEC:  {
				return Q_HANDLED();
			}
			case FFT:  {
				setColor(255, 255, 255);
				setFont(SmallFont);
				snprintf(freq+12, 38, "%f", frequency);
				lcdPrint(freq, 10, 30);
				for(int i=0; i<20; i++){
					for(int j=1; j<256; j++){
						if(i==19){
							  if(D == 4){//effective range 3000-6000Hz
								    spect[i][j] = (int)(((new_2[2*j-1] + new_2[2*j])/2)+.5);
							  }else if(D == 8){//effective range 1500-3000Hz
								  	if(j<128){
										spect[i][j] = (int)(((new_2[2*j-1] + new_2[2*j])/2)+.5);
									}else {
										spect[i][j] = 0;
									}
							  }else if(D == 16){//effective range 750-1500Hz
								    if(j<64){
								    	spect[i][j] = (int)(((new_2[4*j-3] + new_2[4*j-2] + new_2[4*j-1] + new_2[4*j])/4)+.5);
									}else {
										spect[i][j] = 0;
									}
							  }else if(D == 32){ //effective range 375-750Hz
								    if(j<32){
										spect[i][j] = (int)(((new_2[8*j-7] + new_2[8*j-6] + new_2[8*j-5] + new_2[8*j-4] + new_2[8*j-3] + new_2[8*j-2] + new_2[8*j-1] + new_2[8*j])/8)+.5);
									}else {
										spect[i][j] = 0;
									}
							  }else if(D == 64){ //effective range 187.5-375Hz
								  	if(j<16){
										spect[i][j] = (int)(((new_2[16*j-15] + new_2[16*j-14] + new_2[16*j-13] + new_2[16*j-12] + new_2[16*j-11] + new_2[16*j-10] + new_2[16*j-9] + new_2[16*j-8] + new_2[16*j-7] + new_2[16*j-6] + new_2[16*j-5] + new_2[16*j-4] + new_2[16*j-3] + new_2[16*j-2] + new_2[16*j-1] + new_2[16*j])/16)+.5);
									}else {
										spect[i][j] = 0;
									}
							  }else if(D == 128){//effective range 93.75-187.5Hz
								    if(j<8){
								    	spect[i][j] = (int)(((new_2[16*j-15] + new_2[16*j-14] + new_2[16*j-13] + new_2[16*j-12] + new_2[16*j-11] + new_2[16*j-10] + new_2[16*j-9] + new_2[16*j-8] + new_2[16*j-7] + new_2[16*j-6] + new_2[16*j-5] + new_2[16*j-4] + new_2[16*j-3] + new_2[16*j-2] + new_2[16*j-1] + new_2[16*j])/16)+.5);
									}else {
										spect[i][j] = 0;
									}
							  }
						}
						else{
							spect[i][j] = spect[i+1][j];
						}

						if((int)(spect[i][j]+.5)<1){ //black
							setColor(0, 0, 0);
						}
						else if((int)(spect[i][j]+.5)<5){ //purple
							setColor(225, 0, 225);

						}
						else if((int)(spect[i][j]+.5)<10){ //blue
							setColor(0, 0, 225);
						}
						else if((int)(spect[i][j]+.5)<25){ //green
							setColor(0, 225, 0);
						}
						else if((int)(spect[i][j]+.5)<50){ //yellow
							setColor(225, 225, 0);
						}
						else{ //red
							setColor(255, 0, 0);
						}
						fillRect(20+((i%120)*8), 310 - j%256, 20+8+((i%120)*8), 310 - (j%256+1));
					}
				}
				return Q_HANDLED();
			}
			case IDLE:  {
				return Q_HANDLED();
			}
		}

		return Q_SUPER(&Lab2A_on);
}


