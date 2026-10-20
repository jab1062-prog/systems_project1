#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

typedef struct {
    char *word;
    size_t count;
} Entry;

typedef struct {
    Entry *entries;
    size_t length, capacity;
} Table;

static void fail(const char *message)
{
    perror(message);
    exit(EXIT_FAILURE);
}

static void *allocate(size_t size)
{
    void *p = malloc(size);
    if (!p) fail("malloc");
    return p;
}

/* Keep the old pointer until realloc succeeds (September 16). */
static void *grow(void *p, size_t *capacity, size_t element_size)
{
    size_t next = *capacity ? *capacity : 32;
    void *replacement;
    if (*capacity) {
        if (*capacity > SIZE_MAX / 2) { errno = EOVERFLOW; fail("capacity"); }
        next = *capacity * 2;
    }
    if (next > SIZE_MAX / element_size) { errno = EOVERFLOW; fail("capacity"); }
    replacement = realloc(p, next * element_size);
    if (!replacement) fail("realloc");
    *capacity = next;
    return replacement;
}

static int letter(unsigned char c)
{
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}

static void add_word(Table *table, const char *word)
{
    size_t i;
    for (i = 0; i < table->length; ++i) {
        if (strcmp(table->entries[i].word, word) == 0) {
            if (table->entries[i].count == SIZE_MAX) {
                errno = EOVERFLOW;
                fail("word count");
            }
            ++table->entries[i].count;
            return;
        }
    }
    if (table->length == table->capacity)
        table->entries = grow(table->entries, &table->capacity, sizeof(Entry));
    table->entries[i].word = allocate(strlen(word) + 1);
    strcpy(table->entries[i].word, word);
    table->entries[i].count = 1;
    ++table->length;
}

static void append(char **word, size_t *length, size_t *capacity, char c)
{
    /* Always leave a byte for the string terminator. */
    if (*length >= *capacity - 1)
        *word = grow(*word, capacity, 1);
    (*word)[(*length)++] = c;
}

static void finish(Table *table, char *word, size_t *length)
{
    if (*length) {
        word[*length] = '\0';
        add_word(table, word);
        *length = 0;
    }
}

static void read_file(Table *table, const char *path)
{
    unsigned char buffer[4096];
    size_t capacity = 32, length = 0;
    char *word = allocate(capacity);
    int pending_dash = 0;
    int fd = open(path, O_RDONLY);
    ssize_t bytes;
    if (fd < 0) fail(path);
    for (;;) {
        size_t i;
        bytes = read(fd, buffer, sizeof(buffer));
        if (bytes < 0) {
            if (errno == EINTR) continue;
            fail(path);
        }
        if (bytes == 0) break;
        for (i = 0; i < (size_t)bytes; ++i) {
            unsigned char c = buffer[i];
            /* Delay '-' until its right neighbor is known, even across reads. */
            if (pending_dash) {
                if (letter(c)) append(&word, &length, &capacity, '-');
                else finish(table, word, &length);
                pending_dash = 0;
            }
            if (letter(c) || c == '\'') {
                append(&word, &length, &capacity, (char)c);
            } else if (c == '-' && length && letter((unsigned char)word[length - 1])) {
                pending_dash = 1;
            } else {
                finish(table, word, &length);
            }
        }
    }
    finish(table, word, &length);
    free(word);
    if (close(fd) < 0) fail(path);
}

static int has_suffix(const char *name, const char *suffix)
{
    size_t n = strlen(name), s = strlen(suffix);
    return n >= s && strcmp(name + n - s, suffix) == 0;
}

static void scan(Table *table, const char *path, const char *suffix)
{
    DIR *directory = opendir(path);
    struct dirent *entry;
    if (!directory) fail(path);
    for (;;) {
        char *child;
        size_t p, n;
        struct stat data;
        errno = 0;
        entry = readdir(directory);
        if (!entry) {
            if (errno) fail(path);
            break;
        }
        if (entry->d_name[0] == '.') continue;
        p = strlen(path);
        n = strlen(entry->d_name);
        if (p > SIZE_MAX - n - 2) { errno = EOVERFLOW; fail("path length"); }
        child = allocate(p + n + 2);
        strcpy(child, path);
        child[p] = '/';
        strcpy(child + p + 1, entry->d_name);
        if (stat(child, &data) < 0) fail(child);
        if (S_ISDIR(data.st_mode)) scan(table, child, suffix);
        else if (S_ISREG(data.st_mode) && has_suffix(entry->d_name, suffix))
            read_file(table, child);
        free(child);
    }
    if (closedir(directory) < 0) fail(path);
}

static int compare(const void *left, const void *right)
{
    const Entry *a = left, *b = right;
    if (a->count != b->count) return a->count > b->count ? -1 : 1;
    return strcmp(a->word, b->word);
}

static void write_all(const char *text, size_t length)
{
    while (length) {
        ssize_t sent = write(STDOUT_FILENO, text, length);
        if (sent < 0) {
            if (errno == EINTR) continue;
            fail("stdout");
        }
        if (sent == 0) { errno = EIO; fail("stdout"); }
        text += sent;
        length -= (size_t)sent;
    }
}

int main(int argc, char **argv)
{
    Table table = {NULL, 0, 0};
    const char *suffix = ".txt";
    int option_index = -1, suffix_index = -1, paths = 0;
    int i;
    size_t j;
    /* Parse the option first so it applies to every directory argument. */
    for (i = 1; i < argc; ++i) {
        if (strncmp(argv[i], "-s", 2) == 0) {
            if (option_index != -1) {
                errno = EINVAL; fail("duplicate -s");
            }
            option_index = i;
            if (argv[i][2]) suffix = argv[i] + 2;
            else {
                if (i + 1 == argc) { errno = EINVAL; fail("missing suffix"); }
                suffix_index = ++i;
                suffix = argv[i];
            }
        } else ++paths;
    }
    if (!paths) {
        errno = EINVAL;
        fail("usage: words [-s suffix] file-or-directory ...");
    }
    for (i = 1; i < argc; ++i) {
        struct stat data;
        if (i == option_index || i == suffix_index) continue;
        if (stat(argv[i], &data) < 0) fail(argv[i]);
        if (S_ISDIR(data.st_mode)) scan(&table, argv[i], suffix);
        else if (S_ISREG(data.st_mode)) read_file(&table, argv[i]);
        else { errno = EINVAL; fail(argv[i]); }
    }
    if (table.length > 1)
        qsort(table.entries, table.length, sizeof(Entry), compare);
    for (j = 0; j < table.length; ++j) {
        char number[3 * sizeof(size_t) + 3];
        int length = sprintf(number, " %zu\n", table.entries[j].count);
        write_all(table.entries[j].word, strlen(table.entries[j].word));
        write_all(number, (size_t)length);
        free(table.entries[j].word);
    }
    free(table.entries);
    return EXIT_SUCCESS;
}
