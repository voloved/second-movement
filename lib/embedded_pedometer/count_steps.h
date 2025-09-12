#ifndef count_steps_h
#define count_steps_h
#include "stdint.h"

#define COUNT_STEPS_SAMPLING_RATE    25                                               //25 hz sampling rate
#define COUNT_STEPS_NUM_TUPLES       100                                              //80 sets of accelerometer readings (so in other words, 100*3 = 300 samples)
#define COUNT_STEPS_WINDOW_LENGTH    COUNT_STEPS_NUM_TUPLES/COUNT_STEPS_SAMPLING_RATE //window length in seconds

uint8_t count_steps(int8_t *data);

#include "lis2dw.h"

typedef struct {
    /* Step detection data */
    uint32_t steps;             /* Number of steps taken */
    uint32_t subticks;          /* Subticks for timing */
    uint32_t last_steps[2];     /* History of last two steps */

    /* Parameters for step detection */
    uint16_t threshold;         /* Threshold for step detection */
    uint8_t min_step;           /* Minimum time between steps */
    uint8_t max_step;           /* Maximum time between steps */
} step_counter_t;

uint8_t count_steps_rieck(lis2dw_fifo_t *fifo);

#endif /* count_steps_h */