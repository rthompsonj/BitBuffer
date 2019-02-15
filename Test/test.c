#include <stdio.h>
#include <limits.h>
#include "bitbuffer.h"

int main() {
	uint8_t* buffer;
	BitBuffer outbound;
	BitBuffer inbound;

	bitbuffer_create_capacity(&outbound, 128);
	bitbuffer_create_capacity(&inbound, 128);

	uint16_t peer = 999;
	bool accelerated = true;
	uint32_t speed = 9999999;
	uint8_t flag = UCHAR_MAX;
	uint16_t color = 512;
	int integer = -INT_MAX;

	bitbuffer_add_ushort(&outbound, peer);
	bitbuffer_add_bool(&outbound, accelerated);
	bitbuffer_add_uint(&outbound, speed);
	bitbuffer_add_byte(&outbound, flag);
	bitbuffer_add_ushort(&outbound, color);
	bitbuffer_add_int(&outbound, integer);

	buffer = malloc(bitbuffer_length(&outbound));

	int length = bitbuffer_to_array(&outbound, buffer, bitbuffer_length(&outbound));

	printf("Outbound length: %i bytes\n", length);

	bitbuffer_from_array(&inbound, buffer, length);

	printf("Inbound length: %i bytes\n", bitbuffer_length(&inbound));

	peer = bitbuffer_read_ushort(&inbound);
	accelerated = bitbuffer_read_bool(&inbound);
	speed = bitbuffer_read_uint(&inbound);
	flag = bitbuffer_read_byte(&inbound);
	color = bitbuffer_read_ushort(&inbound);
	integer = bitbuffer_read_int(&inbound);

	printf("Peer: %u, Accelerated: %d, Speed: %u, Flag: %u, Color: %u, Integer: %i\n", peer, accelerated, speed, flag, color, integer);
	printf("Is finished: %d\n", bitbuffer_is_finished(&inbound));

	free(buffer);

	bitbuffer_destroy(&outbound);
	bitbuffer_destroy(&inbound);
}