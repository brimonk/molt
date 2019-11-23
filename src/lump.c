/*
 * Brian Chrzanowski
 * Fri Nov 15, 2019 15:27
 *
 * Lump System MK2
 *
 * TODO (brian)
 * 1. Handle moving everything in the file to make room for the lump table to
 *    grow. I.E. remove the LUMP_METALEN restriction.
 * 2. Check more return values in lump_read & lump_write & generally everywhere
 */

#include <string.h>
#include <assert.h>

#include "common.h"
#include "sys.h"
#include "lump.h"

#define LUMP_MAGIC     *(s32 *)"MOLT"
#define LUMP_MAXINFO   65535 // this makes the metadata 2MB
#define LUMP_METALEN   (sizeof(struct lumpheader_t) + sizeof(struct lumpinfo_t) * LUMP_MAXINFO)

static struct sys_file *g_lumpfile;

/* lump_open : opens the given file as the lump file we're using */
int lump_open(char *file)
{
	struct lumpheader_t header;

	g_lumpfile = sys_open(file);

	if (g_lumpfile == NULL) {
		return -1;
	}

	// TODO (brian)
	// only write a fresh header if the file didn't exist before this

	header.magic = LUMP_MAGIC;
	header.flags = 0;
	sys_timestamp(&header.ts_created, NULL);
	header.size = sizeof(header);
	header.lumps = 0;

	sys_write(g_lumpfile, 0, sizeof(header), &header);

	return 0; // return 0 on success
}

/* lump_close : closes the lump file */
int lump_close()
{
	return sys_close(g_lumpfile); // return 0 on success
}

/* lump_getheader : gets the lump system's header */
int lump_getheader(struct lumpheader_t *header)
{
	size_t bytes;

	bytes = sys_read(g_lumpfile, 0, sizeof(struct lumpheader_t), header);

	return bytes != sizeof(struct lumpheader_t);
}

/* lump_getlumpinfo : reads the given lumpinfo at index into the pointer */
int lump_getinfo(struct lumpinfo_t *info, u64 index)
{
	struct lumpheader_t header;
	size_t bytes, offset;
	u64 i;

	memset(info, 0, sizeof(*info));

	bytes = sys_read(g_lumpfile, 0, sizeof(header), &header);

	if (header.lumps <= index) {
		return -1;
	}

	offset = sizeof(struct lumpheader_t) + sizeof(struct lumpinfo_t) * index;

	bytes = sys_read(g_lumpfile, offset, sizeof(struct lumpinfo_t), info);

	return bytes != sizeof(struct lumpinfo_t);
}

/* lump_getnumentries : gets the number of entries for the given tag */
int lump_getnumentries(char *tag, u64 *entries)
{
	struct lumpinfo_t info;
	u64 i, j;
	int rc;

	*entries = 0;

	for (i = 0, j = 0; ; i++) {
		rc = lump_getinfo(&info, i);
		if (rc < 0) {
			break;
		}

		if (strncmp(tag, info.tag, sizeof(info.tag)) == 0) {
			j++;
		}
	}

	*entries = j;

	return 0;
}

/* lump_readsize : reads the size of the lump with the given tag and entry no */
int lump_readsize(char *tag, u64 entry, size_t *size)
{
	struct lumpinfo_t info;
	u64 i;
	int rc;

	for (i = 0; ; i++) {
		rc = lump_getinfo(&info, i);
		if (rc < 0) {
			return -1;
		}

		if (strncmp(tag, info.tag, sizeof(info.tag)) == 0 && entry == info.entry) {
			break;
		}
	}

	*size = info.size;

	return 0;
}

/* lump_read : reads a lump into the buffer */
int lump_read(char *tag, u64 entry, void *dst)
{
	struct lumpinfo_t info;
	u64 i;
	int rc;

	for (i = 0; ; i++) {
		rc = lump_getinfo(&info, i);
		if (rc < 0) {
			return -1;
		}

		if (strncmp(tag, info.tag, sizeof(info.tag)) == 0 && entry == info.entry) {
			break;
		}
	}

	return info.size == sys_read(g_lumpfile, info.offset, info.size, dst) ? 0 : -1;
}

/* lump_write : writes the given lump into the lump system */
int lump_write(char *tag, size_t size, void *src, u64 *entry)
{
	struct lumpheader_t header;
	struct lumpinfo_t info;
	size_t offset_data, offset_info;
	size_t bytes;
	u64 i, datasize;

	assert(strlen(tag) <= sizeof(info.tag));

	// we first have to read the header, to determine where we should write
	// our info record, then write that info record, followed up with
	// writing our data

	bytes = sys_read(g_lumpfile, 0, sizeof(header), &header);

	if (header.lumps == LUMP_MAXINFO) {
		return -1;
	}

	// walk the info table summing how many items we have
	// NOTE potential optimization is to use the file size
	for (i = 0, datasize = 0; i < header.lumps; i++) {
		offset_data = sizeof(struct lumpheader_t) + sizeof(struct lumpinfo_t) * i;
		sys_read(g_lumpfile, offset_data, sizeof(struct lumpinfo_t), &info);
		datasize += info.size;
	}

	offset_info = sizeof(struct lumpheader_t) + sizeof(struct lumpinfo_t) * header.lumps;
	offset_data = LUMP_METALEN + datasize;

	// fill out our info record
	memset(&info, 0, sizeof(info));

	strncpy(info.tag, tag, strlen(tag));
	info.offset = offset_data;
	info.size = size;
	lump_getnumentries(tag, &info.entry);

	sys_write(g_lumpfile, offset_info, sizeof(struct lumpinfo_t), &info);

	sys_write(g_lumpfile, info.offset, size, src);

	// don't forget to update our header
	header.size = offset_data + size;
	header.lumps++;
	sys_write(g_lumpfile, 0, sizeof(struct lumpheader_t), &header);

	if (entry) {
		*entry = info.entry;
	}

	return 0;
}

