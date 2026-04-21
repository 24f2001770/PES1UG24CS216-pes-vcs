#include "pes.h"
#include "index.h"
#include "commit.h"
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

void cmd_init(void) {
    mkdir(PES_DIR, 0755);
    mkdir(OBJECTS_DIR, 0755);
    mkdir(REFS_DIR, 0755);

    FILE *f = fopen(HEAD_FILE, "w");
    if (f) {
        fprintf(f, "ref: refs/heads/main\n");
        fclose(f);
    }

    printf("Initialized empty PES repository in %s/\n", PES_DIR);
}

void cmd_add(int argc, char *argv[]) {
    Index idx;

    if (index_load(&idx) != 0) {
        printf("error: failed to load index\n");
        return;
    }

    for (int i = 2; i < argc; i++) {
        index_add(&idx, argv[i]);
    }
}

void cmd_status(void) {
    Index idx;

    if (index_load(&idx) != 0) {
        printf("error: failed to load index\n");
        return;
    }

    index_status(&idx);
}

void cmd_commit(int argc, char *argv[]) {
    if (argc < 4 || strcmp(argv[2], "-m") != 0) {
        printf("error: commit requires -m \"message\"\n");
        return;
    }

    ObjectID id;

    if (commit_create(argv[3], &id) != 0) {
        printf("error: commit failed\n");
        return;
    }

    char hex[HASH_HEX_SIZE + 1];
    hash_to_hex(&id, hex);

    printf("Committed: %s\n", hex);
}

static void print_commit(const ObjectID *id, const Commit *commit, void *ctx) {
    (void)ctx;

    char hex[HASH_HEX_SIZE + 1];
    hash_to_hex(id, hex);

    printf("commit %s\n", hex);
    printf("Author: %s\n", commit->author);
    printf("Date:   %llu\n", (unsigned long long)commit->timestamp);
    printf("\n    %s\n\n", commit->message);
}

void cmd_log(void) {
    if (commit_walk(print_commit, NULL) != 0) {
        printf("No commits yet.\n");
    }
}
int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: pes <command>\n");
        return 1;
    }

    if (strcmp(argv[1], "init") == 0) cmd_init();
    else if (strcmp(argv[1], "add") == 0) cmd_add(argc, argv);
    else if (strcmp(argv[1], "status") == 0) cmd_status();
    else if (strcmp(argv[1], "commit") == 0) cmd_commit(argc, argv);
    else if (strcmp(argv[1], "log") == 0) cmd_log();
    else printf("Unknown command\n");

    return 0;
}
