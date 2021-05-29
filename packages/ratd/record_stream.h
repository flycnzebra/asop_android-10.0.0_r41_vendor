
#define HEADER_SIZE 1
#define MSG_LEN_BIT 14

struct RecordStream {
	int fd;
	size_t maxRecordLen;

	unsigned char *buffer;

	unsigned char *unconsumed;
	unsigned char *read_end;
	unsigned char *buffer_end;
};

struct RecordStream *record_stream_new(int fd, size_t maxRecordLen);
void record_stream_free(struct RecordStream *rs);
unsigned char * getEndOfRecord(unsigned char *p_begin, unsigned char *p_end);
void *getNextRecord(struct RecordStream *p_rs, size_t *p_outRecordLen);
int record_stream_get_next(struct RecordStream *p_rs, void ** p_outRecord,
		size_t *p_outRecordLen);
