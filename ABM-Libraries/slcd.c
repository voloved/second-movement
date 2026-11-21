#include "../gossamer/peripherals/slcd.c"

#ifdef SLCD

/**
 * \brief Disables automated character display
 *
 *  Disables automated character display.
 */
void slcd_disable_automated_character(void)
{
	SLCD->CTRLC.reg &= ~SLCD_CTRLC_ACMEN;
}

/**
 * \name Automated Bit Mapping Functions
 * @{
 */

/**
 * \brief Enables automated bit display
 *
 *  Enables automated bit display.
 */
void slcd_enable_automated_bit(void)
{
	SLCD->CTRLC.reg |= SLCD_CTRLC_ABMEN;
}

/**
 * \brief Disables automated bit display
 *
 *  Disables automated bit display.
 */
void slcd_disable_automated_bit(void)
{
	SLCD->CTRLC.reg &= ~SLCD_CTRLC_ABMEN;
}

/**
 * \brief Sets automated bit display
 *
 *  Sets automated bit display.
 *
 *  \note Automated bit cannot be set while it is enabled or busy.
 */
void slcd_set_automated_bit(uint8_t size,enum slcd_frame_counter fc)
{
	if(size > 0 && size < 0x3f) {
		SLCD->ABMCFG.reg = SLCD_ABMCFG_SIZE(size) | SLCD_ABMCFG_FCS(fc);
	}
}

/** @} */

/**
 * \brief Disable a frame counter.
 *
 *  Disable one frame counter.
 *
 *	\param[in] fc Frame counter index
 */
void slcd_disable_frame_counter(enum slcd_frame_counter fc)
{
	switch(fc) {
		case SLCD_FRAME_COUNTER_0:
			SLCD->CTRLD.reg &= ~SLCD_CTRLD_FC0EN;
			break;
		case SLCD_FRAME_COUNTER_1:
			SLCD->CTRLD.reg &= ~SLCD_CTRLD_FC1EN;
			break;
		case SLCD_FRAME_COUNTER_2:
			SLCD->CTRLD.reg &= ~SLCD_CTRLD_FC2EN;
			break;
		default :
			break;
	}
	while (slcd_is_syncing()) {
		/* Wait for synchronization */
	}
}
/** @} */

/**
 * \brief Frame counter configuration
 *
 *  Config frame counter.
 *
 *  \note Frame counter cannot be set while it is enabled.
 *
 *  \param[in] fc Frame counter index
 *  \param[in] presc_bypass_enable Bypass of the frame counter prescaler
 *  \param[in] overflow_value  Frame counter overflow value. The number of frame
 *		before overflow is ((overflow_value+1)*8) when presc_bypass_enable=0
 		else (overflow_value+1). The MAX. overflow value is 0x1FFFF.
 */
void slcd_set_frame_counter(enum slcd_frame_counter fc,
										   bool  presc_bypass_enable,
										   uint16_t overflow_value)
{
	*(&(SLCD->FC0.reg) + fc) =  (presc_bypass_enable << SLCD_FC0_PB_Pos)
							 | SLCD_FC0_OVF(overflow_value);
}

/**
 * \brief Enables a frame counter
 *
 *  Enables one frame counter.
 *
 *	\param[in] fc Frame counter index
 */
void slcd_enable_frame_counter(enum slcd_frame_counter fc)
{

	switch(fc) {
		case SLCD_FRAME_COUNTER_0:
			SLCD->CTRLD.reg |= SLCD_CTRLD_FC0EN;
			break;
		case SLCD_FRAME_COUNTER_1:
			SLCD->CTRLD.reg |= SLCD_CTRLD_FC1EN;
			break;
		case SLCD_FRAME_COUNTER_2:
			SLCD->CTRLD.reg |= SLCD_CTRLD_FC2EN;
			break;
		default :
			break;
	}
	while (slcd_is_syncing()) {
		/* Wait for synchronization */
	}
}


#endif // SLCD