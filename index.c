#include "index.h"
#include "pes.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

// declare
int object_write(ObjectType type, const void *data, size_t len, ObjectID *id_out);

// ---------- FIND ----------
IndexEntry* index_find(Index *index, const char *path) {
    for (int i = 0; i < index->count; i++) {
        if (strcmp(index->entries[i].path, path) == 0)
            return &index->entries[i];
    }
    return NULL;
}

// ---------- LOAD ----------
int index_load(Index *index) {
    index->count = 0;

    FILE *f = fopen(".pes/index", "r");
    if (!f) return 0;

    while (index->count < MAX_INDEX_ENTRIES) {
        IndexEntry *e = &index->entries[index->count];
        char hash_hex[HASH_HEX_SIZE + 1];

        unsigned int size_tmp;
        long mtime_tmp;

        if (fscanf(f, "%o %64s %ld %u %255s",
                   &e->mode, hash_hex,
                   &mtime_tmp, &size_tmp,
                   e->path) != 5)
            break;

        hex_to_hash(hash_hex, &e->hash);
        e->mtime_sec = (time_t)mtime_tmp;
        e->size = (uint32_t)size_tmp;

        index->count++;
    }

    fclose(f);
    return 0;
}

// ---------- SAVE ----------
int index_save(const Index *index) {
    FILE *f = fopen(".pes/index.tmp", "w");
    if (!f) return -1;

    for (int i = 0; i < index->count; i++) {
        char hex[HASH_HEX_SIZE + 1];
        hash_to_hex(&index->entries[i].hash, hex);

        fprintf(f, "%o %s %ld %u %s\n",
                index->entries[i].mode,
                hex,
                (long)index->entries[i].mtime_sec,
                index->entries[i].size,
                index->entries[i].path);
    }

    fclose(f);
    return rename(".pes/index.tmp", ".pes/index");
}

// ---------- ADD ----------
int index_add(Index *index, const char *path) {
    memset(index, 0, sizeof(Index));
    index_load(index);

    FILE *f = fopen(path, "rb");
    if (!f) return -1;

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    rewind(f);

    char *data = malloc(size > 0 ? size : 1);
    if (!data) {
        fclose(f);
        return -1;
    }

    if (size > 0) {
        if (fread(data, 1, size, f) != (size_t)size) {
            free(data);
            fclose(f);
            return -1;
        }
    }

    fclose(f);

    ObjectID id;
    if (object_write(OBJ_BLOB, data, size, &id) != 0) {
        free(data);
        return -1;
    }

    free(data);

    struct stat st;
    if (stat(path, &st) != 0) return -1;

    // FIX: prevent duplicates
    IndexEntry *e = index_find(index, path);

    if (!e) {
        if (index->count >= MAX_INDEX_ENTRIES) return -1;
        e = &index->entries[index->count++];
    }

    e->mode = st.st_mode;
    e->hash = id;
    e->mtime_sec = st.st_mtime;
    e->size = st.st_size;

    strncpy(e->path, path, sizeof(e->path));
    e->path[sizeof(e->path) - 1] = '\0';

    return index_save(index);
}

// ---------- STATUS ----------
int index_status(const Index *index) {
    printf("Staged changes:\n");

    if (index->count == 0)
        printf("  (nothing to show)\n");
    else {
        for (int i = 0; i < index->count; i++)
            printf("  staged: %s\n", index->entries[i].path);
    }

    printf("\nUnstaged changes:\n");
    printf("  (nothing to show)\n");

    printf("\nUntracked files:\n");
    printf("  (nothing to show)\n");

    return 0;
}
