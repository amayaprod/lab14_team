#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "benchmark.h"
#include "index.h"
#include "search.h"

// Замер памяти
#ifdef _WIN32
#include <windows.h>
#include <psapi.h>
static double get_current_ram_mb() {
    PROCESS_MEMORY_COUNTERS pmc;
    if ( GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc)) ) {
        return (double)pmc.WorkingSetSize / (1024.0 * 1024.0);
    }
    return 0.0;
}
#else
#include <sys/resource.h>
    static double get_current_ram_mb() {
        FILE* f = fopen("/proc/self/statm", "r");
        if ( !f ) {
            return 0.0;
        }
        size_t size, resident;
        if ( fscanf(f, "%zu %zu", &size, &resident) != 2 ) {
            resident = 0;
        }
        fclose(f);
        return (double)(resident * 4) / 1024.0;
    }
#endif

// Быстрый динамический парсер строки
static int parse_json_line_bench(const char* line, int* doc_id, char* title, size_t max_title, char*** tokens, int* n_tokens) {
    const char* p = strstr(line, "\"doc_id\":");
    if ( !p ) {
        return 0;
    }
    p += 9;
    while ( *p == ' ' || *p == ':' || *p == '"' ) {
        p++;
    }
    *doc_id = atoi(p);

    p = strstr(line, "\"title\":");
    if ( !p ) {
        return 0;
    }
    p += 8;
    while ( *p == ' ' || *p == ':' || *p == '"' ) {
        p++;
    }

    size_t t_idx = 0;
    while ( *p && t_idx < max_title - 1 ) {
        if ( *p == '"' && *(p - 1) != '\\' ) {
            break;
        }
        title[t_idx++] = *p;
        p++;
    }
    title[t_idx] = '\0';

    p = strstr(line, "\"tokens\":");
    if ( !p ) {
        return 0;
    }
    p += 9;
    while ( *p && *p != '[' ) {
        p++;
    }
    if ( *p == '[' ) {
        p++;
    }

    int capacity = 8;
    int count = 0;
    char** list = (char**)malloc(capacity * sizeof(char*));
    if ( !list ) {
        return 0;
    }

    while ( *p && *p != ']' ) {
        if ( *p == '"' ) {
            p++;
            const char* start = p;
            while ( *p && !(*p == '"' && *(p - 1) != '\\') ) {
                p++;
            }
            size_t len = p - start;
            char* token = (char*)malloc(len + 1);
            if ( token ) {
                memcpy(token, start, len);
                token[len] = '\0';

                if ( count >= capacity ) {
                    capacity *= 2;
                    char** new_list = (char**)realloc(list, capacity * sizeof(char*));
                    if ( new_list ) {
                        list = new_list;
                    }
                }
                list[count++] = token;
            }
            if ( *p == '"' ) {
                p++;
            }
        } else {
            p++;
        }
    }
    *tokens = list;
    *n_tokens = count;
    return 1;
}

// Загружает словарь из файла для генерации случайных поисковых запросов
static int load_vocab(const char* data_path, char*** vocab_out) {
    FILE* f = fopen(data_path, "r");
    if ( !f ) {
        return 0;
    }

    int capacity = 2000;
    char** vocab = malloc(capacity * sizeof(char*));
    int size = 0;
    char line[4096];

    while ( fgets(line, sizeof(line), f) && size < 1500 ) {
        int doc_id;
        char title[256];
        char** tokens;
        int n_tokens;
        if ( parse_json_line_bench(line, &doc_id, title, sizeof(title), &tokens, &n_tokens) ) {
            for ( int i = 0; i < n_tokens && size < 1500; i++ ) {
                // Добавляем только уникальные слова длиной > 3 символов
                int exists = 0;
                for ( int j = 0; j < size; j++ ) {
                    if ( strcmp(vocab[j], tokens[i]) == 0 ) {
                        exists = 1;
                        break;
                    }
                }
                if ( !exists && strlen(tokens[i]) > 3 ) {
                    vocab[size++] = strdup(tokens[i]);
                }
            }
            for ( int i = 0; i < n_tokens; i++ ) {
                free(tokens[i]);
            }
            free(tokens);
        }
    }
    fclose(f);
    *vocab_out = vocab;
    return size;
}

void run_all_experiments(const char* data_path) {
    printf("Загрузка словаря для тестирования поиска\n");
    char** vocab = NULL;
    int vocab_size = load_vocab(data_path, &vocab);
    if (vocab_size == 0) {
        printf("Ошибка загрузки словаря\n");
        return;
    }
    printf("Словарь загружен, уникальных слов: %d\n\n", vocab_size);

    int sizes[] = {50000, 200000, 500000};
    TreeType types[] = {TREE_AVL, TREE_RB, TREE_BTREE};

    double time_results[3][3] = {0};
    double mem_results[2][3] = {0};
    double search_results[3][3] = {0};

    srand(42);

    printf("Запуск экспериментов (Индексация и Память)\n");
    for (int s_idx = 0; s_idx < 3; s_idx++) {
        int n = sizes[s_idx];

        for (int t_idx = 0; t_idx < 3; t_idx++) {
            printf("Индексация %dk документов в %s... ", n / 1000, typeName(types[t_idx]));
            fflush(stdout);

            double mem_before = get_current_ram_mb();
            clock_t start = clock();

            Index* idx = createIndex(types[t_idx]);

            // Читаем файл прямо в процессе индексации (0 МБ накладных расходов!)
            FILE* f = fopen(data_path, "r");
            if (!f) {
                printf("Ошибка открытия файла\n");
                return;
            }

            char line[65536];
            int count = 0;
            while (fgets(line, sizeof(line), f) && count < n) {
                int doc_id;
                char title[256];
                char** tokens;
                int n_tokens;
                if (parse_json_line_bench(line, &doc_id, title, sizeof(title), &tokens, &n_tokens)) {
                    int limit_tokens = n_tokens > 8 ? 8 : n_tokens;
                    indexDocument(idx, doc_id, title, (const char**)tokens, limit_tokens);
                    // Сразу очищаем память одного документа!
                    for (int i = 0; i < n_tokens; i++) free(tokens[i]);
                    free(tokens);
                    count++;
                }
            }
            fclose(f);

            clock_t end = clock();
            double mem_after = get_current_ram_mb();

            time_results[s_idx][t_idx] = (double)(end - start) / CLOCKS_PER_SEC;

            double mem_used = mem_after - mem_before;
            if (mem_used < 0) mem_used = 0;

            if (s_idx == 0) mem_results[0][t_idx] = mem_used;
            if (s_idx == 2) mem_results[1][t_idx] = mem_used;

            printf("Время: %.2f сек | Память: %.2f МБ\n", time_results[s_idx][t_idx], mem_used);

            // Если построили 500k, запускаем тесты ПОИСКА
            if ( s_idx == 2 ) {
                printf("  Тестирование поиска для %s\n", typeName(types[t_idx]));
                for ( int words = 1; words <= 3; words++ ) {
                    int queries_count = 1000;
                    clock_t s_start = clock();

                    for ( int q = 0; q < queries_count; q++ ) {
                        char query[256] = "";
                        for ( int w = 0; w < words; w++ ) {
                            int r_word = rand() % vocab_size;
                            if ( w > 0 ) {
                                strcat(query, " ");
                            }
                            strcat(query, vocab[r_word]);
                        }

                        SearchResults* sr = search(idx, query);
                        freeSearchResults(sr);
                    }

                    clock_t s_end = clock();
                    search_results[words-1][t_idx] = ((double)(s_end - s_start) / CLOCKS_PER_SEC) * 1000.0 / queries_count;
                }
            }

            freeIndex(idx);
        }
        printf("\n");
    }

    printf("\nРЕЗУЛЬТАТЫ ЭКСПЕРИМЕНТОВ\n\n");

    printf("### 3.1 Время индексации\n\n");
    printf("| Документов | AVL (сек) | Red-Black (сек) | B-tree (сек) |\n");
    printf("|---|---|---|---|\n");
    for ( int s_idx = 0; s_idx < 3; s_idx++ ) {
        printf("| %dk | %.3f | %.3f | %.3f |\n", sizes[s_idx]/1000,
               time_results[s_idx][0], time_results[s_idx][1], time_results[s_idx][2]);
    }

    printf("\n### 3.2 Время поиска (в индексе на 500k)\n\n");
    printf("| Слов в запросе | AVL (мс/запрос) | Red-Black (мс/запрос) | B-tree (мс/запрос) |\n");
    printf("|---|---|---|---|\n");
    for ( int w = 0; w < 3; w++ ) {
        printf("| %d | %.4f | %.4f | %.4f |\n", w + 1,
               search_results[w][0], search_results[w][1], search_results[w][2]);
    }

    printf("\n### 3.3 Использование памяти\n\n");
    printf("| Документов | AVL (МБ) | Red-Black (МБ) | B-tree (МБ) |\n");
    printf("|---|---|---|---|\n");
    printf("| 50k | %.2f | %.2f | %.2f |\n", mem_results[0][0], mem_results[0][1], mem_results[0][2]);
    printf("| 500k | %.2f | %.2f | %.2f |\n", mem_results[1][0], mem_results[1][1], mem_results[1][2]);
    printf("\n==============================================\n");

    // Очистка словаря
    for ( int i = 0; i < vocab_size; i++ ) {
        free(vocab[i]);
    }
    free(vocab);
}