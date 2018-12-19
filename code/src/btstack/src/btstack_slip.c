/*
 * Copyright (C) 2014 BlueKitchen GmbH
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holders nor the names of
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 * 4. Any redistribution, use, or modification is done solely for
 *    personal benefit and not for any commercial purpose or for
 *    monetary gain.
 *
 * THIS SOFTWARE IS PROVIDED BY BLUEKITCHEN GMBH AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL MATTHIAS
 * RINGWALD OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
 * THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * Please inquire about commercial licensing options at
 * contact@bluekitchen-gmbh.com
 *
 */

#define __BTSTACK_FILE__ "btstack_slip.c"

/*
 *  btstack_slip.c
 *  SLIP encoder/decoder
 */

#include "btstack_slip.h"
#include "btstack_debug.h"

typedef enum {
	SLIP_ENCODER_DEFAULT,
	SLIP_ENCODER_SEND_DC,
	SLIP_ENCODER_SEND_DD
} btstack_slip_encoder_state_t;

// h5 slip state machine
typedef enum {
	SLIP_DECODER_UNKNOWN = 1,
	SLIP_DECODER_ACTIVE,
	SLIP_DECODER_X_C0,
	SLIP_DECODER_X_DB,
	SLIP_DECODER_COMPLETE
} btstack_slip_decoder_state_t;


// encoder
static btstack_slip_encoder_state_t encoder_state;
static const uint8_t* encoder_data;
static uint16_t  encoder_len;

// decoder
static btstack_slip_decoder_state_t decoder_state;
static btstack_slip_decoder_state_t decoder_state_ext;

static uint8_t* decoder_buffer;
static uint8_t* decoder_buffer_ext;

static uint16_t  decoder_max_size;
static uint16_t  decoder_max_size_ext;

static uint16_t  decoder_pos;
static uint16_t  decoder_pos_ext;



// ENCODER

/**
 * @brief Initialise SLIP encoder with data
 * @param data
 * @param len
 */
void btstack_slip_encoder_start(const uint8_t* data, uint16_t len) {
	encoder_state = SLIP_ENCODER_DEFAULT;
	encoder_data  = data;
	encoder_len   = len;
}

/**
 * @brief Check if encoder has data ready
 * @return True if data ready
 */
int  btstack_slip_encoder_has_data(void) {
	if (encoder_state != SLIP_ENCODER_DEFAULT) {
		return 1;
	}

	return encoder_len > 0;
}

/**
 * @brief Get next byte from encoder
 * @return Next bytes from encoder
 */
uint8_t btstack_slip_encoder_get_byte(void) {
	uint8_t next_byte;

	switch (encoder_state) {
	case SLIP_ENCODER_DEFAULT:
		next_byte = *encoder_data++;
		encoder_len--;

		switch (next_byte) {
		case BTSTACK_SLIP_SOF:
			encoder_state = SLIP_ENCODER_SEND_DC;
			return 0xdb;

		case 0xdb:
			encoder_state = SLIP_ENCODER_SEND_DD;
			return 0xdb;

		default:
			return next_byte;
		}

		break;

	case SLIP_ENCODER_SEND_DC:
		encoder_state = SLIP_ENCODER_DEFAULT;
		return 0x0dc;

	case SLIP_ENCODER_SEND_DD:
		encoder_state = SLIP_ENCODER_DEFAULT;
		return 0x0dd;

	default:
		log_error("btstack_slip_encoder_get_byte invalid state %x", encoder_state);
		return 0x00;
	}
}

// Decoder

static void btstack_slip_decoder_reset(void) {
	decoder_state = SLIP_DECODER_UNKNOWN;
	decoder_pos = 0;
}

static void btstack_slip_decoder_reset_ext(void) {
	decoder_state_ext = SLIP_DECODER_UNKNOWN;
	decoder_pos_ext = 0;
}

static void btstack_slip_decoder_store_byte(uint8_t input) {
	if (decoder_pos >= decoder_max_size) {
		log_error("btstack_slip_decoder_store_byte: packet to long");
		btstack_slip_decoder_reset();
	}

	decoder_buffer[decoder_pos++] = input;
}

#include "bt_init.h"
extern tBtCtrlOpt *getBtOptPtr(void);

static void btstack_slip_decoder_store_byte_ext(uint8_t input) {
	//tBtCtrlOpt *ptBtOpt = getBtOptPtr();
	if (decoder_pos_ext >= decoder_max_size_ext) {
		log_error("btstack_slip_decoder_store_byte_ext: packet to long");
		btstack_slip_decoder_reset_ext();
	}

	decoder_buffer_ext[decoder_pos_ext++] = input;
	//if(ptBtOpt->Connected) {
		//log_debug("decoder_pos_ext:%d input:%x", decoder_pos_ext, input);
	//}

	//QuePutc((T_Queue *)(&ptBtOpt->tMasterRecvQueTest), input);
}


/**
 * @brief Initialise SLIP decoder with buffer
 * @param buffer to store received data
 * @param max_size of buffer
 */
void btstack_slip_decoder_init(uint8_t* buffer, uint16_t max_size) {
	decoder_buffer = buffer;
	decoder_max_size = max_size;
	btstack_slip_decoder_reset();
}

void btstack_slip_decoder_init_ext(uint8_t* buffer, uint16_t max_size) {
	decoder_buffer_ext = buffer;
	decoder_max_size_ext = max_size;
	btstack_slip_decoder_reset_ext();
}


/**
 * @brief Process received byte
 * @param data
 */

void btstack_slip_decoder_process(uint8_t input) {
	switch(decoder_state) {
	case SLIP_DECODER_UNKNOWN:
		if (input != BTSTACK_SLIP_SOF) {
			break;
		}

		btstack_slip_decoder_reset();
		decoder_state = SLIP_DECODER_X_C0;
		break;

	case SLIP_DECODER_COMPLETE:
		log_error("btstack_slip_decoder_process called in state COMPLETE");
		btstack_slip_decoder_reset();
		break;

	case SLIP_DECODER_X_C0:
		switch(input) {
		case BTSTACK_SLIP_SOF:
			break;

		case 0xdb:
			decoder_state = SLIP_DECODER_X_DB;
			break;

		default:
			btstack_slip_decoder_store_byte(input);
			decoder_state = SLIP_DECODER_ACTIVE;
			break;
		}

		break;

	case SLIP_DECODER_X_DB:
		switch(input) {
		case 0xdc:
			btstack_slip_decoder_store_byte(BTSTACK_SLIP_SOF);
			decoder_state = SLIP_DECODER_ACTIVE;
			break;

		case 0xdd:
			btstack_slip_decoder_store_byte(0xdb);
			decoder_state = SLIP_DECODER_ACTIVE;
			break;

		default:
			btstack_slip_decoder_reset();
			break;
		}

		break;

	case SLIP_DECODER_ACTIVE:
		switch(input) {
		case BTSTACK_SLIP_SOF:
			if (decoder_pos) {
				decoder_state = SLIP_DECODER_COMPLETE;
			} else {
				btstack_slip_decoder_reset();
			}

			break;

		case 0xdb:
			decoder_state = SLIP_DECODER_X_DB;
			break;

		default:
			btstack_slip_decoder_store_byte(input);
			break;
		}

		break;
	}
}
void btstack_slip_decoder_process_ext(uint8_t input) {
	switch(decoder_state_ext) {
	case SLIP_DECODER_UNKNOWN:
		if (input != BTSTACK_SLIP_SOF) {
			break;
		}

		btstack_slip_decoder_reset_ext();
		decoder_state_ext = SLIP_DECODER_X_C0;
		break;

	case SLIP_DECODER_COMPLETE:
		log_error("btstack_slip_decoder_process_ext called in state COMPLETE");
		btstack_slip_decoder_reset_ext();
		break;

	case SLIP_DECODER_X_C0:
		switch(input) {
		case BTSTACK_SLIP_SOF:
			break;

		case 0xdb:
			decoder_state_ext = SLIP_DECODER_X_DB;
			break;

		default:
			btstack_slip_decoder_store_byte_ext(input);
			decoder_state_ext = SLIP_DECODER_ACTIVE;
			break;
		}

		break;

	case SLIP_DECODER_X_DB:
		switch(input) {
		case 0xdc:
			btstack_slip_decoder_store_byte_ext(BTSTACK_SLIP_SOF);
			decoder_state_ext = SLIP_DECODER_ACTIVE;
			break;

		case 0xdd:
			btstack_slip_decoder_store_byte_ext(0xdb);
			decoder_state_ext = SLIP_DECODER_ACTIVE;
			break;

		default:
			btstack_slip_decoder_reset_ext();
			break;
		}

		break;

	case SLIP_DECODER_ACTIVE:
		switch(input) {
		case BTSTACK_SLIP_SOF:
			if (decoder_pos_ext) {
				decoder_state_ext = SLIP_DECODER_COMPLETE;
			} else {
				btstack_slip_decoder_reset_ext();
			}

			break;

		case 0xdb:
			decoder_state_ext = SLIP_DECODER_X_DB;
			break;

		default:
			btstack_slip_decoder_store_byte_ext(input);
			break;
		}

		break;
	}
}


/**
 * @brief Get size of decoded frame
 * @return size of frame. Size = 0 => frame not complete
 */

uint16_t btstack_slip_decoder_frame_size(void) {
	switch (decoder_state) {
	case SLIP_DECODER_COMPLETE:
		return decoder_pos;

	default:
		return 0;
	}
}


extern uint8_t   pre_buffer_ext[1711];
uint16_t gFrameSizeExt[1024];
int numExt = 0;
uint16_t btstack_slip_decoder_frame_size_ext(void) {
	tBtCtrlOpt *ptBtOpt = getBtOptPtr();
	switch (decoder_state_ext) {
	case SLIP_DECODER_COMPLETE:
		//add by zhufeng
		//log_debug("111 size decoder_pos_ext:%d pre_buffer_ext:%d %d",decoder_pos_ext, pre_buffer_ext[0], pre_buffer_ext[1]);
		//QuePuts((T_Queue *)(&ptBtOpt->tMasterRecvQueTestLen), decoder_pos_ext, 1);
		//log_debug("size decoder_pos_ext:%d",(char)decoder_pos_ext);
		QuePuts((T_Queue *)(&ptBtOpt->tMasterRecvQueTest), &pre_buffer_ext[14], decoder_pos_ext);
		gFrameSizeExt[numExt] = decoder_pos_ext;
		//log_debug("numExt:%d decoder_pos_ext:%d gFrameSizeExt[numExt]:%d",numExt, decoder_pos_ext, gFrameSizeExt[numExt]);
		numExt++;
		return decoder_pos_ext;

	default:
		return 0;
	}
}

