////< @file dma.h
/*
 * MIT License
 *
 * Copyright (c) 2022 Joey Castillo
 * Based heavily on Adafruit_ZeroDMA
 * Written by Phil "PaintYourDragon" Burgess for Adafruit Industries,
 * based partly on DMA insights from Atmel ASFCORE 3.
 *
 * Adafruit invests time and resources providing this open source code,
 * please support Adafruit and open-source hardware by purchasing
 * products from Adafruit!
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

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "dma_util.h"
#include "sam.h"

/**
 * @addtogroup dma Direct Memory Access Controller
 * @brief WIP functions for configuring and using the Direct Memory Access controller.
 * @{
 */ 

#ifndef _SAMD51_

typedef struct {
    uint8_t channel;                            // (0 to DMAC_CH_NUM-1, or 0xFF)
    volatile dma_status_t jobStatus;            // Last known DMA job status
    bool hasDescriptors;                        // 'true' if one or more descriptors assigned
    bool loopFlag;                              // 'true' if descriptor chain loops back to start
    DmacDescriptor *descriptor;                 // Resource-owned descriptor chain head
    void (*callbacks[DMA_CALLBACK_N])(void *);  // Callback function (is passed an instance of this struct)
} gossamer_dma_job_t;

/**
 * @brief Initialize the DMA system.
 * @details Sets up peripheral clocks and points the DMA controller at an empty
 *          list of descriptors.
 *          
 */
void dma_init(void);

/**
 * @brief Configures a DMA job.
 * @details Configures a DMA job with the given peripheral trigger and trigger action.
 *          You must configure the job before adding descriptors to it.
 * @param dmaJob A pointer to the DMA job to configure. This will be modified in place.
 * @param peripheralTrigger The peripheral trigger to use for this job. These are
 *                          defined in your microcontroller's data sheet, in the DMAC's
 *                          register details section, under a headling like "Trigger Source"
 * @param triggerAction The trigger action to use for this job. One of: 
 *                      * DMA_TRIGGER_ACTION_BLOCK
 *                      * DMA_TRIGGER_ACTION_BEAT
 *                      * DMA_TRIGGER_ACTION_TRANSACTION
 * @param flags A OR'ed bitmask of flags to use for this job. Available flags are:
 *              * DMA_CONFIG_LOOP - Loop the job when it completes
 *              * DMA_CONFIG_RUNSTDBY - Keep the job running in standby mode
 */
bool dma_configure(gossamer_dma_job_t *dmaJob, uint8_t peripheralTrigger, dma_trigger_action_t triggerAction, dma_configuration_flags_t flags);

/**
 * @brief Adds a descriptor to a DMA job.
 * @details Adds a descriptor to a DMA job. You must configure the job before adding
 *         descriptors to it.
 * @param dmaJob A pointer to the DMA job. Must be previously configured with dma_configure().
 * @param src The source address for this descriptor.
 * @param dst The destination address for this descriptor.
 * @param count The number of beats to transfer.
 * @param size The beat size. One of:
 *             * DMA_BEAT_SIZE_BYTE - One byte per beat
 *             * DMA_BEAT_SIZE_HWORD - One half word (two bytes) per beat
 *             * DMA_BEAT_SIZE_WORD - One word (four bytes) per beat
 * @param srcInc Whether to increment the source address after each beat.
 * @param dstInc Whether to increment the destination address after each beat.
 * @param stepSize Address increment step size. One of:
 *                 * DMA_STEPSIZE_1 - Increment by 1 * beat size
 *                 * DMA_STEPSIZE_2 - Increment by 2 * beat size
 *                 * DMA_STEPSIZE_4 - Increment by 4 * beat size
 *                 * DMA_STEPSIZE_8 - ...you get the idea
 *                 * DMA_STEPSIZE_16
 *                 * DMA_STEPSIZE_32
 *                 * DMA_STEPSIZE_64
 *                 * DMA_STEPSIZE_128
 * @param stepSel Which address to increment by stepSize after each step. You can only have one:
 *                * DMA_STEPSEL_SOURCE - Increment the source address after each step
 *                * DMA_STEPSEL_DESTINATION - Increment the destination address after each step
 * @param blockAction The action to perform when this descriptor's block transfer completes.
 * 
*/
DmacDescriptor *dma_add_descriptor_goss(gossamer_dma_job_t *dmaJob, void *src, void *dst, uint32_t count, dma_beat_size_t size, dma_address_increment_t addressIncrement, dma_stepsize_t stepSize, dma_stepsel_t stepSel, dma_block_action_t blockAction);

bool dma_start_job(gossamer_dma_job_t *dmaJob);

#endif

/** Mask to retrieve the error category of a status code. */
#define STATUS_CATEGORY_MASK  0xF0

/** Mask to retrieve the error code within the category of a status code. */
#define STATUS_ERROR_MASK     0x0F

/** Status code error categories. */
enum status_categories {
	STATUS_CATEGORY_OK                = 0x00,
	STATUS_CATEGORY_COMMON            = 0x10,
	STATUS_CATEGORY_ANALOG            = 0x30,
	STATUS_CATEGORY_COM               = 0x40,
	STATUS_CATEGORY_IO                = 0x50,
};
enum status_code {
	STATUS_OK                         = STATUS_CATEGORY_OK     | 0x00,
	STATUS_VALID_DATA                 = STATUS_CATEGORY_OK     | 0x01,
	STATUS_NO_CHANGE                  = STATUS_CATEGORY_OK     | 0x02,
	STATUS_ABORTED                    = STATUS_CATEGORY_OK     | 0x04,
	STATUS_BUSY                       = STATUS_CATEGORY_OK     | 0x05,
	STATUS_SUSPEND                    = STATUS_CATEGORY_OK     | 0x06,

	STATUS_ERR_IO                     = STATUS_CATEGORY_COMMON | 0x00,
	STATUS_ERR_REQ_FLUSHED            = STATUS_CATEGORY_COMMON | 0x01,
	STATUS_ERR_TIMEOUT                = STATUS_CATEGORY_COMMON | 0x02,
	STATUS_ERR_BAD_DATA               = STATUS_CATEGORY_COMMON | 0x03,
	STATUS_ERR_NOT_FOUND              = STATUS_CATEGORY_COMMON | 0x04,
	STATUS_ERR_UNSUPPORTED_DEV        = STATUS_CATEGORY_COMMON | 0x05,
	STATUS_ERR_NO_MEMORY              = STATUS_CATEGORY_COMMON | 0x06,
	STATUS_ERR_INVALID_ARG            = STATUS_CATEGORY_COMMON | 0x07,
	STATUS_ERR_BAD_ADDRESS            = STATUS_CATEGORY_COMMON | 0x08,
	STATUS_ERR_BAD_FORMAT             = STATUS_CATEGORY_COMMON | 0x0A,
	STATUS_ERR_BAD_FRQ                = STATUS_CATEGORY_COMMON | 0x0B,
	STATUS_ERR_DENIED                 = STATUS_CATEGORY_COMMON | 0x0c,
	STATUS_ERR_ALREADY_INITIALIZED    = STATUS_CATEGORY_COMMON | 0x0d,
	STATUS_ERR_OVERFLOW               = STATUS_CATEGORY_COMMON | 0x0e,
	STATUS_ERR_NOT_INITIALIZED        = STATUS_CATEGORY_COMMON | 0x0f,

	STATUS_ERR_SAMPLERATE_UNAVAILABLE = STATUS_CATEGORY_ANALOG | 0x00,
	STATUS_ERR_RESOLUTION_UNAVAILABLE = STATUS_CATEGORY_ANALOG | 0x01,

	STATUS_ERR_BAUDRATE_UNAVAILABLE   = STATUS_CATEGORY_COM    | 0x00,
	STATUS_ERR_PACKET_COLLISION       = STATUS_CATEGORY_COM    | 0x01,
	STATUS_ERR_PROTOCOL               = STATUS_CATEGORY_COM    | 0x02,

	STATUS_ERR_PIN_MUX_INVALID        = STATUS_CATEGORY_IO     | 0x00,
};


#if defined(_SAML21_) || defined(_SAML22_)
#define FEATURE_DMA_CHANNEL_STANDBY
#endif

#define DMA_INVALID_CHANNEL        0xff

/* DMA channel interrup flag. */
extern uint8_t g_chan_interrupt_flag[DMAC_CH_NUM];

/** DMA priority level. */
enum dma_priority_level {
	/** Priority level 0. */
	DMA_PRIORITY_LEVEL_0,
	/** Priority level 1. */
	DMA_PRIORITY_LEVEL_1,
	/** Priority level 2. */
	DMA_PRIORITY_LEVEL_2,
	/** Priority level 3. */
	DMA_PRIORITY_LEVEL_3,
};

/** DMA input actions. */
enum dma_event_input_action {
	/** No action. */
	DMA_EVENT_INPUT_NOACT,
	/** Normal transfer and periodic transfer trigger. */
	DMA_EVENT_INPUT_TRIG,
	/** Conditional transfer trigger. */
	DMA_EVENT_INPUT_CTRIG,
	/** Conditional block transfer. */
	DMA_EVENT_INPUT_CBLOCK,
	/** Channel suspend operation. */
	DMA_EVENT_INPUT_SUSPEND,
	/** Channel resume operation. */
	DMA_EVENT_INPUT_RESUME,
	/** Skip next block suspend action. */
	DMA_EVENT_INPUT_SSKIP,
};

/**
 * Address increment step size. These bits select the address increment step
 * size. The setting apply to source or destination address, depending on
 * STEPSEL setting.
 */
enum dma_address_increment_stepsize {
	/** The address is incremented by (beat size * 1). */
	DMA_ADDRESS_INCREMENT_STEP_SIZE_1 = 0,
	/** The address is incremented by (beat size * 2). */
	DMA_ADDRESS_INCREMENT_STEP_SIZE_2,
	/** The address is incremented by (beat size * 4). */
	DMA_ADDRESS_INCREMENT_STEP_SIZE_4,
	/** The address is incremented by (beat size * 8). */
	DMA_ADDRESS_INCREMENT_STEP_SIZE_8,
	/** The address is incremented by (beat size * 16). */
	DMA_ADDRESS_INCREMENT_STEP_SIZE_16,
	/** The address is incremented by (beat size * 32). */
	DMA_ADDRESS_INCREMENT_STEP_SIZE_32,
	/** The address is incremented by (beat size * 64). */
	DMA_ADDRESS_INCREMENT_STEP_SIZE_64,
	/** The address is incremented by (beat size * 128). */
	DMA_ADDRESS_INCREMENT_STEP_SIZE_128,
};

/**
 * DMA step selection. This bit determines whether the step size setting
 * is applied to source or destination address.
 */
enum dma_step_selection {
	/** Step size settings apply to the destination address. */
	DMA_STEPSEL_DST = 0,
	/** Step size settings apply to the source address. */
	DMA_STEPSEL_SRC,
};

/** Configurations for DMA events. */
struct dma_events_config {
	/** Event input actions */
	enum dma_event_input_action input_action;
	/** Enable DMA event output */
	bool event_output_enable;
};

/** DMA configurations for transfer. */
struct dma_resource_config {
	/** DMA transfer priority */
	enum dma_priority_level priority;
	/**DMA peripheral trigger index */
	uint8_t peripheral_trigger;
	/** DMA trigger action */
	dma_trigger_action_t trigger_action;
#ifdef FEATURE_DMA_CHANNEL_STANDBY
	/** Keep DMA channel enabled in standby sleep mode if true */
	bool run_in_standby;
#endif
	/** DMA events configurations */
	struct dma_events_config event_config;
};

/** Forward definition of the DMA resource. */
struct dma_resource;
/** Type definition for a DMA resource callback function. */
typedef void (*dma_callback_t)(struct dma_resource *const resource);

/** Structure for DMA transfer resource. */
struct dma_resource {
	/** Allocated DMA channel ID */
	uint8_t channel_id;
	/** Array of callback functions for DMA transfer job */
	dma_callback_t callback[DMA_CALLBACK_N];
	/** Bit mask for enabled callbacks */
	uint8_t callback_enable;
	/** Status of the last job */
	volatile enum status_code job_status;
	/** Transferred data size */
	uint32_t transfered_size;
	/** DMA transfer descriptor */
	DmacDescriptor* descriptor;
};

/**
 * DMA transfer descriptor configuration. When the source or destination address
 * increment is enabled, the addresses stored into the configuration structure
 * must correspond to the end of the transfer.
 *
 */
struct dma_descriptor_config {
	/** Descriptor valid flag used to identify whether a descriptor is
	    valid or not */
	bool descriptor_valid;
	/** This is used to generate an event on specific transfer action in
	    a channel. Supported only in four lower channels. */
	dma_event_output_selection_t event_output_selection;
	/** Action taken when a block transfer is completed */
	dma_block_action_t block_action;
	/** Beat size is configurable as 8-bit, 16-bit, or 32-bit */
	dma_beat_size_t beat_size;
	/** Used for enabling the source address increment */
	bool src_increment_enable;
	/** Used for enabling the destination address increment */
	bool dst_increment_enable;
	/** This bit selects whether the source or destination address is
	    using the step size settings */
	enum dma_step_selection step_selection;
	/** The step size for source/destination address increment.
	    The next address is calculated
	    as next_addr = addr + (2^step_size * beat size). */
	enum dma_address_increment_stepsize step_size;
	/** It is the number of beats in a block. This count value is
	 * decremented by one after each beat data transfer. */
	uint16_t block_transfer_count;
	/** Transfer source address */
	uint32_t source_address;
	/** Transfer destination address */
	uint32_t destination_address;
	/** Set to zero for static descriptors. This must have a valid memory
	    address for linked descriptors. */
	uint32_t next_descriptor_address;
};

static inline void dma_descriptor_get_config_defaults(struct dma_descriptor_config *config)
{
    if (config == NULL) {
        return;
    }

	/* Set descriptor as valid */
	config->descriptor_valid = true;
	/* Disable event output */
	config->event_output_selection = DMA_EVENT_OUTPUT_DISABLE;
	/* No block action */
	config->block_action = DMA_BLOCK_ACTION_NONE;
	/* Set beat size to one byte */
	config->beat_size = DMA_BEAT_SIZE_BYTE;
	/* Enable source increment */
	config->src_increment_enable = true;
	/* Enable destination increment */
	config->dst_increment_enable = true;
	/* Step size is applied to the destination address */
	config->step_selection = DMA_STEPSEL_DST;
	/* Address increment is beat size multiplied by 1*/
	config->step_size = DMA_ADDRESS_INCREMENT_STEP_SIZE_1;
	/* Default transfer size is set to 0 */
	config->block_transfer_count = 0;
	/* Default source address is set to NULL */
	config->source_address = (uint32_t)NULL;
	/* Default destination address is set to NULL */
	config->destination_address = (uint32_t)NULL;
	/** Next descriptor address set to 0 */
	config->next_descriptor_address = 0;
}

void dma_get_config_defaults(struct dma_resource_config *config);
enum status_code dma_allocate(struct dma_resource *resource,
		struct dma_resource_config *config);

enum status_code dma_start_transfer_job(struct dma_resource *resource);
void dma_descriptor_create(DmacDescriptor* descriptor,
	struct dma_descriptor_config *config);
enum status_code dma_add_descriptor(struct dma_resource *resource,
		DmacDescriptor* descriptor);

/** @} */