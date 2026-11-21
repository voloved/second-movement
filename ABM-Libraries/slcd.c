/*
 * MIT License
 *
 * Copyright (c) 2022 Joey Castillo
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "slcd.h"

#ifdef SLCD

static void _slcd_sync(uint32_t reg) {
    // wait for sync of whichever flags were passed in.
    while (SLCD->SYNCBUSY.reg & reg);
}

void slcd_init(uint64_t lcd_pins, slcd_bias_value_t bias, slcd_duty_value_t duty, slcd_clocksource_value_t clocksource, slcd_prescaler_value_t prescaler, slcd_clockdiv_value_t clkdiv, slcd_xvlcd_value_t use_xvlcd) {
    // select the correct clock for the SLCD peripheral
    OSC32KCTRL->SLCDCTRL.bit.SLCDSEL = clocksource & 1;
    OSC32KCTRL->XOSC32K.bit.RUNSTDBY = 1;

    // Enable the clocks for the SLCD peripheral
    MCLK->APBCMASK.reg |= MCLK_APBCMASK_SLCD;
    if (SLCD->CTRLA.bit.ENABLE) {
        SLCD->CTRLA.bit.ENABLE = 0;
        _slcd_sync(SLCD_SYNCBUSY_ENABLE);
    }
    SLCD->CTRLA.bit.SWRST = 1;
    _slcd_sync(SLCD_SYNCBUSY_SWRST);

    // Tell the LCD driver which pins are connected to the LCD
    SLCD->LPENL.reg = (uint32_t)lcd_pins;
    SLCD->LPENH.reg = (uint32_t)(lcd_pins >> 32);

    // set LCD control registers
    SLCD->CTRLA.reg = SLCD_CTRLA_RUNSTDBY |         // run in standby
                      SLCD_CTRLA_WMOD_LP |          // low power waveform
                      SLCD_CTRLA_BIAS(bias) |       // User defined bias
                      SLCD_CTRLA_DUTY(duty) |       // User defined duty cycle
                      SLCD_CTRLA_XVLCD(use_xvlcd) | // Use external voltage for LCD
                      SLCD_CTRLA_PRESC(prescaler) | // User defined prescaler
                      SLCD_CTRLA_CKDIV(clkdiv) |    // User defined clock divider
                      SLCD_CTRLA_PRF(3) |           // power refresh slowly, 250 Hz
                      SLCD_CTRLA_RRF(5);            // reference refresh slowly, 62.5 Hz
    SLCD->CTRLB.reg = SLCD_CTRLB_BBEN |             // enable bias buffer
                      SLCD_CTRLB_BBD(1);            // configure bias buffer
    SLCD->CTRLD.reg = SLCD_CTRLD_DISPEN;            // enable the COM/SEG waveform outputs
    _slcd_sync(SLCD_SYNCBUSY_CTRLD);
}

void slcd_set_contrast(uint8_t contrast) {
    SLCD->CTRLC.reg = SLCD_CTRLC_CTST(contrast);
}

void slcd_enable(void) {
    SLCD->CTRLA.bit.ENABLE = 1;
    _slcd_sync(SLCD_SYNCBUSY_ENABLE);
}

bool slcd_is_enabled(void) {
    return SLCD->CTRLA.bit.ENABLE;
}

void slcd_clear(void) {
    /// TODO: Can we optimize this? 
    /// If under 32 segments, we can write 0 to just the SDATAL registers.
    /// For only 3 or 4 COMs, we can skip higher numbered registers.
    SLCD->SDATAL0.reg = 0;
    SLCD->SDATAL1.reg = 0;
    SLCD->SDATAL2.reg = 0;
    SLCD->SDATAL3.reg = 0;
    SLCD->SDATAL4.reg = 0;
    SLCD->SDATAL5.reg = 0;
    SLCD->SDATAL6.reg = 0;
    SLCD->SDATAL7.reg = 0;
    SLCD->SDATAH0.reg = 0;
    SLCD->SDATAH1.reg = 0;
    SLCD->SDATAH2.reg = 0;
    SLCD->SDATAH3.reg = 0;
    SLCD->SDATAH4.reg = 0;
    SLCD->SDATAH5.reg = 0;
    SLCD->SDATAH6.reg = 0;
    SLCD->SDATAH7.reg = 0;
}

void slcd_set_segment(uint8_t com, uint8_t seg) {
	((uint32_t *)&(SLCD->SDATAL0))[(com * 2) + (seg >> 5)] |= (seg < 32) ? (1 << seg) : (1 << (seg >> 5));
}

void slcd_clear_segment(uint8_t com, uint8_t seg) {
    ((uint32_t *)&(SLCD->SDATAL0))[(com * 2) + (seg >> 5)] &= ~((seg < 32) ? (1 << seg) : (1 << (seg >> 5)));
}

void slcd_configure_frame_counter(uint8_t fc, uint8_t overflow_count, bool prescale) {
    switch (fc) {
    case 0:
        SLCD->FC0.reg = SLCD_FC0_OVF(overflow_count) | (prescale ? 0 : SLCD_FC0_PB);
        break;
    case 1:
        SLCD->FC1.reg = SLCD_FC1_OVF(overflow_count) | (prescale ? 0 : SLCD_FC1_PB);
        break;
    case 2:
        SLCD->FC2.reg = SLCD_FC2_OVF(overflow_count) | (prescale ? 0 : SLCD_FC2_PB);
        break;
    default:
        break;
    }
}

void slcd_set_frame_counter_enabled(uint8_t fc, bool enabled) {
    switch (fc) {
    case 0:
        SLCD->CTRLD.bit.FC0EN = enabled ? 1 : 0;
        break;
    case 1:
        SLCD->CTRLD.bit.FC1EN = enabled ? 1 : 0;
        break;
    case 2:
        SLCD->CTRLD.bit.FC2EN = enabled ? 1 : 0;
        break;
    default:
        break;
    }
    _slcd_sync(SLCD_SYNCBUSY_CTRLD);
}

void slcd_set_frame_counter_interrupt_enabled(uint8_t fc, bool enabled) {
    if (enabled) SLCD->INTENSET.reg |= 1 << fc;
    else SLCD->INTENCLR.reg |= 1 << fc;
    NVIC_ClearPendingIRQ(SLCD_IRQn);
    if (enabled) NVIC_EnableIRQ(SLCD_IRQn);
    else NVIC_DisableIRQ(SLCD_IRQn);
}

void slcd_configure_blink(bool blink_all, uint8_t bss0, uint8_t bss1, uint8_t fc) {
    SLCD->BCFG.bit.FCS = fc;
    if (blink_all) {
        SLCD->BCFG.bit.MODE = 0;
    } else {
        SLCD->BCFG.bit.MODE = 1;
        SLCD->BCFG.bit.BSS0 = bss0;
        SLCD->BCFG.bit.BSS1 = bss1;
    }
}

void slcd_set_blink_enabled(bool enabled) {
    SLCD->CTRLD.bit.BLINK = enabled ? 1 : 0;
    _slcd_sync(SLCD_SYNCBUSY_CTRLD);
}

void slcd_configure_circular_shift_animation(uint16_t initial_value, uint8_t size, slcd_csrshift_value_t shift_dir, uint8_t fc) {
    SLCD->CSRCFG.bit.FCS = fc;
    SLCD->CSRCFG.bit.SIZE = size;
    SLCD->CSRCFG.bit.DIR = shift_dir;
    SLCD->CSRCFG.bit.DATA = initial_value;
}

void slcd_set_circular_shift_animation_enabled(bool enabled) {
    SLCD->CTRLD.bit.CSREN = enabled ? 1 : 0;
    _slcd_sync(SLCD_SYNCBUSY_CTRLD);
}

void slcd_disable(void) {
    SLCD->CTRLA.bit.ENABLE = 0;
    _slcd_sync(SLCD_SYNCBUSY_ENABLE);
}

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