
#include "sam.h"
#include "dma_util.h"

#ifdef GOSSAMER_DMA_HEADER_ONLY
#define dma_add_descriptor gossamer_dma_add_descriptor
#include "../gossamer/common/dma.h"
#else
#ifndef ABM_DMA_H
#define ABM_DMA_H
#define dma_add_descriptor gossamer_dma_add_descriptor
#include "../gossamer/common/dma.h"
#undef dma_add_descriptor

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
#endif // ABM_DMA_H
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

#define dma_add_descriptor gossamer_dma_add_descriptor
#include "../gossamer/common/dma.h"
#undef dma_add_descriptor

#endif
