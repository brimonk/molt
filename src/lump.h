#ifndef LUMP_H
#define LUMP_H

/*
 * Brian Chrzanowski
 * Fri Nov 15, 2019 15:03
 *
 * The Lump System MK2
 */

struct lumpheader_t {
	u32 magic;
	u32 flags;
	u64 ts_created;
	u64 filelen;
	u64 lumps; // how many lumpinfo_t's appear in the file, right after this
};

struct lumpinfo_t {
	char tag[8];
	u64  offset;
	u64  size;
	u64  entry;
};

/* lump_open : opens the given file as the lump file we're using */
int lump_open(char *file);

/* lump_close : closes the lump file */
int lump_close();

/* lump_getlumpinfo : reads the given lump at index 'index' into the pointer */
int lump_getinfo(struct lumpinfo_t *info, u64 index);

/* lump_getnumentries : gets the number of entries for the given tag */
int lump_getnumentries(char *tag, u64 *entries);

/* lump_readsize : reads the size of the lump with the given tag and entry no */
int lump_readsize(char *tag, u64 entry, size_t *size);

/* lump_read : reads a lump into the buffer */
int lump_read(char *tag, u64 entry, void *dst);

/* lump_write : writes the given lump into the lump system */
int lump_write(char *tag, size_t size, void *src, u64 *entry);

#endif // LUMP_H
