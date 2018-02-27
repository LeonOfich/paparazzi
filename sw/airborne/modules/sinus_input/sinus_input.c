/*
 * Copyright (C) LeonSijbers
 *
 * This file is part of paparazzi
 *
 */
/**
 * @file "modules/sinus_input/sinus_input.c"
 * @author LeonSijbers
 * Creates a sinus input of varying frequnecy
 */

#include "modules/sinus_input/sinus_input.h"
#include "stdio.h"
#include "state.h"
#include "generated/airframe.h"
// void init_sinus_input() {}
// void periodic_sinus_input() {}


extern void init_sinus_input(void);
extern void sinus_input_status(void);
extern void periodic_sinus_input(void);

bool 	sinus_input_flag;
float 	temp;
int8_t 	step_num;
float 	a;
float	freq; 

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
	a = 0;
	freq = 0;
	if (temp > 0.1 && temp >= 3){
		a = 6000 + sin(freq*2*pi)
	}
	else if (temp > 3.1 && temp >= 6.1){
		a = 6000 + sin(freq*2*pi)
	}
	else if (temp<5.1){
		a = 0;
	}
	else
	{
		a = 0;
		step_num++;
		temp = 0;
		sinus_input_flag = false;
		//init_step_input();
	}
	}
