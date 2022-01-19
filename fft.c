#include "fft.h"
#include "complex.h"
#include "trig.h"

float new_2[1024] = {0};
static float new_2im[1024];

float fft_og(float* q, float* w, int n, int m, float sample_f) {
	int a,b,r,d,e,c;
	int k,place;
	a=n/2;
	b=1;
	int i,j;
	float real=0,imagine=0;
	float max,frequency;

	// Ordering algorithm
	for(i=0; i<(m-1); i++){
		d=0;
		for (j=0; j<b; j++){
			for (c=0; c<a; c++){
				e=c+d;
				new_2[e]=q[(c*2)+d];
				new_2im[e]=w[(c*2)+d];
				new_2[e+a]=q[2*c+1+d];
				new_2im[e+a]=w[2*c+1+d];
			}
			d+=(n/b);
		}
		for (r=0; r<n;r++){
			q[r]=new_2[r];
			w[r]=new_2im[r];
		}
		b*=2;
		a=n/(2*b);
	}
	//end ordering algorithm

	b=1;
	k=0;
	int z = 0;
	for (j=0; j<m; j++){
	//MATH
		for(i=0; i<n; i+=2){
			if (i%(n/b)==0 && i!=0)
				k++;
			if(n == 128){
				real=mult_real(q[i+1], w[i+1], cosTable[z], sinTable[z]);
				imagine=mult_im(q[i+1], w[i+1], cosTable[z], sinTable[z]);
			}else if(n == 256){
				real=mult_real(q[i+1], w[i+1], cosTable2[z], sinTable2[z]);
				imagine=mult_im(q[i+1], w[i+1], cosTable2[z], sinTable2[z]);
			}else if(n == 512){
				real=mult_real(q[i+1], w[i+1], cosTable3[z], sinTable3[z]);
				imagine=mult_im(q[i+1], w[i+1], cosTable3[z], sinTable3[z]);
			}else{
				real=mult_real(q[i+1], w[i+1], cosTable4[z], sinTable4[z]);
				imagine=mult_im(q[i+1], w[i+1], cosTable4[z], sinTable4[z]);
			}
//			real=mult_real(q[i+1], w[i+1], cosTable[z], sinTable[z]);
//			imagine=mult_im(q[i+1], w[i+1], cosTable[z], sinTable[z]);
			new_2[i]=q[i]+real;
			new_2im[i]=w[i]+imagine;
			new_2[i+1]=q[i]-real;
			new_2im[i+1]=w[i]-imagine;
			z++;
		}
		for (i=0; i<n; i++){
			q[i]=new_2[i];
			w[i]=new_2im[i];
		}
	//END MATH

	//REORDER
		for (i=0; i<n/2; i++){
			new_2[i]=q[2*i];
			new_2[i+(n/2)]=q[2*i+1];
			new_2im[i]=w[2*i];
			new_2im[i+(n/2)]=w[2*i+1];
		}
		for (i=0; i<n; i++){
			q[i]=new_2[i];
			w[i]=new_2im[i];
		}
	//END REORDER
		b*=2;
		k=0;
	}

	//find magnitudes
	max=0;
	place=1;
	for(i=1;i<(n/2);i++) {
		new_2[i]=q[i]*q[i]+w[i]*w[i];
		if(max < new_2[i]) {
			max=new_2[i];
			place=i;
		}
	}

	float s=sample_f/n; //spacing of bins

	frequency = (sample_f/n)*place;

	//curve fitting for more accuarcy
	//assumes parabolic shape and uses three point to find the shift in the parabola
	//using the equation y=A(x-x0)^2+C
	float y1=new_2[place-1],y2=new_2[place],y3=new_2[place+1];
	float x0=s+(2*s*(y2-y1))/(2*y2-y1-y3);
	x0=x0/s-1;

	if(x0 <0 || x0 > 2) { //error
		return 0;
	}
	if(x0 <= 1)  {
		frequency=frequency-(1-x0)*s;
	}
	else {
		frequency=frequency+(x0-1)*s;
	}

	return frequency;
}
