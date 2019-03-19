/*
 *  Copyright (c) 2019 Stanislav Denisov
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"), to deal
 *  in the Software without restriction, including without limitation the rights
 *  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *  copies of the Software, and to permit persons to whom the Software is
 *  furnished to do so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice shall be included in all
 *  copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 *  SOFTWARE.
 */

#ifndef BITBUFFER_H
#define BITBUFFER_H

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define BITBUFFER_VERSION_MAJOR 1
#define BITBUFFER_VERSION_MINOR 0
#define BITBUFFER_VERSION_PATCH 0

#define BITBUFFER_DEFAULT_CAPACITY 8
#define BITBUFFER_GROW_FACTOR 2
#define BITBUFFER_MIN_GROW 1

#define BITBUFFER_OK 0
#define BITBUFFER_ERROR -1
#define BITBUFFER_TOO_MANY_BITS_ERROR 500

#if defined(_WIN32) && defined(BITBUFFER_DLL)
	#define BITBUFFER_API __declspec(dllexport)
#else
	#define BITBUFFER_API extern
#endif

// API

#ifdef __cplusplus
extern "C" {
#endif

	typedef struct _BitBuffer {
		int readPosition;
		int nextPosition;
		uint32_t* chunks;
		int length;
	} BitBuffer;

	// Public API

	BITBUFFER_API int bitbuffer_create(BitBuffer*);

	BITBUFFER_API int bitbuffer_create_capacity(BitBuffer*, uint32_t);

	BITBUFFER_API int bitbuffer_destroy(BitBuffer*);

	BITBUFFER_API int bitbuffer_length(BitBuffer*);

	BITBUFFER_API bool bitbuffer_is_finished(BitBuffer*);

	BITBUFFER_API int bitbuffer_clear(BitBuffer*);

	BITBUFFER_API int bitbuffer_add(BitBuffer*, int, uint32_t);

	BITBUFFER_API int bitbuffer_read(BitBuffer*, int);

	BITBUFFER_API uint32_t bitbuffer_peek(BitBuffer*, int);

	BITBUFFER_API int bitbuffer_to_array(BitBuffer*, uint8_t*, int);

	BITBUFFER_API void bitbuffer_from_array(BitBuffer*, const uint8_t*, int);

	BITBUFFER_API int bitbuffer_add_bool(BitBuffer*, bool);

	BITBUFFER_API bool bitbuffer_read_bool(BitBuffer*);

	BITBUFFER_API bool bitbuffer_peek_bool(BitBuffer*);

	BITBUFFER_API int bitbuffer_add_byte(BitBuffer*, uint8_t);

	BITBUFFER_API uint8_t bitbuffer_read_byte(BitBuffer*);

	BITBUFFER_API uint8_t bitbuffer_peek_byte(BitBuffer*);

	BITBUFFER_API int bitbuffer_add_short(BitBuffer*, int16_t);

	BITBUFFER_API int16_t bitbuffer_read_short(BitBuffer*);

	BITBUFFER_API int16_t bitbuffer_peek_short(BitBuffer*);

	BITBUFFER_API int bitbuffer_add_ushort(BitBuffer*, uint16_t);

	BITBUFFER_API uint16_t bitbuffer_read_ushort(BitBuffer*);

	BITBUFFER_API uint16_t bitbuffer_peek_ushort(BitBuffer*);

	BITBUFFER_API int bitbuffer_add_int(BitBuffer*, int);

	BITBUFFER_API int bitbuffer_read_int(BitBuffer*);

	BITBUFFER_API int bitbuffer_peek_int(BitBuffer*);

	BITBUFFER_API int bitbuffer_add_uint(BitBuffer*, uint32_t);

	BITBUFFER_API uint32_t bitbuffer_read_uint(BitBuffer*);

	BITBUFFER_API uint32_t bitbuffer_peek_uint(BitBuffer*);

	// Private API

	static void bitbuffer_expand_array(BitBuffer*);

	static int bitbuffer_find_highest_bit_position(uint8_t);

#ifdef __cplusplus
}
#endif

#if defined(BITBUFFER_IMPLEMENTATION) && !defined(BITBUFFER_IMPLEMENTATION_DONE)
#define BITBUFFER_IMPLEMENTATION_DONE 1
#ifdef __cplusplus
extern "C" {
#endif

	// Functions

	int bitbuffer_create(BitBuffer* bitbuffer) {
		return bitbuffer_create_capacity(bitbuffer, BITBUFFER_DEFAULT_CAPACITY);
	}

	int bitbuffer_create_capacity(BitBuffer* bitbuffer, uint32_t capacity) {
		assert(bitbuffer != NULL);

		if (bitbuffer == NULL)
			return BITBUFFER_ERROR;

		bitbuffer->readPosition = 0;
		bitbuffer->nextPosition = 0;
		bitbuffer->chunks = malloc(capacity * sizeof(uint32_t));
		bitbuffer->length = capacity;

		return bitbuffer->chunks != NULL ? BITBUFFER_OK : BITBUFFER_ERROR;
	}

	int bitbuffer_destroy(BitBuffer* bitbuffer) {
		assert(bitbuffer != NULL);

		if (bitbuffer == NULL)
			return BITBUFFER_ERROR;

		bitbuffer_clear(bitbuffer);

		free(bitbuffer->chunks);

		bitbuffer->chunks = NULL;

		return BITBUFFER_OK;
	}

	int bitbuffer_length(BitBuffer* bitbuffer) {
		assert(bitbuffer != NULL);

		return (bitbuffer->nextPosition >> 3) + 1;
	}

	bool bitbuffer_is_finished(BitBuffer* bitbuffer) {
		assert(bitbuffer != NULL);

		return bitbuffer->nextPosition == bitbuffer->readPosition;
	}

	int bitbuffer_clear(BitBuffer* bitbuffer) {
		assert(bitbuffer != NULL);

		if (bitbuffer == NULL)
			return BITBUFFER_ERROR;

		bitbuffer->nextPosition = 0;
		bitbuffer->readPosition = 0;
		bitbuffer->length = 0;

		return BITBUFFER_OK;
	}

	int bitbuffer_add(BitBuffer* bitbuffer, int numBits, uint32_t value) {
		assert(bitbuffer != NULL);
		assert(numBits >= 0);
		assert(numBits <= 32);

		if (bitbuffer == NULL)
			return BITBUFFER_ERROR;

		if (numBits > 32)
			return BITBUFFER_TOO_MANY_BITS_ERROR;

		int index = bitbuffer->nextPosition >> 5;
		int used = bitbuffer->nextPosition & 0x0000001E;

		if ((index + 1) >= bitbuffer->length)
			bitbuffer_expand_array(bitbuffer);

		uint64_t chunkMask = ((1UL << used) - 1);
		uint64_t scratch = bitbuffer->chunks[index] & chunkMask;
		uint64_t result = scratch | ((uint64_t)value << used);

		bitbuffer->chunks[index] = (uint32_t)result;
		bitbuffer->chunks[index + 1] = (uint32_t)(result >> 32);
		bitbuffer->nextPosition += numBits;

		return BITBUFFER_OK;
	}

	int bitbuffer_read(BitBuffer* bitbuffer, int numBits) {
		uint32_t result = bitbuffer_peek(bitbuffer, numBits);

		bitbuffer->readPosition += numBits;

		return result;
	}

	uint32_t bitbuffer_peek(BitBuffer* bitbuffer, int numBits) {
		assert(bitbuffer != NULL);
		assert(numBits >= 0);
		assert(numBits <= 32);

		int index = bitbuffer->readPosition >> 5;
		int used = bitbuffer->readPosition & 0x0000001E;

		uint64_t chunkMask = ((1UL << numBits) - 1) << used;
		uint64_t scratch = (uint64_t)bitbuffer->chunks[index];

		if ((index + 1) < bitbuffer->length)
			scratch |= (uint64_t)bitbuffer->chunks[index + 1] << 32;

		uint64_t result = (scratch & chunkMask) >> used;

		return (uint32_t)result;
	}

	int bitbuffer_to_array(BitBuffer* bitbuffer, uint8_t* data, int length) {
		assert(bitbuffer != NULL);
		assert(data != NULL);
		assert(length > 0);

		bitbuffer_add(bitbuffer, 1, 1);

		int numChunks = (bitbuffer->nextPosition >> 5) + 1;

		for (int i = 0; i < numChunks; i++) {
			int dataIdx = i * 4;
			uint32_t chunk = bitbuffer->chunks[i];

			if (dataIdx < length)
				data[dataIdx] = (uint8_t)(chunk);

			if (dataIdx + 1 < length)
				data[dataIdx + 1] = (uint8_t)(chunk >> 8);

			if (dataIdx + 2 < length)
				data[dataIdx + 2] = (uint8_t)(chunk >> 16);

			if (dataIdx + 3 < length)
				data[dataIdx + 3] = (uint8_t)(chunk >> 24);
		}

		return bitbuffer_length(bitbuffer);
	}

	void bitbuffer_from_array(BitBuffer* bitbuffer, const uint8_t* data, int length) {
		assert(bitbuffer != NULL);
		assert(data != NULL);
		assert(length > 0);

		int numChunks = (length / 4) + 1;

		if (bitbuffer->length < numChunks) {
			int newLength = numChunks * sizeof(uint32_t);

			bitbuffer->chunks = realloc(bitbuffer->chunks, newLength);
			bitbuffer->length = newLength;
		}

		for (int i = 0; i < numChunks; i++) {
			int dataIdx = i * 4;
			uint32_t chunk = 0;

			if (dataIdx < length)
				chunk = (uint32_t)data[dataIdx];

			if (dataIdx + 1 < length)
				chunk = chunk | (uint32_t)data[dataIdx + 1] << 8;

			if (dataIdx + 2 < length)
				chunk = chunk | (uint32_t)data[dataIdx + 2] << 16;

			if (dataIdx + 3 < length)
				chunk = chunk | (uint32_t)data[dataIdx + 3] << 24;

			bitbuffer->chunks[i] = chunk;
		}

		int positionInByte = bitbuffer_find_highest_bit_position(data[length - 1]);

		bitbuffer->nextPosition = ((length - 1) * 8) + positionInByte;
		bitbuffer->readPosition = 0;
	}

	inline int bitbuffer_add_bool(BitBuffer* bitbuffer, bool value) {
		assert(bitbuffer != NULL);

		return bitbuffer_add(bitbuffer, 1, value ? 1U : 0U);
	}

	inline bool bitbuffer_read_bool(BitBuffer* bitbuffer) {
		assert(bitbuffer != NULL);

		return bitbuffer_read(bitbuffer, 1) > 0;
	}

	inline bool bitbuffer_peek_bool(BitBuffer* bitbuffer) {
		assert(bitbuffer != NULL);

		return bitbuffer_peek(bitbuffer, 1) > 0;
	}

	inline int bitbuffer_add_byte(BitBuffer* bitbuffer, uint8_t value) {
		assert(bitbuffer != NULL);

		return bitbuffer_add(bitbuffer, 8, value);
	}

	inline uint8_t bitbuffer_read_byte(BitBuffer* bitbuffer) {
		assert(bitbuffer != NULL);

		return (uint8_t)bitbuffer_read(bitbuffer, 8);
	}

	inline uint8_t bitbuffer_peek_byte(BitBuffer* bitbuffer) {
		assert(bitbuffer != NULL);

		return (uint8_t)bitbuffer_peek(bitbuffer, 8);
	}

	inline int bitbuffer_add_short(BitBuffer* bitbuffer, int16_t value) {
		assert(bitbuffer != NULL);

		return bitbuffer_add_int(bitbuffer, value);
	}

	inline int16_t bitbuffer_read_short(BitBuffer* bitbuffer) {
		assert(bitbuffer != NULL);

		return (int16_t)bitbuffer_read_int(bitbuffer);
	}

	inline int16_t bitbuffer_peek_short(BitBuffer* bitbuffer) {
		assert(bitbuffer != NULL);

		return (int16_t)bitbuffer_peek_int(bitbuffer);
	}

	inline int bitbuffer_add_ushort(BitBuffer* bitbuffer, uint16_t value) {
		assert(bitbuffer != NULL);

		return bitbuffer_add_uint(bitbuffer, value);
	}

	inline uint16_t bitbuffer_read_ushort(BitBuffer* bitbuffer) {
		assert(bitbuffer != NULL);

		return (uint16_t)bitbuffer_read_uint(bitbuffer);
	}

	inline uint16_t bitbuffer_peek_ushort(BitBuffer* bitbuffer) {
		assert(bitbuffer != NULL);

		return (uint16_t)bitbuffer_peek_uint(bitbuffer);
	}

	inline int bitbuffer_add_int(BitBuffer* bitbuffer, int value) {
		assert(bitbuffer != NULL);

		uint32_t zigzag = (uint32_t)((value << 1) ^ (value >> 31));

		return bitbuffer_add_uint(bitbuffer, zigzag);
	}

	inline int bitbuffer_read_int(BitBuffer* bitbuffer) {
		assert(bitbuffer != NULL);

		uint32_t value = bitbuffer_read_uint(bitbuffer);
		int zagzig = (int)((value >> 1) ^ (-(int)(value & 1)));

		return zagzig;
	}

	inline int bitbuffer_peek_int(BitBuffer* bitbuffer) {
		assert(bitbuffer != NULL);

		uint32_t value = bitbuffer_peek_uint(bitbuffer);
		int zagzig = (int)((value >> 1) ^ (-(int)(value & 1)));

		return zagzig;
	}

	inline int bitbuffer_add_uint(BitBuffer* bitbuffer, uint32_t value) {
		assert(bitbuffer != NULL);

		int status = BITBUFFER_OK;
		uint32_t buffer = 0x0u;

		do {
			buffer = value & 0x7Fu;
			value >>= 7;

			if (value > 0)
				buffer |= 0x80u;

			status = bitbuffer_add(bitbuffer, 8, buffer);
		}

		while (value > 0);

		return status;
	}

	inline uint32_t bitbuffer_read_uint(BitBuffer* bitbuffer) {
		assert(bitbuffer != NULL);

		uint32_t buffer = 0x0u;
		uint32_t value = 0x0u;
		int shift = 0;

		do {
			buffer = bitbuffer_read(bitbuffer, 8);
			value |= (buffer & 0x7Fu) << shift;
			shift += 7;
		}

		while ((buffer & 0x80u) > 0);

		return value;
	}

	inline uint32_t bitbuffer_peek_uint(BitBuffer* bitbuffer) {
		assert(bitbuffer != NULL);

		int tempPosition = bitbuffer->readPosition;
		uint32_t value = bitbuffer_read_uint(bitbuffer);

		bitbuffer->readPosition = tempPosition;

		return value;
	}

	static void bitbuffer_expand_array(BitBuffer* bitbuffer) {
		assert(bitbuffer != NULL);

		int newCapacity = (bitbuffer->length * BITBUFFER_GROW_FACTOR) + BITBUFFER_MIN_GROW;
		uint32_t* oldChunks = bitbuffer->chunks;
		uint32_t* newChunks = malloc(newCapacity * sizeof(uint32_t));

		memcpy(newChunks, oldChunks, bitbuffer->length * sizeof(uint32_t));

		bitbuffer->chunks = newChunks;
		bitbuffer->length = newCapacity;

		free(oldChunks);
	}

	static int bitbuffer_find_highest_bit_position(uint8_t data) {
		int shiftCount = 0;

		while (data > 0) {
			data >>= 1;
			shiftCount++;
		}

		return shiftCount;
	}

#ifdef __cplusplus
}
#endif

#endif // BITBUFFER_IMPLEMENTATION

#endif // BITBUFFER_H
