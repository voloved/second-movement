#define GOSSAMER_DMA_HEADER_ONLY
#define dma_add_descriptor gossamer_dma_add_descriptor
#include "../gossamer/peripherals/dma.c"
#undef dma_add_descriptor
#undef GOSSAMER_DMA_HEADER_ONLY
#include "dma.h"

/// TODO: DMA on SAM D51
#ifndef _SAMD51_

struct _dma_module {
    volatile bool _dma_init;
    volatile uint32_t allocated_channels;
    uint8_t free_channels;
};

extern DmacDescriptor descriptor_section[DMAC_CH_NUM];

static struct _dma_module _dma_inst = {
    ._dma_init = false,
    .allocated_channels = 0,
    .free_channels = DMAC_CH_NUM,
};

#if (defined __GNUC__) || (defined __CC_ARM)
#define COMPILER_ALIGNED(a) __attribute__((__aligned__(a)))
#elif (defined __ICCARM__)
#define COMPILER_ALIGNED(a) __attribute__((__aligned__(a)))
#endif

/**
 * \brief Add a DMA transfer descriptor to a DMA resource.
 *
 * This function will add a DMA transfer descriptor to a DMA resource.
 * If there was a transfer descriptor already allocated to the DMA resource,
 * the descriptor will be linked to the next descriptor address.
 *
 * \param[in] resource Pointer to the DMA resource
 * \param[in] descriptor Pointer to the transfer descriptor
 *
 * \retval STATUS_OK The descriptor is added to the DMA resource
 * \retval STATUS_BUSY The DMA resource was busy and the descriptor is not added
 */
enum status_code dma_add_descriptor(struct dma_resource *resource,
		DmacDescriptor* descriptor)
{
	DmacDescriptor* desc = resource->descriptor;

	if (resource->job_status == STATUS_BUSY) {
		return STATUS_BUSY;
	}

	/* Look up for an empty space for the descriptor */
	if (desc == NULL) {
		resource->descriptor = descriptor;
	} else {
		/* Looking for end of descriptor link */
		while(desc->DESCADDR.reg != 0) {
			desc = (DmacDescriptor*)(desc->DESCADDR.reg);
		}

		/* Set to the end of descriptor list */
		desc->DESCADDR.reg = (uint32_t)descriptor;
	}

	return STATUS_OK;
}

/**
 * \brief Start a DMA transfer.
 *
 * This function will start a DMA transfer through an allocated DMA resource.
 *
 * \param[in,out] resource Pointer to the DMA resource
 *
 * \return Status of the transfer start procedure.
 *
 * \retval STATUS_OK The transfer was started successfully
 * \retval STATUS_BUSY The DMA resource was busy and the transfer was not started
 * \retval STATUS_ERR_INVALID_ARG Transfer size is 0 and transfer was not started
 */
enum status_code dma_start_transfer_job(struct dma_resource *resource)
{
	if (resource == NULL) return STATUS_ERR_INVALID_ARG;
	if(resource->channel_id == DMA_INVALID_CHANNEL) return STATUS_ERR_INVALID_ARG;

	cpu_irq_enter_critical();

	/* Check if resource was busy */
	if (resource->job_status == STATUS_BUSY) {
		cpu_irq_leave_critical();
		return STATUS_BUSY;
	}

	/* Check if transfer size is valid */
	if (resource->descriptor->BTCNT.reg == 0) {
		cpu_irq_leave_critical();
		return STATUS_ERR_INVALID_ARG;
	}

	/* Enable DMA interrupt */
	NVIC_EnableIRQ(DMAC_IRQn);

	/* Set the interrupt flag */
	DMAC->CHID.reg = DMAC_CHID_ID(resource->channel_id);
	DMAC->CHINTENSET.reg = (DMAC_CHINTENSET_MASK & g_chan_interrupt_flag[resource->channel_id]);
	/* Set job status */
	resource->job_status = STATUS_BUSY;

	/* Set channel x descriptor 0 to the descriptor base address */
	memcpy(&descriptor_section[resource->channel_id], resource->descriptor,
						sizeof(DmacDescriptor));

	/* Enable the transfer channel */
	DMAC->CHCTRLA.reg |= DMAC_CHCTRLA_ENABLE;

	cpu_irq_leave_critical();
    // Commenting out this global interrupt disable seems to have ABM work...at 350uA

	return STATUS_OK;
}

/**
 * \brief Create a DMA transfer descriptor with configurations.
 *
 * This function will set the transfer configurations to the DMA transfer
 * descriptor.
 *
 * \param[in] descriptor Pointer to the DMA transfer descriptor
 * \param[in] config Pointer to the descriptor configuration structure
 *
 */
void dma_descriptor_create(DmacDescriptor* descriptor,
	struct dma_descriptor_config *config)
{
	/* Set block transfer control */
	descriptor->BTCTRL.bit.VALID = config->descriptor_valid;
	descriptor->BTCTRL.bit.EVOSEL = config->event_output_selection;
	descriptor->BTCTRL.bit.BLOCKACT = config->block_action;
	descriptor->BTCTRL.bit.BEATSIZE = config->beat_size;
	descriptor->BTCTRL.bit.SRCINC = config->src_increment_enable;
	descriptor->BTCTRL.bit.DSTINC = config->dst_increment_enable;
	descriptor->BTCTRL.bit.STEPSEL = config->step_selection;
	descriptor->BTCTRL.bit.STEPSIZE = config->step_size;

	/* Set transfer size, source address and destination address */
	descriptor->BTCNT.reg = config->block_transfer_count;
	descriptor->SRCADDR.reg = config->source_address;
	descriptor->DSTADDR.reg = config->destination_address;

	/* Set next transfer descriptor address */
	descriptor->DESCADDR.reg = config->next_descriptor_address;
}


/**
 * \brief Initializes config with predefined default values.
 *
 * This function will initialize a given DMA configuration structure to
 * a set of known default values. This function should be called on
 * any new instance of the configuration structure before being
 * modified by the user application.
 *
 * The default configuration is as follows:
 *  \li Software trigger is used as the transfer trigger
 *  \li Priority level 0
 *  \li Only software/event trigger
 *  \li Requires a trigger for each transaction
 *  \li No event input /output
 *  \li DMA channel is disabled during sleep mode (if has the feature)
 * \param[out] config Pointer to the configuration
 *
 */
void dma_get_config_defaults(struct dma_resource_config *config)
{
	if(config == NULL) return;
	/* Set as priority 0 */
	config->priority = DMA_PRIORITY_LEVEL_0;
	/* Only software/event trigger */
	config->peripheral_trigger = 0;
	/* Transaction trigger */
	config->trigger_action = DMA_TRIGGER_ACTION_TRANSACTION;

	/* Event configurations, no event input/output */
	config->event_config.input_action = DMA_EVENT_INPUT_NOACT;
	config->event_config.event_output_enable = false;
#ifdef FEATURE_DMA_CHANNEL_STANDBY
	config->run_in_standby = false;
#endif
}

COMPILER_ALIGNED(16)
DmacDescriptor descriptor_section[DMAC_CH_NUM] SECTION_DMAC_DESCRIPTOR;

/** Initial write back memory section. */
COMPILER_ALIGNED(16)
static DmacDescriptor _write_back_section[DMAC_CH_NUM] SECTION_DMAC_DESCRIPTOR;

/** Internal DMA resource pool. */
static struct dma_resource* _dma_active_resource[DMAC_CH_NUM];

/* DMA channel interrup flag. */
uint8_t g_chan_interrupt_flag[DMAC_CH_NUM]={0};

/**
 * \brief Find a free channel for a DMA resource.
 *
 * Find a channel for the requested DMA resource.
 *
 * \return Status of channel allocation.
 * \retval DMA_INVALID_CHANNEL  No channel available
 * \retval count          Allocated channel for the DMA resource
 */
static uint8_t _dma_find_first_free_channel_and_allocate(void)
{
	uint8_t count;
	uint32_t tmp;
	bool allocated = false;

	cpu_irq_enter_critical();

	tmp = _dma_inst.allocated_channels;

	for (count = 0; count < DMAC_CH_NUM; ++count) {
		if (!(tmp & 0x00000001)) {
			/* If free channel found, set as allocated and return
			 *number */

			_dma_inst.allocated_channels |= 1 << count;
			_dma_inst.free_channels--;
			allocated = true;

			break;
		}

		tmp = tmp >> 1;
	}

	cpu_irq_leave_critical();

	if (!allocated) {
		return DMA_INVALID_CHANNEL;
	} else {
		return count;
	}
}


/**
 * \brief Configure the DMA resource.
 *
 * \param[in]  dma_resource Pointer to a DMA resource instance
 * \param[out] resource_config Configurations of the DMA resource
 *
 */
static void _dma_set_config(struct dma_resource *resource,
		struct dma_resource_config *resource_config)
{
	if(resource == NULL) return;
	if(resource_config == NULL) return;
	uint32_t temp_CHCTRLB_reg;
	cpu_irq_enter_critical();

	/** Select the DMA channel and clear software trigger */
	DMAC->CHID.reg = DMAC_CHID_ID(resource->channel_id);
	DMAC->SWTRIGCTRL.reg &= (uint32_t)(~(1 << resource->channel_id));

	temp_CHCTRLB_reg = DMAC_CHCTRLB_LVL(resource_config->priority) | \
			DMAC_CHCTRLB_TRIGSRC(resource_config->peripheral_trigger) | \
			DMAC_CHCTRLB_TRIGACT(resource_config->trigger_action);


	if(resource_config->event_config.input_action){
	temp_CHCTRLB_reg |= DMAC_CHCTRLB_EVIE | DMAC_CHCTRLB_EVACT(
				resource_config->event_config.input_action);
	}

	/** Enable event output, the event output selection is configured in
	 * each transfer descriptor  */
	if (resource_config->event_config.event_output_enable) {
		temp_CHCTRLB_reg |= DMAC_CHCTRLB_EVOE;
	}

	/* Write config to CTRLB register */
	DMAC->CHCTRLB.reg = temp_CHCTRLB_reg;



	cpu_irq_leave_critical();
}

/**
 * \brief Allocate a DMA with configurations.
 *
 * This function will allocate a proper channel for a DMA transfer request.
 *
 * \param[in,out]  dma_resource Pointer to a DMA resource instance
 * \param[in] transfer_config Configurations of the DMA transfer
 *
 * \return Status of the allocation procedure.
 *
 * \retval STATUS_OK The DMA resource was allocated successfully
 * \retval STATUS_ERR_NOT_FOUND DMA resource allocation failed
 */
enum status_code dma_allocate(struct dma_resource *resource,
		struct dma_resource_config *config)
{
	uint8_t new_channel;

	if(resource == NULL) return STATUS_ERR_INVALID_ARG;

	cpu_irq_enter_critical();

	if (!_dma_inst._dma_init) {
		/* Initialize clocks for DMA */
#if defined(_SAML21_) || defined(_SAML22_)
        MCLK->AHBMASK.reg |= MCLK_AHBMASK_DMAC;
#else
		system_ahb_clock_set_mask(PM_AHBMASK_DMAC);
		system_apb_clock_set_mask(SYSTEM_CLOCK_APB_APBB,
				PM_APBBMASK_DMAC);
#endif

		/* Perform a software reset before enable DMA controller */
		DMAC->CTRL.reg &= ~DMAC_CTRL_DMAENABLE;
		DMAC->CTRL.reg = DMAC_CTRL_SWRST;

		/* Setup descriptor base address and write back section base
		 * address */
		DMAC->BASEADDR.reg = (uint32_t)descriptor_section;
		DMAC->WRBADDR.reg = (uint32_t)_write_back_section;

		/* Enable all priority level at the same time */
		DMAC->CTRL.reg = DMAC_CTRL_DMAENABLE | DMAC_CTRL_LVLEN(0xf);

		_dma_inst._dma_init = true;
	}

	/* Find the proper channel */
	new_channel = _dma_find_first_free_channel_and_allocate();

	/* If no channel available, return not found */
	if (new_channel == DMA_INVALID_CHANNEL) {
		cpu_irq_leave_critical();

		return STATUS_ERR_NOT_FOUND;
	}

	/* Set the channel */
	resource->channel_id = new_channel;

	/** Perform a reset for the allocated channel */
	DMAC->CHID.reg = DMAC_CHID_ID(resource->channel_id);
	DMAC->CHCTRLA.reg &= ~DMAC_CHCTRLA_ENABLE;
	DMAC->CHCTRLA.reg = DMAC_CHCTRLA_SWRST;

	while (DMAC->CHCTRLA.bit.SWRST);

#ifdef FEATURE_DMA_CHANNEL_STANDBY
	if(config->run_in_standby){
		DMAC->CHCTRLA.reg |= DMAC_CHCTRLA_RUNSTDBY;
	}
#endif

	/** Configure the DMA control,channel registers and descriptors here */
	_dma_set_config(resource, config);

	resource->descriptor = NULL;

	/* Log the DMA resource into the internal DMA resource pool */
	_dma_active_resource[resource->channel_id] = resource;

	cpu_irq_leave_critical();

	return STATUS_OK;
}
#endif
