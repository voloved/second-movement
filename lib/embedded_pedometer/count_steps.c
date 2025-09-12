#include "count_steps.h"
#include "stdint.h"
#include "stdio.h"  //using this for printing debug outputs
#include <stdlib.h>

//this algorithm is a simple adaptation of the following paper:
//"RecoFit - Using a Wearable Sensor to Find, Recognize, and Count Repetitive Exercises"

#define NUM_AUTOCORR_LAGS       50          //number of lags to calculate for autocorrelation. 50 lags @25Hz corresponds to a step rate of 0.5Hz...its probably not physically possible to walk much slower than this
#define DERIV_FILT_LEN          5           //length of derivative filter
#define LPF_FILT_LEN            9           //length of FIR low pass filter
#define AUTOCORR_DELTA_AMPLITUDE_THRESH 7e8 //this is the min delta between peak and trough of autocorrelation peak
#define AUTOCORR_MIN_HALF_LEN   3           //this is the min number of points the autocorrelation peak should be on either side of the peak

static int8_t deriv_coeffs[DERIV_FILT_LEN]        = {-6,31,0,-31,6};            //coefficients of derivative filter from https://www.dsprelated.com/showarticle/814.php
static int8_t lpf_coeffs[LPF_FILT_LEN]            = {-5,6,34,68,84,68,34,6,-5}; //coefficients of FIR low pass filter generated in matlab using FDATOOL
static uint8_t mag_sqrt[COUNT_STEPS_NUM_TUPLES]               = {0};                        //this holds the square root of magnitude data of X,Y,Z (so its length is NUM_SAMPLES/3)
static int32_t lpf[COUNT_STEPS_NUM_TUPLES]                    = {0};                        //hold the low pass filtered signal
static int64_t autocorr_buff[NUM_AUTOCORR_LAGS]   = {0};                        //holds the autocorrelation results
static int64_t deriv[NUM_AUTOCORR_LAGS]           = {0};                        //holds derivative

static void derivative(int64_t *autocorr_buff, int64_t *deriv);
static void autocorr(int32_t *lpf, int64_t *autocorr_buff);
static void remove_mean(int32_t *lpf);
static void lowpassfilt(uint8_t *mag_sqrt, int32_t *lpf);
static uint8_t get_precise_peakind(int64_t *autocorr_buff, uint8_t peak_ind);
static void get_autocorr_peak_stats(int64_t *autocorr_buff, uint8_t *neg_slope_count, int64_t *delta_amplitude_right, uint8_t *pos_slope_count, int64_t *delta_amplitude_left, uint8_t peak_ind);

/* Approximate l2 norm */
static uint32_t _approx_l2_norm(int8_t x, int8_t y, int8_t z)
{
    /* Absolute values */
    uint32_t ax = abs(x);
    uint32_t ay = abs(y);
    uint32_t az = abs(z);

    /* *INDENT-OFF* */
    /* Sort values: ax >= ay >= az */
    if (ax < ay) { uint32_t t = ax; ax = ay; ay = t; }
    if (ay < az) { uint32_t t = ay; ay = az; az = t; }
    if (ax < ay) { uint32_t t = ax; ax = ay; ay = t; }
    /* *INDENT-ON* */

    /* Approximate sqrt(x^2 + y^2 + z^2) */
    /* alpha ≈ 0.9375 (15/16), beta ≈ 0.375 (3/8) */
    return ax + ((15 * ay) >> 4) + ((3 * az) >> 3);
}


//take a look at the original autocorrelation signal at index i and see if
//its a real peak or if its just a fake "noisy" peak corresponding to
//non-walking. basically just count the number of points of the
//autocorrelation peak to the right and left of the peak. this function gets
//the number of points to the right and left of the peak, as well as the delta amplitude
static void get_autocorr_peak_stats(int64_t *autocorr_buff, uint8_t *neg_slope_count, int64_t *delta_amplitude_right, uint8_t *pos_slope_count, int64_t *delta_amplitude_left, uint8_t peak_ind) {
    
    //first look to the right of the peak. walk forward until the slope begins decreasing
    uint8_t  neg_slope_ind = peak_ind;
    uint16_t loop_limit    = NUM_AUTOCORR_LAGS-1;
    while ((autocorr_buff[neg_slope_ind+1] - autocorr_buff[neg_slope_ind] < 0) && (neg_slope_ind < loop_limit)) {
        *neg_slope_count = *neg_slope_count + 1;
        neg_slope_ind    = neg_slope_ind + 1;
    }
    
    //get the delta amplitude between peak and right trough
    *delta_amplitude_right = autocorr_buff[peak_ind] - autocorr_buff[neg_slope_ind];
    
    //next look to the left of the peak. walk backward until the slope begins increasing
    uint8_t pos_slope_ind = peak_ind;
    loop_limit    = 0;
    while ((autocorr_buff[pos_slope_ind] - autocorr_buff[pos_slope_ind-1] > 0) && (pos_slope_ind > loop_limit)) {
        *pos_slope_count = *pos_slope_count + 1;
        pos_slope_ind    = pos_slope_ind - 1;
    }
    
    //get the delta amplitude between the peak and the left trough
    *delta_amplitude_left = autocorr_buff[peak_ind] - autocorr_buff[pos_slope_ind];
}


//use the original autocorrelation signal to hone in on the
//exact peak index. this corresponds to the point where the points to the
//left and right are less than the current point
static uint8_t get_precise_peakind(int64_t *autocorr_buff, uint8_t peak_ind) {
    uint8_t loop_limit = 0;
    if ((autocorr_buff[peak_ind] > autocorr_buff[peak_ind-1]) && (autocorr_buff[peak_ind] > autocorr_buff[peak_ind+1])) {
        //peak_ind is perfectly set at the peak. nothing to do
    }
    else if ((autocorr_buff[peak_ind] > autocorr_buff[peak_ind+1]) && (autocorr_buff[peak_ind] < autocorr_buff[peak_ind-1])) {
        //peak is to the left. keep moving in that direction
        loop_limit = 0;
        while ((autocorr_buff[peak_ind] > autocorr_buff[peak_ind+1]) && (autocorr_buff[peak_ind] < autocorr_buff[peak_ind-1]) && (loop_limit < 10)) {
            peak_ind = peak_ind - 1;
            loop_limit++;
        }
    }
    else {
        //peak is to the right. keep moving in that direction
        loop_limit = 0;
        while ((autocorr_buff[peak_ind] > autocorr_buff[peak_ind-1]) && (autocorr_buff[peak_ind] < autocorr_buff[peak_ind+1]) && (loop_limit < 10)) {
            peak_ind = peak_ind + 1;
            loop_limit++;
        }
    }
    return peak_ind;
}


//calculate deriviative via FIR filter
static void derivative(int64_t *autocorr_buff, int64_t *deriv) {
    
    uint8_t n          = 0;
    uint8_t i          = 0;
    int64_t temp_deriv = 0;
    for (n = 0; n < NUM_AUTOCORR_LAGS; n++) {
        temp_deriv = 0;
        for (i = 0; i < DERIV_FILT_LEN; i++) {
            if (n-i >= 0) {     //handle the case when n < filter length
                temp_deriv += deriv_coeffs[i]*autocorr_buff[n-i];
            }
        }
        deriv[n] = temp_deriv;
    }
}


//autocorrelation function
//this takes a lot of computation. there are efficient implementations, but this is more intuitive
static void autocorr(int32_t *lpf, int64_t *autocorr_buff) {
    
    uint8_t lag;
    uint16_t i;
    int64_t temp_ac;
    for (lag = 0; lag < NUM_AUTOCORR_LAGS; lag++) {
        temp_ac = 0;
        for (i = 0; i < COUNT_STEPS_NUM_TUPLES-lag; i++) {
            temp_ac += (int64_t)lpf[i]*(int64_t)lpf[i+lag];
        }
        autocorr_buff[lag] = temp_ac;
    }
}


//calculate and remove the mean
static void remove_mean(int32_t *lpf) {
    
    int32_t sum = 0;
    uint16_t i;
    for (i = 0; i < COUNT_STEPS_NUM_TUPLES; i++) {
        sum += lpf[i];
    }
    sum = sum/(COUNT_STEPS_NUM_TUPLES);
    
    for (i = 0; i < COUNT_STEPS_NUM_TUPLES; i++) {
        lpf[i] = lpf[i] - sum;
    }
}


//FIR low pass filter
static void lowpassfilt(uint8_t *mag_sqrt, int32_t *lpf) {
    
    uint16_t n;
    uint8_t i;
    int32_t temp_lpf;
    for (n = 0; n < COUNT_STEPS_NUM_TUPLES; n++) {
        temp_lpf = 0;
        for (i = 0; i < LPF_FILT_LEN; i++) {
            if (n-i >= 0) {     //handle the case when n < filter length
                temp_lpf += (int32_t)lpf_coeffs[i]*(int32_t)mag_sqrt[n-i];
            }
        }
        lpf[n] = temp_lpf;
    }
}


//algorithm interface
uint8_t count_steps(int8_t *data) {
    
    //assume data is in the format data = [x1,y1,z1,x2,y2,z2...etc]
    uint16_t i;
    for (i = 0; i < COUNT_STEPS_NUM_TUPLES; i++) {
        mag_sqrt[i] = (uint8_t)_approx_l2_norm(data[i*3+0], data[i*3+1], data[i*3+2]);
    }
    
    //apply low pass filter to mag_sqrt, result is stored in lpf
    lowpassfilt(mag_sqrt, lpf);
    
    //remove mean from lpf, store result in lpf
    remove_mean(lpf);
    
    //do the autocorrelation on the lpf buffer, store the result in autocorr_buff
    autocorr(lpf, autocorr_buff);
    
    //get the derivative of the autocorr_buff, store in deriv
    derivative(autocorr_buff, deriv);
    
    //look for first zero crossing where derivative goes from positive to negative. that
    //corresponds to the first positive peak in the autocorrelation. look at two samples
    //instead of just one to maybe reduce the chances of getting tricked by noise
    uint8_t peak_ind = 0;
    //start index is set to 8 lags, which corresponds to a walking rate of 2.5Hz @20Hz sampling rate. its probably
    //running if its faster than this
    for (i = 8; i < NUM_AUTOCORR_LAGS; i++) {
        if ((deriv[i] > 0) && (deriv[i-1] > 0) && (deriv[i-2] < 0) && (deriv[i-3] < 0)) {
            peak_ind = i-1;
            break;
        }
    }
    
    //hone in on the exact peak index
    peak_ind = get_precise_peakind(autocorr_buff, peak_ind);
    //printf("peak ind: %i\n", peak_ind);
    
    //get autocorrelation peak stats
    uint8_t neg_slope_count = 0;
    int64_t delta_amplitude_right = 0;
    uint8_t pos_slope_count = 0;
    int64_t delta_amplitude_left = 0;
    get_autocorr_peak_stats(autocorr_buff, &neg_slope_count, &delta_amplitude_right, &pos_slope_count, &delta_amplitude_left, peak_ind);
    
    //now check the conditions to see if it was a real peak or not, and if so, calculate the number of steps
    uint8_t num_steps = 0;
    if ((pos_slope_count > AUTOCORR_MIN_HALF_LEN) && (neg_slope_count > AUTOCORR_MIN_HALF_LEN) && (delta_amplitude_right > AUTOCORR_DELTA_AMPLITUDE_THRESH) && (delta_amplitude_left > AUTOCORR_DELTA_AMPLITUDE_THRESH)) {
        //the period is peak_ind/COUNT_STEPS_SAMPLING_RATE seconds. that corresponds to a frequency of 1/period
        //with the frequency known, and the number of seconds is 4 seconds, you can then find out the number of steps
        num_steps = (COUNT_STEPS_SAMPLING_RATE*COUNT_STEPS_WINDOW_LENGTH)/peak_ind;
    } else {
        //not a valid autocorrelation peak
        num_steps = 0;
    }
    
    //printf("num steps: %i\n", num_steps);
    return num_steps;
}



/* Default settings */
#define DEFAULT_THRESHOLD 100
#define DEFAULT_MIN_STEP 10
#define DEFAULT_MAX_STEP 20

static step_counter_t state = {
    .threshold = DEFAULT_THRESHOLD,
    .min_step = DEFAULT_MIN_STEP,
    .max_step = DEFAULT_MAX_STEP
};

/* Print lis2dw status to console. */
static void _lis2dw_print_state(void)
{
    printf("LIS2DW status:\n");
    printf("  Power mode:\t%x\n", lis2dw_get_mode());
    printf("  Data rate:\t%x\n", lis2dw_get_data_rate());
    printf("  LP mode:\t%x\n", lis2dw_get_low_power_mode());
    printf("  BW filter:\t%x\n", lis2dw_get_bandwidth_filtering());
    printf("  Range:\t%x \n", lis2dw_get_range());
    printf("  Filter type:\t%x\n", lis2dw_get_filter_type());
    printf("  Low noise:\t%x\n", lis2dw_get_low_noise_mode());
    printf("\n");
}


/* Approximate l2 norm */
static uint32_t _approx_l2_norm_rieck(lis2dw_reading_t reading)
{
    /* Absolute values */
    uint32_t ax = abs(reading.x);
    uint32_t ay = abs(reading.y);
    uint32_t az = abs(reading.z);

    /* *INDENT-OFF* */
    /* Sort values: ax >= ay >= az */
    if (ax < ay) { uint32_t t = ax; ax = ay; ay = t; }
    if (ay < az) { uint32_t t = ay; ay = az; az = t; }
    if (ax < ay) { uint32_t t = ax; ax = ay; ay = t; }
    /* *INDENT-ON* */

    /* Approximate sqrt(x^2 + y^2 + z^2) */
    /* alpha ≈ 0.9375 (15/16), beta ≈ 0.375 (3/8) */
    return ax + ((15 * ay) >> 4) + ((3 * az) >> 3);
}


uint8_t count_steps_rieck(lis2dw_fifo_t *fifo)
{
    uint8_t steps = 0;
    for (uint8_t i = 0; i < fifo->count; i++) {
        /* Calculate magnitude of acceleration */
        uint32_t mag = _approx_l2_norm_rieck(fifo->readings[i]);
        /* Clamp magnitude to 16 bits and scale down to 8 bits */
        mag = (mag > 0xffff) ? 0xffff : mag;
        mag >>= 8;

        /* Check if we are crossing the threshold */
        if (mag > state.threshold) {
            /* Check if enough time has passed since last step */
            if (state.subticks - state.last_steps[0] < state.min_step) {
                printf("Step too short at %lu (%lu < %u)\n", state.subticks,
                       state.subticks - state.last_steps[0], state.min_step);
                goto skip;
            }

            steps++;
            printf("Step detected at %lu\n", state.subticks);
            printf("Total steps: %lu\n", state.steps);

            /* Shift step history: new step becomes most recent */
            state.last_steps[1] = state.last_steps[0];
            state.last_steps[0] = state.subticks;
        }

        /* Check if the current step is too long */
        if (state.subticks - state.last_steps[0] <= state.max_step)
            goto skip;

        /* Check if the previous step was too long */
        if (state.last_steps[0] - state.last_steps[1] <= state.max_step)
            goto skip;

        printf("Step too long at %lu (%lu > %u && %lu > %u)\n", state.last_steps[0],
               state.subticks - state.last_steps[0], state.max_step,
               state.last_steps[0] - state.last_steps[1], state.max_step);

        /* Remove current step */
        steps--;
        printf("Removing step at %lu\n", state.last_steps[0]);
        printf("Total steps: %lu\n", state.steps);
        state.last_steps[0] = state.last_steps[1];

      skip:
        /* Increment subticks */
        state.subticks += 1;
    }
    return steps;
}
