#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <errno.h>

#include <string.h>
#include <stdint.h>
#ifdef HAVE_WINSOCK
#include <winsock2.h>  
#else
#include <netinet/in.h>
#endif

#include "head.h"

#include "record_stream.h"
#define test

extern struct RecordStream *record_stream_new(int fd, size_t maxRecordLen) {
	struct RecordStream *ret;

	assert(maxRecordLen <= 0xffff);

	ret = (struct RecordStream *) calloc(1, sizeof(struct RecordStream));

	ret->fd = fd;
	ret->maxRecordLen = maxRecordLen;
	ret->buffer = (unsigned char *) malloc(maxRecordLen + HEADER_SIZE);
	memset(ret->buffer, 0, maxRecordLen + HEADER_SIZE);
	ret->unconsumed = ret->buffer;
	ret->read_end = ret->buffer;
	ret->buffer_end = ret->buffer + maxRecordLen + HEADER_SIZE;
	return ret;
}

extern void record_stream_free(struct RecordStream *rs) {
	free(rs->buffer);
	free(rs);
}


unsigned char * getEndOfRecord(unsigned char *p_begin, unsigned char *p_end) {
	size_t len;
	unsigned char * p_ret;
	if (p_end < p_begin + HEADER_SIZE) {
		return NULL;
	}
#ifndef test
	len = ntohl(*((uint16_t *)(p_begin + MSG_LEN_BIT)));

	p_ret = p_begin + HEADER_SIZE + len;
#else
	p_ret = p_end;
#endif
	if (p_end < p_ret) {
		return NULL;
	}
	return p_ret;
}

void *getNextRecord(struct RecordStream *p_rs, size_t *p_outRecordLen) {
    
	unsigned char *record_start, *record_end;
	record_end = getEndOfRecord(p_rs->unconsumed, p_rs->read_end);

	if (record_end != NULL) {
		record_start = p_rs->unconsumed;
		p_rs->unconsumed = record_end;
		*p_outRecordLen = record_end - record_start;
		return record_start;
	}
	return NULL;
}
int record_stream_get_next(struct RecordStream *p_rs, void ** p_outRecord,size_t *p_outRecordLen) {
	void *ret;

	ssize_t countRead;
	ret = getNextRecord(p_rs, p_outRecordLen);

	if (ret != NULL) {
		*p_outRecord = ret;
		return 0;
	}

	
	if (p_rs->unconsumed == p_rs->buffer && p_rs->read_end == p_rs->buffer_end) {
		
		assert(0);
		errno = EFBIG;
		return -1;
	}

	if (p_rs->unconsumed != p_rs->buffer) {
	
		size_t toMove;

		toMove = p_rs->read_end - p_rs->unconsumed;
		if (toMove) {
			memmove(p_rs->buffer, p_rs->unconsumed, toMove);
		}

		p_rs->read_end = p_rs->buffer + toMove;
		p_rs->unconsumed = p_rs->buffer;
	}

	countRead = read(p_rs->fd, p_rs->read_end,p_rs->buffer_end - p_rs->read_end);

	if (countRead <= 0) {
		
		*p_outRecord = NULL;		
		return countRead;
	}

	p_rs->read_end += countRead;

	ret = getNextRecord(p_rs, p_outRecordLen);
#ifdef test
	if(*p_outRecordLen != countRead)
	{

		MPCLOG(LLV_ERROR, "record stream proc err, proc_result = %d, resd_result = %d\n",*p_outRecordLen, countRead);
	}
#endif
	if (ret == NULL) {
		
		errno = EAGAIN;
		return -1;
	}
	*p_outRecord = ret;
	return 0;
}
