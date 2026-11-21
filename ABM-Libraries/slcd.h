#pragma once

#include "../gossamer/common/slcd.h"

#ifdef SLCD

enum slcd_frame_counter {
	/** SLCD frame counter 0 */
	SLCD_FRAME_COUNTER_0,
	/** SLCD frame counter 1 */
	SLCD_FRAME_COUNTER_1,
	/** SLCD frame counter 2 */
	SLCD_FRAME_COUNTER_2,
};

/**
 * \brief Disables automated character display
 *
 *  Disables automated character display.
 */
void slcd_disable_automated_character(void);
void slcd_enable_automated_bit(void);
void slcd_disable_automated_bit(void);
void slcd_set_automated_bit(uint8_t size,enum slcd_frame_counter fc);
void slcd_disable_frame_counter(enum slcd_frame_counter fc);
void slcd_set_frame_counter(enum slcd_frame_counter fc,
										   bool  presc_bypass_enable,
										   uint16_t overflow_value);
void slcd_enable_frame_counter(enum slcd_frame_counter fc);


/**
 * \brief Determines if SLCD module is currently synchronizing to the bus
 *
 * Checks to see if the underlying hardware peripheral module(s) are currently
 * synchronizing across multiple clock domains to the hardware bus, This
 * function can be used to delay further operations on a module until such time
 * that it is ready, to prevent blocking delays for synchronization in the
 * user application.
 *
 * \return Synchronization status of the underlying hardware module.
 *
 * \retval true  If the module synchronization is ongoing
 * \retval false If the module has completed synchronization
 */
static inline bool slcd_is_syncing(void)
{

	if (SLCD->SYNCBUSY.reg) {
		return true;
	}

	return false;
}


#endif // SLCD

/** @} */