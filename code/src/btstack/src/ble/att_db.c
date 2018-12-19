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

#define __BTSTACK_FILE__ "att_db.c"


#include <stdio.h>
#include <string.h>

#include "ble/att_db.h"
#include "ble/core.h"
#include "bluetooth.h"
#include "btstack_debug.h"
#include "btstack_util.h"

typedef enum {
	ATT_READ,
	ATT_WRITE,
} att_operation_t;

// Buetooth Base UUID 00000000-0000-1000-8000-00805F9B34FB in little endian
static const uint8_t bluetooth_base_uuid[] = { 0xfb, 0x34, 0x9b, 0x5f, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };


static int is_Bluetooth_Base_UUID(uint8_t const* uuid) {
	if (memcmp(&uuid[0],  &bluetooth_base_uuid[0], 12)) {
		return 0;
	}

	if (memcmp(&uuid[14], &bluetooth_base_uuid[14], 2)) {
		return 0;
	}

	return 1;
}

static uint16_t uuid16_from_uuid(uint16_t uuid_len, uint8_t* uuid) {
	if (uuid_len == 2) {
		return little_endian_read_16(uuid, 0);
	}

	if (!is_Bluetooth_Base_UUID(uuid)) {
		return 0;
	}

	return little_endian_read_16(uuid, 12);
}

// ATT Database

// new java-style iterator
typedef struct att_iterator {
	// private
	uint8_t const* att_ptr;
	// public
	uint16_t size;
	uint16_t flags;
	uint16_t handle;
	uint8_t  const* uuid;
	uint16_t value_len;
	uint8_t  const* value;
} att_iterator_t;

static void att_persistent_ccc_cache(att_iterator_t* it);

static uint8_t const* att_db = NULL;
static att_read_callback_t  att_read_callback  = NULL;
static att_write_callback_t att_write_callback = NULL;
static uint8_t  att_prepare_write_error_code   = 0;
static uint16_t att_prepare_write_error_handle = 0x0000;

// single cache for att_is_persistent_ccc - stores flags before write callback
static uint16_t att_persistent_ccc_handle;
static uint16_t att_persistent_ccc_uuid16;

static void att_iterator_init(att_iterator_t* it) {
	it->att_ptr = att_db;
}

static int att_iterator_has_next(att_iterator_t* it) {
	return it->att_ptr != NULL;
}

static void att_iterator_fetch_next(att_iterator_t* it) {
	it->size   = little_endian_read_16(it->att_ptr, 0);

	if (it->size == 0) {
		it->flags = 0;
		it->handle = 0;
		it->uuid = NULL;
		it->value_len = 0;
		it->value = NULL;
		it->att_ptr = NULL;
		return;
	}

	it->flags  = little_endian_read_16(it->att_ptr, 2);
	it->handle = little_endian_read_16(it->att_ptr, 4);
	it->uuid   = &it->att_ptr[6];

	// handle 128 bit UUIDs
	if (it->flags & ATT_PROPERTY_UUID128) {
		it->value_len = it->size - 22;
		it->value  = &it->att_ptr[22];
	} else {
		it->value_len = it->size - 8;
		it->value  = &it->att_ptr[8];
	}

	// advance AFTER setting values
	it->att_ptr += it->size;
}

static int att_iterator_match_uuid16(att_iterator_t* it, uint16_t uuid) {
	if (it->handle == 0) {
		return 0;
	}

	if (it->flags & ATT_PROPERTY_UUID128) {
		if (!is_Bluetooth_Base_UUID(it->uuid)) {
			return 0;
		}

		return little_endian_read_16(it->uuid, 12) == uuid;
	}

	return little_endian_read_16(it->uuid, 0)  == uuid;
}

static int att_iterator_match_uuid(att_iterator_t* it, uint8_t* uuid, uint16_t uuid_len) {
	if (it->handle == 0) {
		return 0;
	}

	// input: UUID16
	if (uuid_len == 2) {
		return att_iterator_match_uuid16(it, little_endian_read_16(uuid, 0));
	}

	// input and db: UUID128
	if (it->flags & ATT_PROPERTY_UUID128) {
		return memcmp(it->uuid, uuid, 16) == 0;
	}

	// input: UUID128, db: UUID16
	if (!is_Bluetooth_Base_UUID(uuid)) {
		return 0;
	}

	return little_endian_read_16(uuid, 12) == little_endian_read_16(it->uuid, 0);
}


static int att_find_handle(att_iterator_t* it, uint16_t handle) {
	if (handle == 0) {
		return 0;
	}

	att_iterator_init(it);

	while (att_iterator_has_next(it)) {
		att_iterator_fetch_next(it);

		if (it->handle != handle) {
			continue;
		}

		return 1;
	}

	return 0;
}

// experimental client API
uint16_t att_uuid_for_handle(uint16_t attribute_handle) {
	att_iterator_t it;
	int ok = att_find_handle(&it, attribute_handle);

	if (!ok) {
		return 0;
	}

	if (it.flags & ATT_PROPERTY_UUID128) {
		return 0;
	}

	return little_endian_read_16(it.uuid, 0);
}
// end of client API

static void att_update_value_len(att_iterator_t* it, hci_con_handle_t con_handle) {
	if ((it->flags & ATT_PROPERTY_DYNAMIC) == 0) {
		return;
	}

	it->value_len = (*att_read_callback)(con_handle, it->handle, 0, NULL, 0);
	return;
}

// copy attribute value from offset into buffer with given size
static int att_copy_value(att_iterator_t* it, uint16_t offset, uint8_t* buffer, uint16_t buffer_size, hci_con_handle_t con_handle) {
	// DYNAMIC
	if (it->flags & ATT_PROPERTY_DYNAMIC) {
		return (*att_read_callback)(con_handle, it->handle, offset, buffer, buffer_size);
	}

	// STATIC
	uint16_t bytes_to_copy = it->value_len - offset;

	if (bytes_to_copy > buffer_size) {
		bytes_to_copy = buffer_size;
	}

	memcpy(buffer, it->value, bytes_to_copy);
	return bytes_to_copy;
}

void att_set_db(uint8_t const* db) {
	// validate db version
	if (db == NULL) {
		return;
	}

	if (*db++ != ATT_DB_VERSION) {
		log_error("ATT DB version differs, please regenerate .h from .gatt file or update att_db_util.c");
		return;
	}

	att_db = db;
}

void att_set_read_callback(att_read_callback_t callback) {
	att_read_callback = callback;
}

void att_set_write_callback(att_write_callback_t callback) {
	att_write_callback = callback;
}

void att_dump_attributes(void) {
	att_iterator_t it;
	att_iterator_init(&it);
	uint8_t uuid128[16];

	while (att_iterator_has_next(&it)) {
		att_iterator_fetch_next(&it);

		if (it.handle == 0) {
			log_info("Handle: END");
			return;
		}

		log_info("Handle: 0x%04x, flags: 0x%04x, uuid: ", it.handle, it.flags);

		if (it.flags & ATT_PROPERTY_UUID128) {
			reverse_128(it.uuid, uuid128);
			log_info("%s", uuid128_to_str(uuid128));
		} else {
			log_info("%04x", little_endian_read_16(it.uuid, 0));
		}

		log_info(", value_len: %u, value: ", it.value_len);
		log_info_hexdump(it.value, it.value_len);
	}
}

static void att_prepare_write_reset(void) {
	att_prepare_write_error_code = 0;
	att_prepare_write_error_handle = 0x0000;
}

static void att_prepare_write_update_errors(uint8_t error_code, uint16_t handle) {
	// first ATT_ERROR_INVALID_ATTRIBUTE_VALUE_LENGTH has highest priority
	if (error_code == ATT_ERROR_INVALID_ATTRIBUTE_VALUE_LENGTH && error_code != att_prepare_write_error_code) {
		att_prepare_write_error_code = error_code;
		att_prepare_write_error_handle = handle;
		return;
	}

	// first ATT_ERROR_INVALID_OFFSET is next
	if (error_code == ATT_ERROR_INVALID_OFFSET && att_prepare_write_error_code == 0) {
		att_prepare_write_error_code = error_code;
		att_prepare_write_error_handle = handle;
		return;
	}
}

static uint16_t setup_error(uint8_t* response_buffer, uint16_t request, uint16_t handle, uint8_t error_code) {
	response_buffer[0] = ATT_ERROR_RESPONSE;
	response_buffer[1] = request;
	little_endian_store_16(response_buffer, 2, handle);
	response_buffer[4] = error_code;
	return 5;
}

static inline uint16_t setup_error_read_not_permitted(uint8_t* response_buffer, uint16_t request, uint16_t start_handle) {
	return setup_error(response_buffer, request, start_handle, ATT_ERROR_READ_NOT_PERMITTED);
}

static inline uint16_t setup_error_write_not_permitted(uint8_t* response_buffer, uint16_t request, uint16_t start_handle) {
	return setup_error(response_buffer, request, start_handle, ATT_ERROR_WRITE_NOT_PERMITTED);
}

static inline uint16_t setup_error_atribute_not_found(uint8_t* response_buffer, uint16_t request, uint16_t start_handle) {
	return setup_error(response_buffer, request, start_handle, ATT_ERROR_ATTRIBUTE_NOT_FOUND);
}

static inline uint16_t setup_error_invalid_handle(uint8_t* response_buffer, uint16_t request, uint16_t handle) {
	return setup_error(response_buffer, request, handle, ATT_ERROR_INVALID_HANDLE);
}

static inline uint16_t setup_error_invalid_offset(uint8_t* response_buffer, uint16_t request, uint16_t handle) {
	return setup_error(response_buffer, request, handle, ATT_ERROR_INVALID_OFFSET);
}

static uint8_t att_validate_security(att_connection_t* att_connection, att_operation_t operation, att_iterator_t* it) {
	int required_security_level = 0;

	switch (operation) {
	case ATT_READ:
		if (it->flags & ATT_PROPERTY_READ_PERMISSION_BIT_0) {
			required_security_level |= 1;
		}

		if (it->flags & ATT_PROPERTY_READ_PERMISSION_BIT_1) {
			required_security_level |= 2;
		}

		break;

	case ATT_WRITE:
		if (it->flags & ATT_PROPERTY_WRITE_PERMISSION_BIT_0) {
			required_security_level |= 1;
		}

		if (it->flags & ATT_PROPERTY_WRITE_PERMISSION_BIT_1) {
			required_security_level |= 2;
		}

		break;
	}

	int required_encryption_size = it->flags >> 12;

	if (required_encryption_size) {
		required_encryption_size++;  // store -1 to fit into 4 bit
	}

	log_info("att_validate_security. flags 0x%04x (=> security level %u, key size %u) authorized %u, authenticated %u, encryption_key_size %u",
	         it->flags, required_security_level, required_encryption_size, att_connection->authorized, att_connection->authenticated, att_connection->encryption_key_size);

	if ((required_security_level >= ATT_SECURITY_AUTHORIZED) && (att_connection->authorized == 0)) {
		return ATT_ERROR_INSUFFICIENT_AUTHORIZATION;
	}

	if ((required_security_level >= ATT_SECURITY_AUTHENTICATED) && (att_connection->authenticated == 0)) {
		return ATT_ERROR_INSUFFICIENT_AUTHENTICATION;
	}

	if (required_security_level >= ATT_SECURITY_ENCRYPTED) {
		if ((required_encryption_size > 0) && (att_connection->encryption_key_size == 0)) {
			return ATT_ERROR_INSUFFICIENT_ENCRYPTION;
		}

		if (required_encryption_size > att_connection->encryption_key_size) {
			return ATT_ERROR_INSUFFICIENT_ENCRYPTION_KEY_SIZE;
		}
	}

	return 0;
}

//
// MARK: ATT_EXCHANGE_MTU_REQUEST
//
static uint16_t handle_exchange_mtu_request(att_connection_t* att_connection, uint8_t* request_buffer,  uint16_t request_len,
        uint8_t* response_buffer) {
	UNUSED(request_len);
	uint16_t client_rx_mtu = little_endian_read_16(request_buffer, 1);

	// find min(local max mtu, remote mtu) and use as mtu for this connection
	if (client_rx_mtu < att_connection->max_mtu) {
		att_connection->mtu = client_rx_mtu;
	} else {
		att_connection->mtu = att_connection->max_mtu;
	}

	response_buffer[0] = ATT_EXCHANGE_MTU_RESPONSE;
	little_endian_store_16(response_buffer, 1, att_connection->mtu);
	return 3;
}


//
// MARK: ATT_FIND_INFORMATION_REQUEST
//
// TODO: handle other types then GATT_PRIMARY_SERVICE_UUID and GATT_SECONDARY_SERVICE_UUID
//
static uint16_t handle_find_information_request2(att_connection_t* att_connection, uint8_t* response_buffer, uint16_t response_buffer_size,
        uint16_t start_handle, uint16_t end_handle) {
	UNUSED(att_connection);
	log_info("ATT_FIND_INFORMATION_REQUEST: from %04X to %04X", start_handle, end_handle);
	uint8_t request_type = ATT_FIND_INFORMATION_REQUEST;

	if (start_handle > end_handle || start_handle == 0) {
		return setup_error_invalid_handle(response_buffer, request_type, start_handle);
	}

	uint16_t offset   = 1;
	uint16_t uuid_len = 0;
	att_iterator_t it;
	att_iterator_init(&it);

	while (att_iterator_has_next(&it)) {
		att_iterator_fetch_next(&it);

		if (!it.handle) {
			break;
		}

		if (it.handle > end_handle) {
			break;
		}

		if (it.handle < start_handle) {
			continue;
		}

		// log_info("Handle 0x%04x", it.handle);
		uint16_t this_uuid_len = (it.flags & ATT_PROPERTY_UUID128) ? 16 : 2;

		// check if value has same len as last one if not first result
		if (offset > 1) {
			if (this_uuid_len != uuid_len) {
				break;
			}
		}

		// first
		if (offset == 1) {
			uuid_len = this_uuid_len;
			// set format field
			response_buffer[offset] = (it.flags & ATT_PROPERTY_UUID128) ? 0x02 : 0x01;
			offset++;
		}

		// space?
		if (offset + 2 + uuid_len > response_buffer_size) {
			break;
		}

		// store
		little_endian_store_16(response_buffer, offset, it.handle);
		offset += 2;
		memcpy(response_buffer + offset, it.uuid, uuid_len);
		offset += uuid_len;
	}

	if (offset == 1) {
		return setup_error_atribute_not_found(response_buffer, request_type, start_handle);
	}

	response_buffer[0] = ATT_FIND_INFORMATION_REPLY;
	return offset;
}

static uint16_t handle_find_information_request(att_connection_t* att_connection, uint8_t* request_buffer,  uint16_t request_len,
        uint8_t* response_buffer, uint16_t response_buffer_size) {
	UNUSED(request_len);
	return handle_find_information_request2(att_connection, response_buffer, response_buffer_size, little_endian_read_16(request_buffer, 1), little_endian_read_16(request_buffer, 3));
}

//
// MARK: ATT_FIND_BY_TYPE_VALUE
//
// "Only attributes with attribute handles between and including the Starting Handle parameter
// and the Ending Handle parameter that match the requested attri- bute type and the attribute
// value that have sufficient permissions to allow reading will be returned" -> (1)
//
// TODO: handle other types then GATT_PRIMARY_SERVICE_UUID and GATT_SECONDARY_SERVICE_UUID
//
// NOTE: doesn't handle DYNAMIC values
// NOTE: only supports 16 bit UUIDs
//
static uint16_t handle_find_by_type_value_request2(att_connection_t* att_connection, uint8_t* response_buffer, uint16_t response_buffer_size,
        uint16_t start_handle, uint16_t end_handle,
        uint16_t attribute_type, uint16_t attribute_len, uint8_t* attribute_value) {
	UNUSED(att_connection);
	log_info("ATT_FIND_BY_TYPE_VALUE_REQUEST: from %04X to %04X, type %04X, value: ", start_handle, end_handle, attribute_type);
	log_info_hexdump(attribute_value, attribute_len);
	uint8_t request_type = ATT_FIND_BY_TYPE_VALUE_REQUEST;

	if (start_handle > end_handle || start_handle == 0) {
		return setup_error_invalid_handle(response_buffer, request_type, start_handle);
	}

	uint16_t offset      = 1;
	uint16_t in_group    = 0;
	uint16_t prev_handle = 0;
	att_iterator_t it;
	att_iterator_init(&it);

	while (att_iterator_has_next(&it)) {
		att_iterator_fetch_next(&it);

		if (it.handle && it.handle < start_handle) {
			continue;
		}

		if (it.handle > end_handle) {
			break;  // (1)
		}

		// close current tag, if within a group and a new service definition starts or we reach end of att db
		if (in_group &&
		        (it.handle == 0 || att_iterator_match_uuid16(&it, GATT_PRIMARY_SERVICE_UUID) || att_iterator_match_uuid16(&it, GATT_SECONDARY_SERVICE_UUID))) {
			log_info("End of group, handle 0x%04x", prev_handle);
			little_endian_store_16(response_buffer, offset, prev_handle);
			offset += 2;
			in_group = 0;

			// check if space for another handle pair available
			if (offset + 4 > response_buffer_size) {
				break;
			}
		}

		// keep track of previous handle
		prev_handle = it.handle;

		// does current attribute match
		if (it.handle && att_iterator_match_uuid16(&it, attribute_type) && attribute_len == it.value_len && memcmp(attribute_value, it.value, it.value_len) == 0) {
			log_info("Begin of group, handle 0x%04x", it.handle);
			little_endian_store_16(response_buffer, offset, it.handle);
			offset += 2;
			in_group = 1;
		}
	}

	if (offset == 1) {
		return setup_error_atribute_not_found(response_buffer, request_type, start_handle);
	}

	response_buffer[0] = ATT_FIND_BY_TYPE_VALUE_RESPONSE;
	return offset;
}

static uint16_t handle_find_by_type_value_request(att_connection_t* att_connection, uint8_t* request_buffer,  uint16_t request_len,
        uint8_t* response_buffer, uint16_t response_buffer_size) {
	int attribute_len = request_len - 7;
	return handle_find_by_type_value_request2(att_connection, response_buffer, response_buffer_size, little_endian_read_16(request_buffer, 1),
	        little_endian_read_16(request_buffer, 3), little_endian_read_16(request_buffer, 5), attribute_len, &request_buffer[7]);
}

//
// MARK: ATT_READ_BY_TYPE_REQUEST
//
static uint16_t handle_read_by_type_request2(att_connection_t* att_connection, uint8_t* response_buffer, uint16_t response_buffer_size,
        uint16_t start_handle, uint16_t end_handle,
        uint16_t attribute_type_len, uint8_t* attribute_type) {
	log_info("ATT_READ_BY_TYPE_REQUEST: from %04X to %04X, type: ", start_handle, end_handle);
	log_info_hexdump(attribute_type, attribute_type_len);
	uint8_t request_type = ATT_READ_BY_TYPE_REQUEST;

	if (start_handle > end_handle || start_handle == 0) {
		return setup_error_invalid_handle(response_buffer, request_type, start_handle);
	}

	uint16_t offset   = 1;
	uint16_t pair_len = 0;
	att_iterator_t it;
	att_iterator_init(&it);
	uint8_t error_code = 0;
	uint16_t first_matching_but_unreadable_handle = 0;
#ifdef ENABLE_ATT_DELAYED_READ_RESPONSE
	int read_request_pending = 0;
#endif

	while (att_iterator_has_next(&it)) {
		att_iterator_fetch_next(&it);

		if (!it.handle) {
			break;
		}

		if (it.handle < start_handle) {
			continue;
		}

		if (it.handle > end_handle) {
			break;  // (1)
		}

		// does current attribute match
		if (!att_iterator_match_uuid(&it, attribute_type, attribute_type_len)) {
			continue;
		}

		// skip handles that cannot be read but rembember that there has been at least one
		if ((it.flags & ATT_PROPERTY_READ) == 0) {
			if (first_matching_but_unreadable_handle == 0) {
				first_matching_but_unreadable_handle = it.handle;
			}

			continue;
		}

		// check security requirements
		error_code = att_validate_security(att_connection, ATT_READ, &it);

		if (error_code) {
			break;
		}

		att_update_value_len(&it, att_connection->con_handle);
#ifdef ENABLE_ATT_DELAYED_READ_RESPONSE

		if (it.value_len == ATT_READ_RESPONSE_PENDING) {
			read_request_pending = 1;
		}

		if (read_request_pending) {
			continue;
		}

#endif
		// check if value has same len as last one
		uint16_t this_pair_len = 2 + it.value_len;

		if (offset > 1) {
			if (pair_len != this_pair_len) {
				break;
			}
		}

		// first
		if (offset == 1) {
			pair_len = this_pair_len;
			response_buffer[offset] = pair_len;
			offset++;
		}

		// space?
		if (offset + pair_len > response_buffer_size) {
			if (offset > 2) {
				break;
			}

			it.value_len = response_buffer_size - 4;
			response_buffer[1] = 2 + it.value_len;
		}

		// store
		little_endian_store_16(response_buffer, offset, it.handle);
		offset += 2;
		uint16_t bytes_copied = att_copy_value(&it, 0, response_buffer + offset, it.value_len, att_connection->con_handle);
		offset += bytes_copied;
	}

#ifdef ENABLE_ATT_DELAYED_READ_RESPONSE

	if (read_request_pending) {
		return ATT_READ_RESPONSE_PENDING;
	}

#endif

	// at least one attribute could be read
	if (offset > 1) {
		response_buffer[0] = ATT_READ_BY_TYPE_RESPONSE;
		return offset;
	}

	// first attribute had an error
	if (error_code) {
		return setup_error(response_buffer, request_type, start_handle, error_code);
	}

	// no other errors, but all found attributes had been non-readable
	if (first_matching_but_unreadable_handle) {
		return setup_error_read_not_permitted(response_buffer, request_type, first_matching_but_unreadable_handle);
	}

	// attribute not found
	return setup_error_atribute_not_found(response_buffer, request_type, start_handle);
}

static uint16_t handle_read_by_type_request(att_connection_t* att_connection, uint8_t* request_buffer,  uint16_t request_len,
        uint8_t* response_buffer, uint16_t response_buffer_size) {
	int attribute_type_len;

	if (request_len <= 7) {
		attribute_type_len = 2;
	} else {
		attribute_type_len = 16;
	}

	return handle_read_by_type_request2(att_connection, response_buffer, response_buffer_size, little_endian_read_16(request_buffer, 1), little_endian_read_16(request_buffer, 3), attribute_type_len, &request_buffer[5]);
}

//
// MARK: ATT_READ_BY_TYPE_REQUEST
//
static uint16_t handle_read_request2(att_connection_t* att_connection, uint8_t* response_buffer, uint16_t response_buffer_size, uint16_t handle) {
	log_info("ATT_READ_REQUEST: handle %04x", handle);
	uint8_t request_type = ATT_READ_REQUEST;
	att_iterator_t it;
	int ok = att_find_handle(&it, handle);

	if (!ok) {
		return setup_error_invalid_handle(response_buffer, request_type, handle);
	}

	// check if handle can be read
	if ((it.flags & ATT_PROPERTY_READ) == 0) {
		return setup_error_read_not_permitted(response_buffer, request_type, handle);
	}

	// check security requirements
	uint8_t error_code = att_validate_security(att_connection, ATT_READ, &it);

	if (error_code) {
		return setup_error(response_buffer, request_type, handle, error_code);
	}

	att_update_value_len(&it, att_connection->con_handle);
#ifdef ENABLE_ATT_DELAYED_READ_RESPONSE

	if (it.value_len == ATT_READ_RESPONSE_PENDING) {
		return ATT_READ_RESPONSE_PENDING;
	}

#endif
	uint16_t offset   = 1;

	// limit data
	if (offset + it.value_len > response_buffer_size) {
		it.value_len = response_buffer_size - 1;
	}

	// store
	uint16_t bytes_copied = att_copy_value(&it, 0, response_buffer + offset, it.value_len, att_connection->con_handle);
	offset += bytes_copied;
	response_buffer[0] = ATT_READ_RESPONSE;
	return offset;
}

static uint16_t handle_read_request(att_connection_t* att_connection, uint8_t* request_buffer,  uint16_t request_len,
                                    uint8_t* response_buffer, uint16_t response_buffer_size) {
	UNUSED(request_len);
	return handle_read_request2(att_connection, response_buffer, response_buffer_size, little_endian_read_16(request_buffer, 1));
}

//
// MARK: ATT_READ_BLOB_REQUEST 0x0c
//
static uint16_t handle_read_blob_request2(att_connection_t* att_connection, uint8_t* response_buffer, uint16_t response_buffer_size, uint16_t handle, uint16_t value_offset) {
	log_info("ATT_READ_BLOB_REQUEST: handle %04x, offset %u", handle, value_offset);
	uint8_t request_type = ATT_READ_BLOB_REQUEST;
	att_iterator_t it;
	int ok = att_find_handle(&it, handle);

	if (!ok) {
		return setup_error_invalid_handle(response_buffer, request_type, handle);
	}

	// check if handle can be read
	if ((it.flags & ATT_PROPERTY_READ) == 0) {
		return setup_error_read_not_permitted(response_buffer, request_type, handle);
	}

	// check security requirements
	uint8_t error_code = att_validate_security(att_connection, ATT_READ, &it);

	if (error_code) {
		return setup_error(response_buffer, request_type, handle, error_code);
	}

	att_update_value_len(&it, att_connection->con_handle);
#ifdef ENABLE_ATT_DELAYED_READ_RESPONSE

	if (it.value_len == ATT_READ_RESPONSE_PENDING) {
		return ATT_READ_RESPONSE_PENDING;
	}

#endif

	if (value_offset > it.value_len) {
		return setup_error_invalid_offset(response_buffer, request_type, handle);
	}

	// limit data
	uint16_t offset   = 1;

	if (offset + it.value_len - value_offset > response_buffer_size) {
		it.value_len = response_buffer_size - 1 + value_offset;
	}

	// store
	uint16_t bytes_copied = att_copy_value(&it, value_offset, response_buffer + offset, it.value_len - value_offset, att_connection->con_handle);
	offset += bytes_copied;
	response_buffer[0] = ATT_READ_BLOB_RESPONSE;
	return offset;
}

static uint16_t handle_read_blob_request(att_connection_t* att_connection, uint8_t* request_buffer,  uint16_t request_len,
        uint8_t* response_buffer, uint16_t response_buffer_size) {
	UNUSED(request_len);
	return handle_read_blob_request2(att_connection, response_buffer, response_buffer_size, little_endian_read_16(request_buffer, 1), little_endian_read_16(request_buffer, 3));
}

//
// MARK: ATT_READ_MULTIPLE_REQUEST 0x0e
//
static uint16_t handle_read_multiple_request2(att_connection_t* att_connection, uint8_t* response_buffer, uint16_t response_buffer_size, uint16_t num_handles, uint8_t* handles) {
	log_info("ATT_READ_MULTIPLE_REQUEST: num handles %u", num_handles);
	uint8_t request_type = ATT_READ_MULTIPLE_REQUEST;
	// TODO: figure out which error to respond with
	// if (num_handles < 2){
	//     return setup_error(response_buffer, ATT_READ_MULTIPLE_REQUEST, handle, ???);
	// }
	uint16_t offset   = 1;
	int i;
	uint8_t error_code = 0;
	uint16_t handle = 0;
#ifdef ENABLE_ATT_DELAYED_READ_RESPONSE
	int read_request_pending = 0;
#endif

	for (i=0; i<num_handles; i++) {
		handle = little_endian_read_16(handles, i << 1);

		if (handle == 0) {
			return setup_error_invalid_handle(response_buffer, request_type, handle);
		}

		att_iterator_t it;
		int ok = att_find_handle(&it, handle);

		if (!ok) {
			return setup_error_invalid_handle(response_buffer, request_type, handle);
		}

		// check if handle can be read
		if ((it.flags & ATT_PROPERTY_READ) == 0) {
			error_code = ATT_ERROR_READ_NOT_PERMITTED;
			break;
		}

		// check security requirements
		error_code = att_validate_security(att_connection, ATT_READ, &it);

		if (error_code) {
			break;
		}

		att_update_value_len(&it, att_connection->con_handle);
#ifdef ENABLE_ATT_DELAYED_READ_RESPONSE

		if (it.value_len == ATT_READ_RESPONSE_PENDING) {
			read_request_pending = 1;
		}

		if (read_request_pending) {
			continue;
		}

#endif

		// limit data
		if (offset + it.value_len > response_buffer_size) {
			it.value_len = response_buffer_size - 1;
		}

		// store
		uint16_t bytes_copied = att_copy_value(&it, 0, response_buffer + offset, it.value_len, att_connection->con_handle);
		offset += bytes_copied;
	}

	if (error_code) {
		return setup_error(response_buffer, request_type, handle, error_code);
	}

	response_buffer[0] = ATT_READ_MULTIPLE_RESPONSE;
	return offset;
}
static uint16_t handle_read_multiple_request(att_connection_t* att_connection, uint8_t* request_buffer,  uint16_t request_len,
        uint8_t* response_buffer, uint16_t response_buffer_size) {
	int num_handles = (request_len - 1) >> 1;
	return handle_read_multiple_request2(att_connection, response_buffer, response_buffer_size, num_handles, &request_buffer[1]);
}

//
// MARK: ATT_READ_BY_GROUP_TYPE_REQUEST 0x10
//
// Only handles GATT_PRIMARY_SERVICE_UUID and GATT_SECONDARY_SERVICE_UUID
// Core v4.0, vol 3, part g, 2.5.3
// "The «Primary Service» and «Secondary Service» grouping types may be used in the Read By Group Type Request.
//  The «Characteristic» grouping type shall not be used in the ATT Read By Group Type Request."
//
// NOTE: doesn't handle DYNAMIC values
//
// NOTE: we don't check for security as PRIMARY and SECONDAY SERVICE definition shouldn't be protected
// Core 4.0, vol 3, part g, 8.1
// "The list of services and characteristics that a device supports is not considered private or
//  confidential information, and therefore the Service and Characteristic Discovery procedures
//  shall always be permitted. "
//
static uint16_t handle_read_by_group_type_request2(att_connection_t* att_connection, uint8_t* response_buffer, uint16_t response_buffer_size,
        uint16_t start_handle, uint16_t end_handle,
        uint16_t attribute_type_len, uint8_t* attribute_type) {
	UNUSED(att_connection);
	log_info("ATT_READ_BY_GROUP_TYPE_REQUEST: from %04X to %04X, buffer size %u, type: ", start_handle, end_handle, response_buffer_size);
	log_info_hexdump(attribute_type, attribute_type_len);
	uint8_t request_type = ATT_READ_BY_GROUP_TYPE_REQUEST;

	if (start_handle > end_handle || start_handle == 0) {
		return setup_error_invalid_handle(response_buffer, request_type, start_handle);
	}

	// assert UUID is primary or secondary service uuid
	uint16_t uuid16 = uuid16_from_uuid(attribute_type_len, attribute_type);

	if (uuid16 != GATT_PRIMARY_SERVICE_UUID && uuid16 != GATT_SECONDARY_SERVICE_UUID) {
		return setup_error(response_buffer, request_type, start_handle, ATT_ERROR_UNSUPPORTED_GROUP_TYPE);
	}

	uint16_t offset   = 1;
	uint16_t pair_len = 0;
	uint16_t in_group = 0;
	uint16_t group_start_handle = 0;
	uint8_t const* group_start_value = NULL;
	uint16_t prev_handle = 0;
	att_iterator_t it;
	att_iterator_init(&it);

	while (att_iterator_has_next(&it)) {
		att_iterator_fetch_next(&it);

		if (it.handle && it.handle < start_handle) {
			continue;
		}

		if (it.handle > end_handle) {
			break;  // (1)
		}

		// log_info("Handle 0x%04x", it.handle);
		// close current tag, if within a group and a new service definition starts or we reach end of att db
		if (in_group &&
		        (it.handle == 0 || att_iterator_match_uuid16(&it, GATT_PRIMARY_SERVICE_UUID) || att_iterator_match_uuid16(&it, GATT_SECONDARY_SERVICE_UUID))) {
			// log_info("End of group, handle 0x%04x, val_len: %u", prev_handle, pair_len - 4);
			little_endian_store_16(response_buffer, offset, group_start_handle);
			offset += 2;
			little_endian_store_16(response_buffer, offset, prev_handle);
			offset += 2;
			memcpy(response_buffer + offset, group_start_value, pair_len - 4);
			offset += pair_len - 4;
			in_group = 0;

			// check if space for another handle pair available
			if (offset + pair_len > response_buffer_size) {
				break;
			}
		}

		// keep track of previous handle
		prev_handle = it.handle;

		// does current attribute match
		// log_info("compare: %04x == %04x", *(uint16_t*) context->attribute_type, *(uint16_t*) uuid);
		if (it.handle && att_iterator_match_uuid(&it, attribute_type, attribute_type_len)) {
			// check if value has same len as last one
			uint16_t this_pair_len = 4 + it.value_len;

			if (offset > 1) {
				if (this_pair_len != pair_len) {
					break;
				}
			}

			// log_info("Begin of group, handle 0x%04x", it.handle);
			// first
			if (offset == 1) {
				pair_len = this_pair_len;
				response_buffer[offset] = this_pair_len;
				offset++;
			}

			group_start_handle = it.handle;
			group_start_value  = it.value;
			in_group = 1;
		}
	}

	if (offset == 1) {
		return setup_error_atribute_not_found(response_buffer, request_type, start_handle);
	}

	response_buffer[0] = ATT_READ_BY_GROUP_TYPE_RESPONSE;
	return offset;
}
static uint16_t handle_read_by_group_type_request(att_connection_t* att_connection, uint8_t* request_buffer,  uint16_t request_len,
        uint8_t* response_buffer, uint16_t response_buffer_size) {
	int attribute_type_len;

	if (request_len <= 7) {
		attribute_type_len = 2;
	} else {
		attribute_type_len = 16;
	}

	return handle_read_by_group_type_request2(att_connection, response_buffer, response_buffer_size, little_endian_read_16(request_buffer, 1), little_endian_read_16(request_buffer, 3), attribute_type_len, &request_buffer[5]);
}

//
// MARK: ATT_WRITE_REQUEST 0x12
static uint16_t handle_write_request(att_connection_t* att_connection, uint8_t* request_buffer,  uint16_t request_len,
                                     uint8_t* response_buffer, uint16_t response_buffer_size) {
	UNUSED(response_buffer_size);
	uint8_t request_type = ATT_WRITE_REQUEST;
	uint16_t handle = little_endian_read_16(request_buffer, 1);
	att_iterator_t it;
	int ok = att_find_handle(&it, handle);

	if (!ok) {
		return setup_error_invalid_handle(response_buffer, request_type, handle);
	}

	if (!att_write_callback) {
		return setup_error_write_not_permitted(response_buffer, request_type, handle);
	}

	if ((it.flags & ATT_PROPERTY_WRITE) == 0) {
		return setup_error_write_not_permitted(response_buffer, request_type, handle);
	}

	if ((it.flags & ATT_PROPERTY_DYNAMIC) == 0) {
		return setup_error_write_not_permitted(response_buffer, request_type, handle);
	}

	// check security requirements
	uint8_t error_code = att_validate_security(att_connection, ATT_WRITE, &it);

	if (error_code) {
		return setup_error(response_buffer, request_type, handle, error_code);
	}

	att_persistent_ccc_cache(&it);
	error_code = (*att_write_callback)(att_connection->con_handle, handle, ATT_TRANSACTION_MODE_NONE, 0, request_buffer + 3, request_len - 3);

	if (error_code) {
		return setup_error(response_buffer, request_type, handle, error_code);
	}

	response_buffer[0] = ATT_WRITE_RESPONSE;
	return 1;
}

//
// MARK: ATT_PREPARE_WRITE_REQUEST 0x16
static uint16_t handle_prepare_write_request(att_connection_t* att_connection, uint8_t* request_buffer,  uint16_t request_len,
        uint8_t* response_buffer, uint16_t response_buffer_size) {
	UNUSED(response_buffer_size);
	uint8_t request_type = ATT_PREPARE_WRITE_REQUEST;
	uint16_t handle = little_endian_read_16(request_buffer, 1);
	uint16_t offset = little_endian_read_16(request_buffer, 3);

	if (!att_write_callback) {
		return setup_error_write_not_permitted(response_buffer, request_type, handle);
	}

	att_iterator_t it;
	int ok = att_find_handle(&it, handle);

	if (!ok) {
		return setup_error_invalid_handle(response_buffer, request_type, handle);
	}

	if ((it.flags & ATT_PROPERTY_WRITE) == 0) {
		return setup_error_write_not_permitted(response_buffer, request_type, handle);
	}

	if ((it.flags & ATT_PROPERTY_DYNAMIC) == 0) {
		return setup_error_write_not_permitted(response_buffer, request_type, handle);
	}

	// check security requirements
	uint8_t error_code = att_validate_security(att_connection, ATT_WRITE, &it);

	if (error_code) {
		return setup_error(response_buffer, request_type, handle, error_code);
	}

	error_code = (*att_write_callback)(att_connection->con_handle, handle, ATT_TRANSACTION_MODE_ACTIVE, offset, request_buffer + 5, request_len - 5);

	switch (error_code) {
	case 0:
		break;

	case ATT_ERROR_INVALID_OFFSET:
	case ATT_ERROR_INVALID_ATTRIBUTE_VALUE_LENGTH:
		// postpone to execute write request
		att_prepare_write_update_errors(error_code, handle);
		break;

	default:
		return setup_error(response_buffer, request_type, handle, error_code);
	}

	// response: echo request
	memcpy(response_buffer, request_buffer, request_len);
	response_buffer[0] = ATT_PREPARE_WRITE_RESPONSE;
	return request_len;
}

/*
 * @brief transcation queue of prepared writes, e.g., after disconnect
 */
void att_clear_transaction_queue(att_connection_t* att_connection) {
	(*att_write_callback)(att_connection->con_handle, 0, ATT_TRANSACTION_MODE_CANCEL, 0, NULL, 0);
}

// MARK: ATT_EXECUTE_WRITE_REQUEST 0x18
// NOTE: security has been verified by handle_prepare_write_request
static uint16_t handle_execute_write_request(att_connection_t* att_connection, uint8_t* request_buffer,  uint16_t request_len,
        uint8_t* response_buffer, uint16_t response_buffer_size) {
	UNUSED(request_len);
	UNUSED(response_buffer_size);
	uint8_t request_type = ATT_EXECUTE_WRITE_REQUEST;

	if (request_buffer[1]) {
		// validate queued write
		if (att_prepare_write_error_code == 0) {
			att_prepare_write_error_code = (*att_write_callback)(att_connection->con_handle, 0, ATT_TRANSACTION_MODE_VALIDATE, 0, NULL, 0);
		}

		// deliver queued errors
		if (att_prepare_write_error_code) {
			att_clear_transaction_queue(att_connection);
			uint8_t  error_code = att_prepare_write_error_code;
			uint16_t handle     = att_prepare_write_error_handle;
			att_prepare_write_reset();
			return setup_error(response_buffer, request_type, handle, error_code);
		}

		att_write_callback(att_connection->con_handle, 0, ATT_TRANSACTION_MODE_EXECUTE, 0, NULL, 0);
	} else {
		att_clear_transaction_queue(att_connection);
	}

	response_buffer[0] = ATT_EXECUTE_WRITE_RESPONSE;
	return 1;
}

// MARK: ATT_WRITE_COMMAND 0x52
// Core 4.0, vol 3, part F, 3.4.5.3
// "No Error Response or Write Response shall be sent in response to this command"
static void handle_write_command(att_connection_t* att_connection, uint8_t* request_buffer,  uint16_t request_len,
                                 uint8_t* response_buffer, uint16_t response_buffer_size) {
	UNUSED(response_buffer);
	UNUSED(response_buffer_size);
	uint16_t handle = little_endian_read_16(request_buffer, 1);

	if (!att_write_callback) {
		return;
	}

	att_iterator_t it;
	int ok = att_find_handle(&it, handle);

	if (!ok) {
		return;
	}

	if ((it.flags & ATT_PROPERTY_DYNAMIC) == 0) {
		return;
	}

	if ((it.flags & ATT_PROPERTY_WRITE_WITHOUT_RESPONSE) == 0) {
		return;
	}

	if (att_validate_security(att_connection, ATT_WRITE, &it)) {
		return;
	}

	att_persistent_ccc_cache(&it);
	(*att_write_callback)(att_connection->con_handle, handle, ATT_TRANSACTION_MODE_NONE, 0, request_buffer + 3, request_len - 3);
}

// MARK: helper for ATT_HANDLE_VALUE_NOTIFICATION and ATT_HANDLE_VALUE_INDICATION
static uint16_t prepare_handle_value(att_connection_t* att_connection,
                                     uint16_t handle,
                                     uint8_t* value,
                                     uint16_t value_len,
                                     uint8_t* response_buffer) {
	little_endian_store_16(response_buffer, 1, handle);

	if (value_len > att_connection->mtu - 3) {
		value_len = att_connection->mtu - 3;
	}

	memcpy(&response_buffer[3], value, value_len);
	return value_len + 3;
}

// MARK: ATT_HANDLE_VALUE_NOTIFICATION 0x1b
uint16_t att_prepare_handle_value_notification(att_connection_t* att_connection,
        uint16_t handle,
        uint8_t* value,
        uint16_t value_len,
        uint8_t* response_buffer) {
	response_buffer[0] = ATT_HANDLE_VALUE_NOTIFICATION;
	return prepare_handle_value(att_connection, handle, value, value_len, response_buffer);
}

// MARK: ATT_HANDLE_VALUE_INDICATION 0x1d
uint16_t att_prepare_handle_value_indication(att_connection_t* att_connection,
        uint16_t handle,
        uint8_t* value,
        uint16_t value_len,
        uint8_t* response_buffer) {
	response_buffer[0] = ATT_HANDLE_VALUE_INDICATION;
	return prepare_handle_value(att_connection, handle, value, value_len, response_buffer);
}

// MARK: Dispatcher
uint16_t att_handle_request(att_connection_t* att_connection,
                            uint8_t* request_buffer,
                            uint16_t request_len,
                            uint8_t* response_buffer) {
	uint16_t response_len = 0;
	uint16_t response_buffer_size = att_connection->mtu;

	switch (request_buffer[0]) {
	case ATT_EXCHANGE_MTU_REQUEST:
		response_len = handle_exchange_mtu_request(att_connection, request_buffer, request_len, response_buffer);
		break;

	case ATT_FIND_INFORMATION_REQUEST:
		response_len = handle_find_information_request(att_connection, request_buffer, request_len,response_buffer, response_buffer_size);
		break;

	case ATT_FIND_BY_TYPE_VALUE_REQUEST:
		response_len = handle_find_by_type_value_request(att_connection, request_buffer, request_len, response_buffer, response_buffer_size);
		break;

	case ATT_READ_BY_TYPE_REQUEST:
		response_len = handle_read_by_type_request(att_connection, request_buffer, request_len, response_buffer, response_buffer_size);
		break;

	case ATT_READ_REQUEST:
		response_len = handle_read_request(att_connection, request_buffer, request_len, response_buffer, response_buffer_size);
		break;

	case ATT_READ_BLOB_REQUEST:
		response_len = handle_read_blob_request(att_connection, request_buffer, request_len, response_buffer, response_buffer_size);
		break;

	case ATT_READ_MULTIPLE_REQUEST:
		response_len = handle_read_multiple_request(att_connection, request_buffer, request_len, response_buffer, response_buffer_size);
		break;

	case ATT_READ_BY_GROUP_TYPE_REQUEST:
		response_len = handle_read_by_group_type_request(att_connection, request_buffer, request_len, response_buffer, response_buffer_size);
		break;

	case ATT_WRITE_REQUEST:
		response_len = handle_write_request(att_connection, request_buffer, request_len, response_buffer, response_buffer_size);
		break;

	case ATT_PREPARE_WRITE_REQUEST:
		response_len = handle_prepare_write_request(att_connection, request_buffer, request_len, response_buffer, response_buffer_size);
		break;

	case ATT_EXECUTE_WRITE_REQUEST:
		response_len = handle_execute_write_request(att_connection, request_buffer, request_len, response_buffer, response_buffer_size);
		break;

	case ATT_WRITE_COMMAND:
		handle_write_command(att_connection, request_buffer, request_len, response_buffer, response_buffer_size);
		break;
#ifdef ENABLE_LE_SIGNED_WRITE

	case ATT_SIGNED_WRITE_COMMAND:
		log_info("handle_signed_write_command preprocessed by att_server.c");
		break;
#endif

	default:
		log_info("Unhandled ATT Command: %02X, DATA: ", request_buffer[0]);
		log_info_hexdump(&request_buffer[9], request_len-9);
		break;
	}

	return response_len;
}

// returns 1 if service found. only primary service.
int gatt_server_get_get_handle_range_for_service_with_uuid16(uint16_t uuid16, uint16_t* start_handle, uint16_t* end_handle) {
	uint16_t in_group    = 0;
	uint16_t prev_handle = 0;
	uint8_t attribute_value[2];
	int attribute_len = sizeof(attribute_value);
	little_endian_store_16(attribute_value, 0, uuid16);
	att_iterator_t it;
	att_iterator_init(&it);

	while (att_iterator_has_next(&it)) {
		att_iterator_fetch_next(&it);
		int new_service_started = att_iterator_match_uuid16(&it, GATT_PRIMARY_SERVICE_UUID) || att_iterator_match_uuid16(&it, GATT_SECONDARY_SERVICE_UUID);

		// close current tag, if within a group and a new service definition starts or we reach end of att db
		if (in_group &&
		        (it.handle == 0 || new_service_started)) {
			*end_handle = prev_handle;
			return 1;
		}

		// keep track of previous handle
		prev_handle = it.handle;

		// check if found
		if (it.handle && new_service_started && attribute_len == it.value_len && memcmp(attribute_value, it.value, it.value_len) == 0) {
			*start_handle = it.handle;
			in_group = 1;
		}
	}

	return 0;
}

// returns 0 if not found
uint16_t gatt_server_get_value_handle_for_characteristic_with_uuid16(uint16_t start_handle, uint16_t end_handle, uint16_t uuid16) {
	att_iterator_t it;
	att_iterator_init(&it);

	while (att_iterator_has_next(&it)) {
		att_iterator_fetch_next(&it);

		if (it.handle && it.handle < start_handle) {
			continue;
		}

		if (it.handle > end_handle) {
			break;  // (1)
		}

		if (it.handle == 0) {
			break;
		}

		if (att_iterator_match_uuid16(&it, uuid16)) {
			return it.handle;
		}
	}

	return 0;
}

// returns 0 if not found
uint16_t gatt_server_get_client_configuration_handle_for_characteristic_with_uuid16(uint16_t start_handle, uint16_t end_handle, uint16_t uuid16) {
	att_iterator_t it;
	att_iterator_init(&it);
	int characteristic_found = 0;

	while (att_iterator_has_next(&it)) {
		att_iterator_fetch_next(&it);

		if (it.handle && it.handle < start_handle) {
			continue;
		}

		if (it.handle > end_handle) {
			break;  // (1)
		}

		if (it.handle == 0) {
			break;
		}

		if (att_iterator_match_uuid16(&it, uuid16)) {
			characteristic_found = 1;
			continue;
		}

		if (att_iterator_match_uuid16(&it, GATT_PRIMARY_SERVICE_UUID)
		        || att_iterator_match_uuid16(&it, GATT_SECONDARY_SERVICE_UUID)
		        || att_iterator_match_uuid16(&it, GATT_CHARACTERISTICS_UUID)) {
			if (characteristic_found) {
				break;
			}

			continue;
		}

		if (att_iterator_match_uuid16(&it, GATT_CLIENT_CHARACTERISTICS_CONFIGURATION)) {
			return it.handle;
		}
	}

	return 0;
}

// returns 1 if service found. only primary service.
int gatt_server_get_get_handle_range_for_service_with_uuid128(const uint8_t* uuid128, uint16_t* start_handle, uint16_t* end_handle) {
	uint16_t in_group    = 0;
	uint16_t prev_handle = 0;
	uint8_t attribute_value[16];
	int attribute_len = sizeof(attribute_value);
	reverse_128(uuid128, attribute_value);
	att_iterator_t it;
	att_iterator_init(&it);

	while (att_iterator_has_next(&it)) {
		att_iterator_fetch_next(&it);
		int new_service_started = att_iterator_match_uuid16(&it, GATT_PRIMARY_SERVICE_UUID) || att_iterator_match_uuid16(&it, GATT_SECONDARY_SERVICE_UUID);

		// close current tag, if within a group and a new service definition starts or we reach end of att db
		if (in_group &&
		        (it.handle == 0 || new_service_started)) {
			*end_handle = prev_handle;
			return 1;
		}

		// keep track of previous handle
		prev_handle = it.handle;

		// check if found
		if (it.handle && new_service_started && attribute_len == it.value_len && memcmp(attribute_value, it.value, it.value_len) == 0) {
			*start_handle = it.handle;
			in_group = 1;
		}
	}

	return 0;
}

// returns 0 if not found
uint16_t gatt_server_get_value_handle_for_characteristic_with_uuid128(uint16_t start_handle, uint16_t end_handle, const uint8_t* uuid128) {
	uint8_t attribute_value[16];
	reverse_128(uuid128, attribute_value);
	att_iterator_t it;
	att_iterator_init(&it);

	while (att_iterator_has_next(&it)) {
		att_iterator_fetch_next(&it);

		if (it.handle && it.handle < start_handle) {
			continue;
		}

		if (it.handle > end_handle) {
			break;  // (1)
		}

		if (it.handle == 0) {
			break;
		}

		if (att_iterator_match_uuid(&it, attribute_value, 16)) {
			return it.handle;
		}
	}

	return 0;
}

// returns 0 if not found
uint16_t gatt_server_get_client_configuration_handle_for_characteristic_with_uuid128(uint16_t start_handle, uint16_t end_handle, const uint8_t* uuid128) {
	uint8_t attribute_value[16];
	reverse_128(uuid128, attribute_value);
	att_iterator_t it;
	att_iterator_init(&it);
	int characteristic_found = 0;

	while (att_iterator_has_next(&it)) {
		att_iterator_fetch_next(&it);

		if (it.handle && it.handle < start_handle) {
			continue;
		}

		if (it.handle > end_handle) {
			break;  // (1)
		}

		if (it.handle == 0) {
			break;
		}

		if (att_iterator_match_uuid(&it, attribute_value, 16)) {
			characteristic_found = 1;
			continue;
		}

		if (att_iterator_match_uuid16(&it, GATT_PRIMARY_SERVICE_UUID)
		        || att_iterator_match_uuid16(&it, GATT_SECONDARY_SERVICE_UUID)
		        || att_iterator_match_uuid16(&it, GATT_CHARACTERISTICS_UUID)) {
			if (characteristic_found) {
				break;
			}

			continue;
		}

		if (att_iterator_match_uuid16(&it, GATT_CLIENT_CHARACTERISTICS_CONFIGURATION)) {
			return it.handle;
		}
	}

	return 0;
}


// 1-item cache to optimize query during write_callback
static void att_persistent_ccc_cache(att_iterator_t* it) {
	att_persistent_ccc_handle = it->handle;

	if (it->flags & ATT_PROPERTY_UUID128) {
		att_persistent_ccc_uuid16 = 0;
	} else {
		att_persistent_ccc_uuid16 = little_endian_read_16(it->uuid, 0);
	}
}

int att_is_persistent_ccc(uint16_t handle) {
	if (handle != att_persistent_ccc_handle) {
		att_iterator_t it;
		int ok = att_find_handle(&it, handle);

		if (!ok) {
			return 0;
		}

		att_persistent_ccc_cache(&it);
	}

	return att_persistent_ccc_uuid16 == GATT_CLIENT_CHARACTERISTICS_CONFIGURATION;
}

// att_read_callback helpers
uint16_t att_read_callback_handle_blob(const uint8_t* blob, uint16_t blob_size, uint16_t offset, uint8_t* buffer, uint16_t buffer_size) {
	if (buffer) {
		uint16_t bytes_to_copy = btstack_min(blob_size - offset, buffer_size);
		memcpy(buffer, &blob[offset], bytes_to_copy);
		return bytes_to_copy;
	} else {
		return blob_size;
	}
}

uint16_t att_read_callback_handle_little_endian_32(uint32_t value, uint16_t offset, uint8_t* buffer, uint16_t buffer_size) {
	uint8_t value_buffer[4];
	little_endian_store_32(value_buffer, 0, value);
	return att_read_callback_handle_blob(value_buffer, sizeof(value_buffer), offset, buffer, buffer_size);
}

uint16_t att_read_callback_handle_little_endian_16(uint16_t value, uint16_t offset, uint8_t* buffer, uint16_t buffer_size) {
	uint8_t value_buffer[2];
	little_endian_store_16(value_buffer, 0, value);
	return att_read_callback_handle_blob(value_buffer, sizeof(value_buffer), offset, buffer, buffer_size);
}

uint16_t att_read_callback_handle_byte(uint8_t value, uint16_t offset, uint8_t* buffer, uint16_t buffer_size) {
	uint8_t value_buffer[1];
	value_buffer[0] = value;
	return att_read_callback_handle_blob(value_buffer, sizeof(value_buffer), offset, buffer, buffer_size);
}
