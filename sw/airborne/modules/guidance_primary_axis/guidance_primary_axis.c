/*
 * Copyright (C) Sihao Sun
 *
 * This file is part of paparazzi
 *
 * paparazzi is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * paparazzi is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with paparazzi; see the file COPYING.  If not, see
 * <http://www.gnu.org/licenses/>.
 */
/**
 * @file "modules/guidance_primary_axis/guidance_primary_axis.c"
 * @author Sihao Sun
 * guidance using primary axis instead of euler angles
 */

#include "firmwares/rotorcraft/stabilization/stabilization_attitude_rc_setpoint.h"
#include "modules/guidance_primary_axis/guidance_primary_axis.h"
#include "firmwares/rotorcraft/stabilization/stabilization_indi.h"
#include "generated/airframe.h"
#include "subsystems/ins/ins_int.h"
#include "subsystems/radio_control.h"
#include "state.h"
#include "subsystems/imu.h"
#include "firmwares/rotorcraft/guidance/guidance_h.h"
#include "firmwares/rotorcraft/guidance/guidance_v.h"
#include "firmwares/rotorcraft/stabilization/stabilization_attitude.h"
#include "firmwares/rotorcraft/autopilot_rc_helpers.h"
#include "mcu_periph/sys_time.h"
#include "autopilot.h"
#include "stabilization/stabilization_attitude_ref_quat_int.h"
#include "firmwares/rotorcraft/stabilization.h"
#include "stdio.h"
#include "filters/low_pass_filter.h"
#include "subsystems/abi.h"
#include "math/pprz_algebra_int.h"
#include "math/pprz_algebra_float.h"
#include "modules/attitude_optitrack/attitude_optitrack.h"
#include "modules/sliding_mode_observer/sliding_mode_observer.h"
#include "modules/step_input/step_input.h"

#ifdef GUIDANCE_PA_POS_GAIN
float guidance_pa_pos_gain = GUIDANCE_PA_POS_GAIN;
#else
float guidance_pa_pos_gain = 1.0;
#endif

#ifdef GUIDANCE_PA_SPEED_GAIN
float guidance_pa_speed_gain = GUIDANCE_INDI_SPEED_GAIN;
#else
float guidance_pa_speed_gain = 2.0;
#endif

#ifdef GUIDANCE_PA_ATT_GAIN 
float guidance_pa_att_gain = GUIDANCE_PA_ATT_GAIN
#else
float guidance_pa_att_gain = -10.0;
#endif

struct FloatVect3 n_pa = {0.0,0.0,-1.0};
struct FloatVect3 nd_i_state_dot_b = {0.0,0.0,0.0};
struct FloatVect3 nd_i_state_dot_i = {0.0,0.0,0.0};
struct FirstOrderLowPass nd_i_state_x, nd_i_state_y, nd_i_state_z;
struct FirstOrderLowPass theta_filt, psi_des_filt;
//struct FirstOrderLowPass sp_accel_filter_x;
//struct FirstOrderLowPass sp_accel_filter_y;
//struct FirstOrderLowPass sp_accel_filter_z;
Butterworth2LowPass sp_accel_filter_x;
Butterworth2LowPass sp_accel_filter_y;
Butterworth2LowPass sp_accel_filter_z;

bool guidance_primary_axis_status(void)
{
	return primary_axis_status;
}

void guidance_primary_axis_init(void)
{
	primary_axis_status = 0;
	low_pass_filter_init();
	primary_axis_n_gain_x = PRIMARY_AXIS_GUIDANCE_NX_GAIN;
	primary_axis_n_gain_y = PRIMARY_AXIS_GUIDANCE_NY_GAIN;
    primary_axis_n_abs    = PRIMARY_AXIS_GUIDANCE_ABS;
	return;
}

void low_pass_filter_init(void)
{	
	float tau = 1.0 / (2.0 * M_PI * 8.0);
	float tau_nd = 1.0/(2.0 * M_PI * 2.5);
	float sample_time = 1.0 / PERIODIC_FREQUENCY;
	init_first_order_low_pass(&nd_i_state_x,tau_nd,sample_time,0);
	init_first_order_low_pass(&nd_i_state_y,tau_nd,sample_time,0);
	init_first_order_low_pass(&nd_i_state_z,tau_nd,sample_time,-1);
	init_first_order_low_pass(&theta_filt,tau,sample_time,stateGetNedToBodyEulers_f()->theta);
	init_first_order_low_pass(&psi_des_filt,tau,sample_time,guidance_h.sp.heading);
	init_butterworth_2_low_pass(&sp_accel_filter_x,tau_nd,sample_time,0);
	init_butterworth_2_low_pass(&sp_accel_filter_y,tau_nd,sample_time,0);
	init_butterworth_2_low_pass(&sp_accel_filter_z,tau_nd,sample_time,0);	
	return;
}

void guidance_primary_axis_end(void)
{
	primary_axis_status = 0;
	return;
}

void guidance_primary_axis_run(void)
{
	//Necessary parameters
	float g = 9.8125;
	n_pa.x = primary_axis_n_abs * primary_axis_n_gain_x;
	//n_pa.y =  - primary_axis_n_abs * primary_axis_n_gain_y;
	n_pa.y =  primary_axis_n_abs * primary_axis_n_gain_y;

	//Flag to hack guidance loop
	primary_axis_status = 1;

	//Linear controller to find the acceleration setpoint rate_cmd_primary_axis position and velocity
	float pos_x_err = POS_FLOAT_OF_BFP(guidance_h.ref.pos.x) - stateGetPositionNed_f()->x;
	float pos_y_err = POS_FLOAT_OF_BFP(guidance_h.ref.pos.y) - stateGetPositionNed_f()->y;
	float pos_z_err = POS_FLOAT_OF_BFP(guidance_v_z_ref - stateGetPositionNed_i()->z);

	//printf("%d\t%d\t%f\n", guidance_v_z_ref, stateGetPositionNed_i()->z,stateGetPositionEnu_f()->z);
	//printf("%d\t%f\t%d\t%f\n",guidance_h.ref.pos.x, stateGetPositionNed_f()->x, guidance_h.ref.pos.y, stateGetPositionNed_f()->y);
	
	float speed_sp_x = pos_x_err * guidance_pa_pos_gain;
	float speed_sp_y = pos_y_err * guidance_pa_pos_gain;
	float speed_sp_z = pos_z_err * guidance_pa_pos_gain*0.5;

	struct FloatVect3 sp_accel = {0.0,0.0,0.0};
	struct FloatVect3 sp_accel_raw = {0.0,0.0,0.0};

	sp_accel_raw.x = (speed_sp_x - stateGetSpeedNed_f()->x) * guidance_pa_speed_gain;
	sp_accel_raw.y = (speed_sp_y - stateGetSpeedNed_f()->y) * guidance_pa_speed_gain;
	sp_accel_raw.z = (speed_sp_z - stateGetSpeedNed_f()->z) * guidance_pa_speed_gain;

	sp_accel.x = update_butterworth_2_low_pass(&sp_accel_filter_x, sp_accel_raw.x);
	sp_accel.y = update_butterworth_2_low_pass(&sp_accel_filter_y, sp_accel_raw.y);
	sp_accel.z = update_butterworth_2_low_pass(&sp_accel_filter_z, sp_accel_raw.z);

	sp_accel_primary_axis.x = sp_accel_raw.x;
	sp_accel_primary_axis.y = sp_accel_raw.y;
	sp_accel_primary_axis.z = sp_accel_raw.z;
	sp_accel_primary_axis_filter.x = sp_accel.x;
	sp_accel_primary_axis_filter.y = sp_accel.y;
	sp_accel_primary_axis_filter.z = sp_accel.z;	
	float phi,theta,psi;

	if(attitude_optitrack_status()==true){
		phi 	= attitude_optitrack.phi;
	  	theta = attitude_optitrack.theta;
	  	psi 	= attitude_optitrack.psi;

	}else {
		phi 	= stateGetNedToBodyEulers_f()->phi;
		theta = stateGetNedToBodyEulers_f()->theta;
		psi 	= stateGetNedToBodyEulers_f()->psi;	
	}
	
	float r, p_des, q_des, r_des;

//#if GUIDANCE_PA_RC_DEBUG
//#warning "GUIDANCE_PARC_DEBUG lets you control the accelerations via RC, but disables autonomous flight!"
  //for rc control horizontal, rotate from body axes to NED
if ((autopilot.mode == AP_MODE_ATTITUDE_DIRECT) || (autopilot.mode == AP_MODE_ATTITUDE_Z_HOLD)) {
  	float rc_x = -(radio_control.values[RADIO_PITCH]/9600.0)*10.0;
  	float rc_y = (radio_control.values[RADIO_ROLL]/9600.0)*10.0;
//  	sp_accel.x = cosf(psi) * rc_x - sinf(psi) * rc_y;
//  	sp_accel.y = sinf(psi) * rc_x + cosf(psi) * rc_y;
//  	sp_accel.x = rc_x;
//  	sp_accel.y = rc_y;
  	sp_accel.x = cosf(-27/57.3) * rc_x - sinf(-27/57.3) * rc_y;
  	sp_accel.y = sinf(-27/57.3) * rc_x + cosf(-27/57.3) * rc_y; 	

    float rd = radio_control.values[RADIO_YAW];
    DeadBand(rd, STABILIZATION_ATTITUDE_DEADBAND_R);
    r_des =  rd * STABILIZATION_ATTITUDE_SP_MAX_R / (MAX_PPRZ - STABILIZATION_ATTITUDE_DEADBAND_R);

    if (autopilot.mode == AP_MODE_ATTITUDE_DIRECT)
    {
	  //for rc vertical control
	  	sp_accel.z = -(radio_control.values[RADIO_THROTTLE]-4500)*16.0/9600.0;
    }

//printf("%f\n", sp_accel.z);
}
else if (autopilot.mode == AP_MODE_NAV){
	float psi_des = guidance_h.sp.heading;

	// Compute command r (check availability);
	float theta_filt_last = get_first_order_low_pass(&theta_filt);
	float psi_des_filt_last = get_first_order_low_pass(&psi_des_filt);
	update_first_order_low_pass(&theta_filt,theta);
	update_first_order_low_pass(&psi_des_filt,psi_des);
	float theta_dot = (get_first_order_low_pass(&theta_filt)-theta_filt_last)*PERIODIC_FREQUENCY;
	float psi_des_dot = (get_first_order_low_pass(&psi_des_filt)-psi_des_filt_last)*PERIODIC_FREQUENCY;

	float psi_dot_cmd = psi_des_dot + 5.0*(psi_des-psi);

	r_des = psi_dot_cmd*cos(phi)*cos(theta)-sin(phi)*theta_dot;
}
	

	//Acceleration projecting on body axis (nd_i)
	nd_i_state.x = sp_accel.x;
	nd_i_state.y = sp_accel.y;
	nd_i_state.z = sp_accel.z<0 ? sp_accel.z-g : -g;

	float norm_nd = sqrtf(nd_i_state.x*nd_i_state.x+nd_i_state.y*nd_i_state.y+nd_i_state.z*nd_i_state.z);
	nd_i_state.x =  nd_i_state.x/norm_nd;
	nd_i_state.y =  nd_i_state.y/norm_nd; 	
	nd_i_state.z =  nd_i_state.z/norm_nd;

/*	//psi = -27.0/57.3;
	if(step_input_status() == true && (autopilot.mode == AP_MODE_ATTITUDE_Z_HOLD || autopilot.mode == AP_MODE_NAV))
	{
		call_step_input(&nx_desire_step, 0.342, 0.1, 0.6, 1.2); //30deg
		//call_step_input(&y_desire, 0.342, 2.1, 2.6, 3.2);
		nd_i_state.x = nx_desire_step;
		nd_i_state.y = y_desire;
		nd_i_state.z = -sqrtf(1.0-x_desire*x_desire);
		psi = 0.0;
	}	*/

	struct FloatMat33 R_BI;
	MAT33_ELMT(R_BI,0,0) = cos(theta)*cos(psi);
	MAT33_ELMT(R_BI,0,1) = cos(theta)*sin(psi);
	MAT33_ELMT(R_BI,0,2) = -sin(theta);
	MAT33_ELMT(R_BI,1,0) = sin(phi)*sin(theta)*cos(psi)-cos(phi)*sin(psi);
	MAT33_ELMT(R_BI,1,1) = sin(phi)*sin(theta)*sin(psi)+cos(phi)*cos(psi);
	MAT33_ELMT(R_BI,1,2) = sin(phi)*cos(theta);
	MAT33_ELMT(R_BI,2,0) = cos(phi)*sin(theta)*cos(psi)+sin(phi)*sin(psi);
	MAT33_ELMT(R_BI,2,1) = cos(phi)*sin(theta)*sin(psi)-sin(phi)*cos(psi);
	MAT33_ELMT(R_BI,2,2) = cos(phi)*cos(theta);

	MAT33_VECT3_MUL(nd_state, R_BI, nd_i_state);


	//Calculate command thrust
	float thrust_specific;
	thrust_specific = -(sp_accel.z-g)/cos(phi)/cos(theta);

	//printf("%f\t%f\t%f\n", speed_sp_z, stateGetSpeedNed_f()->z,thrust_specific);

	//Compute command p and q using NDI
	struct FloatRates *body_rates = stateGetBodyRates_f();
  	if (attitude_optitrack_status() == false)
    	r = body_rates->r; 
  	else{
    	if (fabs(body_rates->r) < 35) // gyroscope limitation on Bebop2, +-2000deg/sec
      		r = body_rates->r;
    	else
      		r = angular_rate_optitrack.r;
  	}
	float nd_i_state_x_last = get_first_order_low_pass(&nd_i_state_x);
	float nd_i_state_y_last = get_first_order_low_pass(&nd_i_state_y);
	float nd_i_state_z_last = get_first_order_low_pass(&nd_i_state_z);	
	update_first_order_low_pass(&nd_i_state_x,nd_i_state.x);
	update_first_order_low_pass(&nd_i_state_y,nd_i_state.y);
	update_first_order_low_pass(&nd_i_state_z,nd_i_state.z);	
	nd_i_state_dot_i.x = (get_first_order_low_pass(&nd_i_state_x)-nd_i_state_x_last)*PERIODIC_FREQUENCY;
	nd_i_state_dot_i.y = (get_first_order_low_pass(&nd_i_state_y)-nd_i_state_y_last)*PERIODIC_FREQUENCY;
	nd_i_state_dot_i.z = (get_first_order_low_pass(&nd_i_state_z)-nd_i_state_z_last)*PERIODIC_FREQUENCY;
	
	MAT33_VECT3_MUL(nd_i_state_dot_b, R_BI, nd_i_state_dot_i);
	p_des =  1.0/nd_state.z*(guidance_pa_att_gain*(nd_state.y-n_pa.y)+nd_state.x*r - 0.0*nd_i_state_dot_b.y);
	q_des = -1.0/nd_state.z*(guidance_pa_att_gain*(nd_state.x-n_pa.x)-nd_state.y*r - 0.0*nd_i_state_dot_b.x);

	//Angular rate command from primay axis guidance
	rate_cmd_primary_axis[0] = p_des;
	rate_cmd_primary_axis[1] = q_des;
	rate_cmd_primary_axis[2] = r_des;
	thrust_primary_axis = thrust_specific;

	//printf("%f\t%f\n",nd_state.x,nd_state.y);
	return;
}

void primary_axis_status_take_off(void)
{

}