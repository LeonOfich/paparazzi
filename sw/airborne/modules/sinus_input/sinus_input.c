/*
 * Copyright (C) LeonSijbers
 *
 * This file is part of paparazzi
 *
 */
/**
 * @file "modules/sinus_input/sinus_input.c"
 * @author LeonSijbers
 * Creates atrust sinus input of varying frequnecy
 */

#include "modules/sinus_input/sinus_input.h"
#include "stdio.h"
#include "state.h"
#include "generated/airframe.h"
// void init_sinus_input() {}
// void periodic_sinus_input() {}


bool 	sinus_input_flag;
float 	temp;
int8_t 	step_num;
float 	atrust;
float	freq; 
float 	magnitude;

void init_sinus_input(void){
	step_num = 0;
 	sinus_input_flag = false;
 	temp = 0;
}

bool sinus_input_status(void){
 	return sinus_input_flag;
}

void periodic_sinus_input(void){
	if (step_num <= 0){
		sinus_input_flag = true;
	}
	temp += 1.0/PERIODIC_FREQUENCY;
	//printf("%f\n", temp);
	magnitude = 2000;
     if (temp >= 0.350 && temp < 10.35){freq = 0.10; atrust = 6000 + magnitude*sin(temp*freq*2*M_PI); printf("%f\t%f\t%f\n",temp, freq, atrust);}
else if (temp >= 10.70 && temp < 17.20){freq = 0.23; atrust = 6000 + magnitude*sin(temp*freq*2*M_PI); printf("%f\t%f\t%f\n",temp, freq, atrust);}
else if (temp >= 17.55 && temp < 22.55){freq = 0.40; atrust = 6000 + magnitude*sin(temp*freq*2*M_PI); printf("%f\t%f\t%f\n",temp, freq, atrust);}
else if (temp >= 22.90 && temp < 27.70){freq = 0.62; atrust = 6000 + magnitude*sin(temp*freq*2*M_PI); printf("%f\t%f\t%f\n",temp, freq, atrust);}
else if (temp >= 28.05 && temp < 31.35){freq = 0.90; atrust = 6000 + magnitude*sin(temp*freq*2*M_PI); printf("%f\t%f\t%f\n",temp, freq, atrust);}
else if (temp >= 31.70 && temp < 34.80){freq = 1.28; atrust = 6000 + magnitude*sin(temp*freq*2*M_PI); printf("%f\t%f\t%f\n",temp, freq, atrust);}
else if (temp >= 35.15 && temp < 37.45){freq = 1.76; atrust = 6000 + magnitude*sin(temp*freq*2*M_PI); printf("%f\t%f\t%f\n",temp, freq, atrust);}
else if (temp >= 37.80 && temp < 39.90){freq = 2.39; atrust = 6000 + magnitude*sin(temp*freq*2*M_PI); printf("%f\t%f\t%f\n",temp, freq, atrust);}
else if (temp >= 40.25 && temp < 41.85){freq = 3.20; atrust = 6000 + magnitude*sin(temp*freq*2*M_PI); printf("%f\t%f\t%f\n",temp, freq, atrust);}
else if (temp >= 42.20 && temp < 43.60){freq = 4.26; atrust = 6000 + magnitude*sin(temp*freq*2*M_PI); printf("%f\t%f\t%f\n",temp, freq, atrust);}
else if (temp >= 43.95 && temp < 45.05){freq = 5.64; atrust = 6000 + magnitude*sin(temp*freq*2*M_PI); printf("%f\t%f\t%f\n",temp, freq, atrust);}
else if (temp >= 45.40 && temp < 46.30){freq = 7.43; atrust = 6000 + magnitude*sin(temp*freq*2*M_PI); printf("%f\t%f\t%f\n",temp, freq, atrust);}
else if (temp >= 46.65 && temp < 47.35){freq = 9.76; atrust = 6000 + magnitude*sin(temp*freq*2*M_PI); printf("%f\t%f\t%f\n",temp, freq, atrust);}
else if (temp >= 47.70 && temp < 48.30){freq = 12.79; atrust = 6000 + magnitude*sin(temp*freq*2*M_PI); printf("%f\t%f\t%f\n",temp, freq, atrust);}
else if (temp >= 48.65 && temp < 49.15){freq = 16.73; atrust = 6000 + magnitude*sin(temp*freq*2*M_PI); printf("%f\t%f\t%f\n",temp, freq, atrust);}
else if (temp >= 49.50 && temp < 49.90){freq = 21.85; atrust = 6000 + magnitude*sin(temp*freq*2*M_PI); printf("%f\t%f\t%f\n",temp, freq, atrust);}
else if (temp >= 50.25 && temp < 50.55){freq = 28.50; atrust = 6000 + magnitude*sin(temp*freq*2*M_PI); printf("%f\t%f\t%f\n",temp, freq, atrust);}
else if (temp >= 75 && temp < 100){
	atrust = 6000;
	printf("%f\t%f\t%f\n",temp, freq, atrust);
}
	else if (temp > 100.1){
		step_num++;
		temp = 0;
		sinus_input_flag = false;
		//init_sinus_input();		
	}
	else {
	atrust = 6000;
	magnitude = 2000;
	freq = 0;
		printf("%f\t%f\t%f\n",temp, freq, atrust);
	}
	}

