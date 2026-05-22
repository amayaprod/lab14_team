#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "index/index.h"
#include "index/search.h"


static int parse_json_line(const char* line, int* doc_id, char* title, size_t max_title, char*** tokens, int* n_tokens) {
    const char* p = strstr(line, "\"doc_id\":");
    if (!p) return 0;
    p += 9;
    while (*p == ' ' || *p == ':' || *p == '"') p++;
    *doc_id = atoi(p);
    p = strstr(line, "\"title\":");
    if (!p) return 0;
    p += 8;
    while (*p == ' ' || *p == ':' || *p == '"') p++;
    
    size_t t_idx = 0;
    while (*p && t_idx < max_title - 1) {
        if (*p == '"' && *(p - 1) != '\\') {
            break;
        }
        title[t_idx++] = *p;
        p++;
    }
    title[t_idx] = '\0';
    p = strstr(line, "\"tokens\":");
    if (!p) return 0;
    p += 9;
    while (*p && *p != '[') p++;
    if (*p == '[') p++;
    int capacity = 32;
    int count = 0;
    char** list = (char**)malloc(capacity * sizeof(char*));
    if (!list) return 0;
    while (*p && *p != ']') {
        if (*p == '"') {
            p++;
            const char* start = p;
            while (*p && !(*p == '"' && *(p - 1) != '\\')) {
                p++;
            }
            size_t len = p - start;
            char* token = (char*)malloc(len + 1);
            if (token) {
                memcpy(token, start, len);
                token[len] = '\0';
                
                if (count >= capacity) {
                    capacity *= 2;
                    char** new_list = (char**)realloc(list, capacity * sizeof(char*));
                    if (new_list) {
                        list = new_list;
                    }
                }
                list[count++] = token;
            }
            if (*p == '"') p++;
        } else {
            p++;
        }
    }
    *tokens = list;
    *n_tokens = count;
    return 1;
}

static void runIndex(TreeType type, const char* data_path, const char* idx_path) {
    FILE* f = fopen(data_path, "r");
    if (!f) {
        fprintf(stderr, "Не удалось открыть входной файл данных: %s\n", data_path);
        exit(1);
    }
    printf("Создание поискового индекса (%s) \n", typeName(type));
    Index* idx = createIndex(type);
    if (!idx) {
        fprintf(stderr, "Не удалось инициализировать индекс\n");
        fclose(f);
        exit(1);
    }
    char line_buffer[65536];
    int indexed_count = 0;
    clock_t start_time = clock();
    while (fgets(line_buffer, sizeof(line_buffer), f)) {
        int doc_id = 0;
        char title[512] = {0};
        char** tokens = NULL;
        int n_tokens = 0;
        if (parse_json_line(line_buffer, &doc_id, title, sizeof(title), &tokens, &n_tokens)) {
            indexDocument(idx, doc_id, title, (const char**)tokens, n_tokens);
            indexed_count++;
            for (int i = 0; i < n_tokens; i++) {
                free(tokens[i]);
            }
            free(tokens);
        }
        if (indexed_count % 25000 == 0 && indexed_count > 0) {
            printf("Индексировано документов: %d\n", indexed_count);
        }
    }
    clock_t end_time = clock();
    double elapsed = (double)(end_time - start_time) / CLOCKS_PER_SEC;
    printf("Индексация завершена Всего: %d док. Время: %.2f сек.\n", indexed_count, elapsed);
    printf("Сохранение индекса в: %s\n", idx_path);
    saveIndex(idx, idx_path);
    freeIndex(idx);
    fclose(f);
}

static void runSearch(TreeType type, const char* idx_path,
                      const char* query, int json_out) {
    Index* idx = loadIndex(idx_path, type);
    if (!idx) {
        fprintf(stderr, "Ошибка: не удалось загрузить индекс: %s\n", idx_path);
        exit(1);
    }
    SearchResults* sr = search(idx, query);
    if (!sr) {
        fprintf(stderr, "Ошибка при выполнении поиска\n");
        freeIndex(idx);
        exit(1);
    }
    if (json_out) {
        printResultsJSON(sr);
    } else {
        printResultsText(sr);
    }
    freeSearchResults(sr);
    freeIndex(idx);
}

static void usage(const char* prog) {
    fprintf(stderr,
        "Usage:\n"
        "  %s index  --type=<avl|rb|btree> [--data=PATH] [--index=PATH]\n"
        "  %s search --type=<avl|rb|btree> [--index=PATH] [--json] \"query\"\n",
        prog, prog);
}

int main(int argc, char* argv[]) {
    if (argc < 3) { usage(argv[0]); return 1; }

    const char* mode = argv[1];
    TreeType    type = TREE_AVL;
    const char* data_path = "data/processed/docs.jsonl";
    char        idx_path[512] = {0};
    int         json_out = 0;
    const char* query    = NULL;

    for (int i = 2; i < argc; i++) {
        if      (strncmp(argv[i], "--type=",  7) == 0) type = parseType(argv[i] + 7);
        else if (strncmp(argv[i], "--data=",  7) == 0) data_path = argv[i] + 7;
        else if (strncmp(argv[i], "--index=", 8) == 0)
            strncpy(idx_path, argv[i] + 8, sizeof(idx_path) - 1);
        else if (strcmp(argv[i], "--json")    == 0)    json_out = 1;
        else if (argv[i][0] != '-')                    query = argv[i];
    }

    if (idx_path[0] == '\0')
        snprintf(idx_path, sizeof(idx_path), "data/index_%s.txt", typeName(type));

    if (strcmp(mode, "index") == 0) {
        runIndex(type, data_path, idx_path);
    } else if (strcmp(mode, "search") == 0) {
        if (!query) { fprintf(stderr, "No query provided(Не задана строка поискового запроса)\n"); return 1; }
        runSearch(type, idx_path, query, json_out);
    } else {
        fprintf(stderr, "Unknown mode: %s\n", mode);
        usage(argv[0]);
        return 1;
    }
    return 0;
}
