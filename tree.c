#include "tree.h"
#include "index.h"
#include "object.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// build tree from index (flat, no directories)
int tree_from_index(ObjectID *id_out) {
    Index index;

    if (index_load(&index) != 0)
        return -1;

    if (index.count == 0)
        return -1;

    char buffer[4096];
    int offset = 0;

    for (int i = 0; i < index.count; i++) {
        char hex[HASH_HEX_SIZE + 1];
        hash_to_hex(&index.entries[i].hash, hex);

        offset += snprintf(buffer + offset, sizeof(buffer) - offset,
            "%o blob %s %s\n",
            index.entries[i].mode,
            hex,
            index.entries[i].path
        );
    }

    return object_write(OBJ_TREE, buffer, offset, id_out);
}
