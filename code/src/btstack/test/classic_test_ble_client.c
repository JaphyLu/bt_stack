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

#define __BTSTACK_FILE__ "le_streamer_client.c"

/*
 * le_streamer_client.c
 */

// *****************************************************************************
/* EXAMPLE_START(le_streamer_client): Connects to 'LE Streamer' and subscribes to test characteristic
 */
// *****************************************************************************

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "btstack.h"

// prototypes
static void handle_gatt_client_event(uint8_t packet_type, uint16_t channel, uint8_t* packet, uint16_t size);

typedef enum {
	TC_OFF,
	TC_IDLE,
	TC_W4_SCAN_RESULT,
	TC_W4_CONNECT,
	TC_W4_SERVICE_RESULT,
	TC_W4_CHARACTERISTIC_RX_RESULT,
	TC_W4_CHARACTERISTIC_TX_RESULT,
	TC_W4_ENABLE_NOTIFICATIONS_COMPLETE,
	TC_W4_TEST_DATA
} gc_state_t;


// addr and type of device with correct name
static bd_addr_t      le_streamer_addr;
static bd_addr_type_t le_streamer_addr_type;

static hci_con_handle_t connection_handle;

// On the GATT Server, RX Characteristic is used for receive data via Write, and TX Characteristic is used to send data via Notifications
static uint8_t le_streamer_service_uuid[16]           = { 0x00, 0x00, 0xFF, 0x10, 0x00, 0x00, 0x10, 0x00, 0x80, 0x00, 0x00, 0x80, 0x5F, 0x9B, 0x34, 0xFB};
static uint8_t le_streamer_characteristic_rx_uuid[16] = { 0x00, 0x00, 0xFF, 0x11, 0x00, 0x00, 0x10, 0x00, 0x80, 0x00, 0x00, 0x80, 0x5F, 0x9B, 0x34, 0xFB};
static uint8_t le_streamer_characteristic_tx_uuid[16] = { 0x00, 0x00, 0xFF, 0x12, 0x00, 0x00, 0x10, 0x00, 0x80, 0x00, 0x00, 0x80, 0x5F, 0x9B, 0x34, 0xFB};

static gatt_client_service_t le_streamer_service;
static gatt_client_characteristic_t le_streamer_characteristic_rx;
static gatt_client_characteristic_t le_streamer_characteristic_tx;

static gatt_client_notification_t notification_listener;
static int listener_registered;

static gc_state_t state = TC_OFF;
static btstack_packet_callback_registration_t hci_event_callback_registration;

/*
 * @section Track throughput
 * @text We calculate the throughput by setting a start time and measuring the amount of
 * data sent. After a configurable REPORT_INTERVAL_MS, we print the throughput in kB/s
 * and reset the counter and start time.
 */

/* LISTING_START(tracking): Tracking throughput */

#define TEST_MODE_WRITE_WITHOUT_RESPONSE 0
#define TEST_MODE_ENABLE_NOTIFICATIONS   0
#define TEST_MODE_DUPLEX                 0

// configure test mode: send only, receive only, full duplex
#define TEST_MODE TEST_MODE_DUPLEX

#define REPORT_INTERVAL_MS 3000

// support for multiple clients
typedef struct {
	char name;
	int le_notification_enabled;
	int  counter;
	char test_data[200];
	int  test_data_len;
	uint32_t test_data_sent;
	uint32_t test_data_start;
} le_streamer_connection_t;

static le_streamer_connection_t le_streamer_connection;
static uint16_t BtTestReady = 0;

static void test_reset(le_streamer_connection_t* context) {
	context->test_data_start = btstack_run_loop_get_time_ms();
	context->test_data_sent = 0;
}

static void test_track_data(le_streamer_connection_t* context, int bytes_sent) {
	context->test_data_sent += bytes_sent;
	// evaluate
	uint32_t now = btstack_run_loop_get_time_ms();
	uint32_t time_passed = now - context->test_data_start;

	if (time_passed < REPORT_INTERVAL_MS) {
		return;
	}

	// print speed
	int bytes_per_second = context->test_data_sent * 1000 / time_passed;
	log_debug("%c: %"PRIu32" bytes -> %u.%03u kB/s\n", context->name, context->test_data_sent, bytes_per_second / 1000, bytes_per_second % 1000);
	// restart
	context->test_data_start = now;
	context->test_data_sent  = 0;
}
/* LISTING_END(tracking): Tracking throughput */


// stramer
static void streamer(le_streamer_connection_t* context) {
	if (connection_handle == HCI_CON_HANDLE_INVALID) {
		return;
	}

	// create test data
	context->counter++;

	if (context->counter > 'Z') {
		context->counter = 'A';
	}

	memset(context->test_data, context->counter, context->test_data_len);
	// send
	log_debug("streamer write test_data:%s, data_len:%d", context->test_data, context->test_data_len);
	uint8_t status = gatt_client_write_value_of_characteristic(handle_gatt_client_event, connection_handle, le_streamer_characteristic_rx.value_handle, context->test_data_len, (uint8_t*) context->test_data);

	if (status) {
		log_debug("error %02x for write without response!\n", status);
		return;
	} else {
		test_track_data(&le_streamer_connection, context->test_data_len);
	}

	// request again
	//gatt_client_request_can_write_without_response_event(handle_gatt_client_event, connection_handle);
}


// returns 1 if name is found in advertisement
static int advertisement_report_contains_name(const char* name, uint8_t* advertisement_report) {
	// get advertisement from report event
	const uint8_t* adv_data = gap_event_advertising_report_get_data(advertisement_report);
	uint16_t        adv_len  = gap_event_advertising_report_get_data_length(advertisement_report);
	int             name_len = strlen(name);
	// iterate over advertisement data
	ad_context_t context;

	for (ad_iterator_init(&context, adv_len, adv_data) ; ad_iterator_has_more(&context) ; ad_iterator_next(&context)) {
		uint8_t data_type    = ad_iterator_get_data_type(&context);
		uint8_t data_size    = ad_iterator_get_data_len(&context);
		const uint8_t* data = ad_iterator_get_data(&context);
		int i;

		switch (data_type) {
		case BLUETOOTH_DATA_TYPE_SHORTENED_LOCAL_NAME:
		case BLUETOOTH_DATA_TYPE_COMPLETE_LOCAL_NAME:

			// compare common prefix
			for (i=0; i<data_size && i<name_len; i++) {
				if (data[i] != name[i]) {
					break;
				}
			}

			// prefix match
			return 1;

		default:
			break;
		}
	}

	return 0;
}

static void handle_gatt_client_event(uint8_t packet_type, uint16_t channel, uint8_t* packet, uint16_t size) {
	UNUSED(packet_type);
	UNUSED(channel);
	UNUSED(size);
	uint16_t mtu;
	uint16_t value_handle,value_length,value;
	
	switch(state) {
	case TC_W4_SERVICE_RESULT:
		switch(hci_event_packet_get_type(packet)) {
		case GATT_EVENT_SERVICE_QUERY_RESULT:
			// store service (we expect only one)
			gatt_event_service_query_result_get_service(packet, &le_streamer_service);
			break;

		case GATT_EVENT_QUERY_COMPLETE:
			if (packet[4] != 0) {
				log_debug("SERVICE_QUERY_RESULT - Error status %x.\n", packet[4]);
				gap_disconnect(connection_handle);
				break;
			}

			// service query complete, look for characteristic
			state = TC_W4_CHARACTERISTIC_RX_RESULT;
			log_debug("Search for LE Streamer RX characteristic.\n");
			gatt_client_discover_characteristics_for_service_by_uuid128(handle_gatt_client_event, connection_handle, &le_streamer_service, le_streamer_characteristic_rx_uuid);
			break;

		default:
			break;
		}

		break;

	case TC_W4_CHARACTERISTIC_RX_RESULT:
		switch(hci_event_packet_get_type(packet)) {
		case GATT_EVENT_CHARACTERISTIC_QUERY_RESULT:
			gatt_event_characteristic_query_result_get_characteristic(packet, &le_streamer_characteristic_rx);
			break;

		case GATT_EVENT_QUERY_COMPLETE:
			if (packet[4] != 0) {
				log_debug("CHARACTERISTIC_QUERY_RESULT - Error status %x.\n", packet[4]);
				gap_disconnect(connection_handle);
				break;
			}

			// rx characteristiic found, look for tx characteristic
			state = TC_W4_CHARACTERISTIC_TX_RESULT;
			log_debug("Search for LE Streamer TX characteristic.\n");
			gatt_client_discover_characteristics_for_service_by_uuid128(handle_gatt_client_event, connection_handle, &le_streamer_service, le_streamer_characteristic_tx_uuid);
			break;

		default:
			break;
		}

		break;

	case TC_W4_CHARACTERISTIC_TX_RESULT:
		switch(hci_event_packet_get_type(packet)) {
		case GATT_EVENT_CHARACTERISTIC_QUERY_RESULT:
			gatt_event_characteristic_query_result_get_characteristic(packet, &le_streamer_characteristic_tx);
			break;

		case GATT_EVENT_QUERY_COMPLETE:
			if (packet[4] != 0) {
				log_debug("CHARACTERISTIC_QUERY_RESULT - Error status %x.\n", packet[4]);
				gap_disconnect(connection_handle);
				break;
			}

			// register handler for notifications
			listener_registered = 1;
			gatt_client_listen_for_characteristic_value_updates(&notification_listener, handle_gatt_client_event, connection_handle, &le_streamer_characteristic_tx);
			// setup tracking
			le_streamer_connection.name = 'A';
			le_streamer_connection.test_data_len = ATT_DEFAULT_MTU - 3;
			test_reset(&le_streamer_connection);
			gatt_client_get_mtu(connection_handle, &mtu);
			le_streamer_connection.test_data_len = btstack_min(mtu - 3, sizeof(le_streamer_connection.test_data));
			log_debug("%c: ATT MTU = %u => use test data of len %u\n", le_streamer_connection.name, mtu, le_streamer_connection.test_data_len);
			// enable notifications
#if (TEST_MODE & TEST_MODE_ENABLE_NOTIFICATIONS)
			log_debug("Start streaming - enable notify on test characteristic.\n");
			state = TC_W4_ENABLE_NOTIFICATIONS_COMPLETE;
			gatt_client_write_client_characteristic_configuration(handle_gatt_client_event, connection_handle,
			        &le_streamer_characteristic_tx, GATT_CLIENT_CHARACTERISTICS_CONFIGURATION_NOTIFICATION);
			break;
#endif
			state = TC_W4_TEST_DATA;
#if (TEST_MODE & TEST_MODE_WRITE_WITHOUT_RESPONSE)
			log_debug("Start streaming - request can send now.\n");
			gatt_client_request_can_write_without_response_event(handle_gatt_client_event, connection_handle);
#endif
			break;

		default:
			break;
		}

		break;

	case TC_W4_ENABLE_NOTIFICATIONS_COMPLETE:
		switch(hci_event_packet_get_type(packet)) {
		case GATT_EVENT_QUERY_COMPLETE:
			log_debug("Notifications enabled, status %02x\n", gatt_event_query_complete_get_status(packet));
			state = TC_W4_TEST_DATA;
#if (TEST_MODE & TEST_MODE_WRITE_WITHOUT_RESPONSE)
			log_debug("Start streaming - request can send now.\n");
			gatt_client_request_can_write_without_response_event(handle_gatt_client_event, connection_handle);
#endif
			break;

		default:
			break;
		}

		break;

	case TC_W4_TEST_DATA:
		switch(hci_event_packet_get_type(packet)) {
		case GATT_EVENT_NOTIFICATION:
			value_handle = little_endian_read_16(packet, 4);
			value_length = little_endian_read_16(packet, 6);
			value = &packet[8];
			log_debug("zhufeng:Notification handle 0x%04x, value: ", value_handle);
			log_debug_hexdump(value, value_length);
			//test_track_data(&le_streamer_connection, gatt_event_notification_get_value_length(packet));
			break;

		case GATT_EVENT_QUERY_COMPLETE:
			break;

		case GATT_EVENT_CAN_WRITE_WITHOUT_RESPONSE:
			//streamer(&le_streamer_connection);
			break;

		default:
			log_debug("Unknown packet type %x\n", hci_event_packet_get_type(packet));
			break;
		}

		break;

	default:
		log_debug("error\n");
		break;
	}
}

// Either connect to remote specified on command line or start scan for device with "LE Streamer" in advertisement
static void le_streamer_client_start(void) {

	log_debug("Start scanning!\n");
	state = TC_W4_SCAN_RESULT;
	gap_set_scan_parameters(0,0x0030, 0x0030);
	gap_start_scan();

}

static void hci_event_handler(uint8_t packet_type, uint16_t channel, uint8_t* packet, uint16_t size) {
	UNUSED(channel);
	UNUSED(size);

	if (packet_type != HCI_EVENT_PACKET) {
		return;
	}

	uint16_t conn_interval;
	uint8_t event = hci_event_packet_get_type(packet);

	switch (event) {
	case BTSTACK_EVENT_STATE:

		// BTstack activated, get started
		if (btstack_event_state_get_state(packet) == HCI_STATE_WORKING) {
			le_streamer_client_start();
			BtTestReady = 1;
		} else {
			state = TC_OFF;
		}

		break;

	case GAP_EVENT_ADVERTISING_REPORT:
		if (state != TC_W4_SCAN_RESULT) {
			return;
		}

		// check name in advertisement
		if (!advertisement_report_contains_name("LE", packet)) {
			return;
		}

		// store address and type
		gap_event_advertising_report_get_address(packet, le_streamer_addr);
		le_streamer_addr_type = gap_event_advertising_report_get_address_type(packet);
		// stop scanning, and connect to the device
		state = TC_W4_CONNECT;
		gap_stop_scan();
		log_debug("Stop scan. Connect to device with addr %s.\n", bd_addr_to_str(le_streamer_addr));
		gap_connect(le_streamer_addr,le_streamer_addr_type);
		break;

	case HCI_EVENT_LE_META:

		// wait for connection complete
		if (hci_event_le_meta_get_subevent_code(packet) !=  HCI_SUBEVENT_LE_CONNECTION_COMPLETE) {
			break;
		}

		if (state != TC_W4_CONNECT) {
			return;
		}

		connection_handle = hci_subevent_le_connection_complete_get_connection_handle(packet);
		// print connection parameters (without using float operations)
		conn_interval = hci_subevent_le_connection_complete_get_conn_interval(packet);
		log_debug("Connection Interval: %u.%02u ms\n", conn_interval * 125 / 100, 25 * (conn_interval & 3));
		log_debug("Connection Latency: %u\n", hci_subevent_le_connection_complete_get_conn_latency(packet));
		// initialize gatt client context with handle, and add it to the list of active clients
		// query primary services
		log_debug("Search for LE Streamer service.\n");
		state = TC_W4_SERVICE_RESULT;
		gatt_client_discover_primary_services_by_uuid128(handle_gatt_client_event, connection_handle, le_streamer_service_uuid);
		break;

	case HCI_EVENT_DISCONNECTION_COMPLETE:
		// unregister listener
		connection_handle = HCI_CON_HANDLE_INVALID;

		if (listener_registered) {
			listener_registered = 0;
			gatt_client_stop_listening_for_characteristic_value_updates(&notification_listener);
		}

		log_debug("Disconnected %s\n", bd_addr_to_str(le_streamer_addr));

		if (state == TC_OFF) {
			break;
		}

		//le_streamer_client_start();
		break;

	default:
		break;
	}
}

static void stdin_process(char c){
	
	log_debug("stdin process:%c", c);
	switch (c){
        case 'c':
			streamer(&le_streamer_connection);
		break;
		
		default:
		break;
	}
}

int btstack_main(int argc, const char* argv[]);
int btstack_main(int argc, const char* argv[]) {
	(void)argc;
	(void)argv;

	hci_event_callback_registration.callback = &hci_event_handler;
	hci_add_event_handler(&hci_event_callback_registration);
	l2cap_init();
	gatt_client_init();
	sm_init();
	sm_set_io_capabilities(IO_CAPABILITY_NO_INPUT_NO_OUTPUT);
	// use different connection parameters: conn interval min/max (* 1.25 ms), slave latency, supervision timeout, CE len min/max (* 0.6125 ms)
	// gap_set_connection_parameters(0x06, 0x06, 4, 1000, 0x01, 0x06 * 2);
	// turn on!
	hci_power_control(HCI_POWER_ON);
	
	while(1) {
		btstack_run_loop_embedded_execute_once();
		if(BtTestReady > 0) break;
	}
	
		extern int PortRecvs_std(unsigned char channel, unsigned char *pszBuf,unsigned short usRcvLen,unsigned short usTimeOut);
	char index[10];

	log_debug("pls input on sscom");
	while(1)
	{
		memset(index,0,sizeof(index));
		if(PortRecvs_std(0,&index, 10, 0) > 0)
		{
			stdin_process(index[0]);
		}
		btstack_run_loop_embedded_execute_once();
	}
	
	return 0;
}
/* EXAMPLE_END */
