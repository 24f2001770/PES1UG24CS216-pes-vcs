#include "commit.h"
#include "tree.h"
#include "object.h"
#include "index.h"
#include "pes.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// =======================
// CREATE COMMIT
// =======================
int commit_create(const char *message, ObjectID *commit_id_out) {
    ObjectID tree_id;

    // STEP 1: build tree
    if (tree_from_index(&tree_id) != 0)
        return -1;

    // STEP 2: convert tree hash
    char tree_hex[HASH_HEX_SIZE + 1];
    hash_to_hex(&tree_id, tree_hex);

    // STEP 3: create commit content (simple format)
    char buffer[1024];
    int len = snprintf(buffer, sizeof(buffer),
        "tree %s\n"
        "author %s\n"
        "message %s\n",
        tree_hex,
        pes_author(),
        message
    );

    if (len <= 0) return -1;

    // STEP 4: write commit object
    if (object_write(OBJ_COMMIT, buffer, len, commit_id_out) != 0)
        return -1;

    // STEP 5: update HEAD
    char commit_hex[HASH_HEX_SIZE + 1];
    hash_to_hex(commit_id_out, commit_hex);

    FILE *f = fopen(".pes/HEAD", "w");
    if (!f) return -1;

    fprintf(f, "%s\n", commit_hex);
    fclose(f);

    return 0;
}

// =======================
// PARSE COMMIT
// =======================
int commit_parse(const void *data, size_t len, Commit *commit_out) {
    char *buf = malloc(len + 1);
    if (!buf) return -1;

    memcpy(buf, data, len);
    buf[len] = '\0';

    // initialize parent = zero
    memset(&commit_out->parent, 0, sizeof(ObjectID));

    char *line = strtok(buf, "\n");

    while (line) {
        if (strncmp(line, "tree ", 5) == 0) {
            hex_to_hash(line + 5, &commit_out->tree);
        }
        else if (strncmp(line, "parent ", 7) == 0) {
            hex_to_hash(line + 7, &commit_out->parent);
        }
        else if (strncmp(line, "author ", 7) == 0) {
            strncpy(commit_out->author, line + 7,
                    sizeof(commit_out->author) - 1);
            commit_out->author[sizeof(commit_out->author) - 1] = '\0';
        }
        else if (strncmp(line, "message ", 8) == 0) {
            strncpy(commit_out->message, line + 8,
                    sizeof(commit_out->message) - 1);
            commit_out->message[sizeof(commit_out->message) - 1] = '\0';
        }

        line = strtok(NULL, "\n");
    }

    commit_out->timestamp = 0; // not used in this simplified version

    free(buf);
    return 0;
}

// =======================
// WALK COMMITS (LOG)
// =======================
int commit_walk(commit_walk_fn callback, void *ctx) {
    FILE *f = fopen(".pes/HEAD", "r");
    if (!f) return -1;

    char hex[HASH_HEX_SIZE + 1];
    if (!fgets(hex, sizeof(hex), f)) {
        fclose(f);
        return -1;
    }
    fclose(f);

    hex[strcspn(hex, "\n")] = 0;

    ObjectID current;
    if (hex_to_hash(hex, &current) != 0)
        return -1;

    while (1) {
        void *data;
        size_t len;
        ObjectType type;

        if (object_read(&current, &type, &data, &len) != 0)
            break;

        if (type != OBJ_COMMIT) {
            free(data);
            break;
        }

        Commit commit;
        if (commit_parse(data, len, &commit) != 0) {
            free(data);
            break;
        }

        free(data);

        callback(&current, &commit, ctx);

        // stop if no parent (all zeros)
        int is_zero = 1;
        for (int i = 0; i < HASH_SIZE; i++) {
            if (commit.parent.hash[i] != 0) {
                is_zero = 0;
                break;
            }
        }

        if (is_zero)
            break;

        current = commit.parent;
    }

    return 0;
}
