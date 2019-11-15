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
#define LUMP_METALEN   (sizeof(struct lumpheader_t) + sizeof(struct lumpinfo_t) * 65535)

static struct sys_file g_lumpfile;

/* lump_open : opens the given file as the lump file we're using */
int lump_open(char *file)
{
	struct lumpheader_t header;

	g_lumpfile = sys_open(file);

	if (g_lumpfile.fd < 0) {
		return -1;
	}

	// TODO (brian)
	// only write a fresh header if the file didn't exist before this

	header.magic = LUMP_MAGIC;
	header.flags = 0;
	sys_timestamp(&header.ts_created, NULL);
	header.filelen = sizeof(header);
	header.lumps = 0;

	sys_write(g_lumpfile, 0, sizeof(header), &header);

	return 0; // return 0 on success
}

/* lump_close : closes the lump file */
int lump_close()
{
	return sys_close(g_lumpfile); // return 0 on success
}

/* lump_getlumpinfo : reads the given lump at index 'index' into the pointer */
int lump_getinfo(struct lumpinfo_t *info, u64 index)
{
	struct lumpheader_t header;
	size_t bytes, offset;
	u64 i;

	memset(info, 0, sizeof(*info));

	bytes = sys_read(g_lumpfile, 0, sizeof(header), &header);

	if (header.lumps < index) {
		return -1;
	}

	offset = sizeof(struct lumpheader_t) + sizeof(struct lumpinfo_t) * index;

	sys_read(g_lumpfile, offset, sizeof(struct lumpinfo_t), info);

	return 0;
}

/* lump_getnumentries : gets the number of entries for the given tag */
int lump_getnumentries(char *tag, u64 *entries)
{
	struct lumpinfo_t info;
	u64 i;
	int rc;

	for (i = 0; ; i++) {
		rc = lump_getinfo(&info, i);
		if (rc < 0) {
			return -1;
		}

		if (strncmp(tag, info.tag, sizeof(info.tag))) {
			break;
		}
	}

	*entries = i;

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
	size_t bytes, offset;
	u64 i;

	// we first have to read the header, to determine where we should write
	// our info record, then write that info record, followed up with
	// writing our data

	bytes = sys_read(g_lumpfile, 0, sizeof(header), &header);

	if (header.lumps == LUMP_MAXINFO) {
		return -1;
	}

	assert(strlen(tag) <= sizeof(info.tag));

	strncpy(info.tag, tag, strlen(tag));
	info.offset = sys_getsize(g_lumpfile);
	info.size = size;
	lump_getnumentries(tag, &info.entry);

	offset = sizeof(struct lumpheader_t) + sizeof(struct lumpinfo_t) * header.lumps;

	sys_write(g_lumpfile, offset, sizeof(struct lumpinfo_t), &info);

	sys_write(g_lumpfile, info.offset, size, src);

	if (entry) {
		*entry = info.entry;
	}

	return 0;
}

