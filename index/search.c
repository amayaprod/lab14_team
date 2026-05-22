#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#include "search.h"
#include "../posting.h"

// Перевод строки в нижний регистр
static void strToLower(char* str) {
    for ( int i = 0; str[i]; ++i ) {
        str[i] = tolower(str[i]);
    }
}

// Метод двух указателей
static Vector* intersectTwoPostings(const Vector* list1, const Vector* list2) {
    Vector* result = createPostingList();
    if ( !list1 ||
         !list2 ||
         list1 -> size == 0 ||
         list2 -> size == 0
         ) {
        return result;
    }

    size_t i = 0;
    size_t j = 0;

    while ( i < list1 -> size && j < list2 -> size ) {
        PostingEntry* p1 = (PostingEntry*)getVectorItem((Vector*)list1, i);
        PostingEntry* p2 = (PostingEntry*)getVectorItem((Vector*)list2, j);

        if ( p1 -> doc_id == p2 -> doc_id ) {
            appendPosting(result, p1 -> doc_id, p1 -> title);
            ++i, ++j;
        } else if ( p1 -> doc_id < p2 -> doc_id ) {
            ++i;
        } else {
            ++j;
        }
    }
    return result;
}

Vector* intersectPostings(Vector** lists, int n) {
    if ( n <= 0 ) {
        return createPostingList();
    }
    if ( n == 1 ) {
        return clonePostingList(lists[0]);
    }

    Vector* current = clonePostingList(lists[0]);

    for ( int i = 1; i < n; ++i ) {
        Vector* next_list = lists[i];
        Vector* new_current = intersectTwoPostings(current, next_list);

        vectorFree(current);
        current = new_current;
        if ( current -> size == 0) {
            break;
        }
    }
    return current;
}

SearchResults* search(Index* idx, const char* query) {
    clock_t start_time = clock();

    SearchResults* res = (SearchResults*)malloc(sizeof(SearchResults));
    res -> results = createVector(sizeof(SearchResult));
    res -> total = 0;
    res -> time_ms = 0.0;

    if ( !query || strlen(query) == 0 ) {
        return res;
    }

    char* query_copy = strdup(query);
    strToLower(query_copy);
    Vector* lists = createVector(sizeof(Vector*));

    char* token = strtok(query_copy, " \t\n\r");
    while ( token != NULL ) {
        // Запрос к дереву: lookupTerm должен быть реализован в index.c
        Vector* term_postings = lookupTerm(idx, token);

        if ( term_postings == NULL || term_postings -> size == 0 ) {
            vectorFree(lists);
            free(query_copy);
            res -> time_ms = ((double)(clock() - start_time) / CLOCKS_PER_SEC) * 1000.0;
            return res;
        }

        appendVectorItem(lists, &term_postings);
        token = strtok(NULL, " \t\n\r");
    }

    int n_terms = lists -> size;
    if ( n_terms > 0 ) {
        Vector** lists_array = (Vector**)lists -> data;
        Vector* final_intersection = intersectPostings(lists_array, n_terms);

        res -> total = final_intersection -> size;

        int limit = (final_intersection -> size < 10) ? final_intersection -> size : 10;
        for ( int i = 0; i < limit; ++i ) {
            PostingEntry* pe = (PostingEntry*)getVectorItem(final_intersection, i);
            SearchResult sr;

            sr.doc_id = pe -> doc_id;
            strncpy(sr.title, pe -> title, MAX_TITLE_LEN - 1);
            sr.title[MAX_TITLE_LEN - 1] = '\0';
            sr.score = n_terms;

            appendVectorItem(res->results, &sr);
        }
        vectorFree(final_intersection);
    }

    vectorFree(lists);
    free(query_copy);
    res -> time_ms = ((double)(clock() - start_time) / CLOCKS_PER_SEC) * 1000.0;

    return res;
}

void printResultsText(const SearchResults* sr) {
    if ( !sr ) {
        return;
    }

    printf("\nВремя: %.2f мс | Найдено: %d документов\n", sr -> time_ms, sr -> total);
    if ( sr -> results -> size == 0 ) {
        printf("Совпадений не найдено.\n\n");
        return;
    }
    printf("\n");
    for ( size_t i = 0; i < sr -> results -> size; ++i ) {
        SearchResult* r = (SearchResult*)getVectorItem(sr -> results, i);
        printf(" %2zu. [id=%d] %s\n", i + 1, r -> doc_id, r -> title);
    }
    printf("\n");
}

void printResultsJSON(const SearchResults* sr) {
    if ( !sr ) {
        return;
    }

    printf("{\n  \"time_ms\": %.2f,\n  \"total\": %d,\n  \"results\": [\n", sr -> time_ms, sr -> total);
    for (size_t i = 0; i < sr -> results -> size; ++i ) {
        SearchResult* r = (SearchResult*)getVectorItem(sr -> results, i);
        printf("    {\"doc_id\": %d, \"score\": %d, \"title\": \"%s\"}%s\n",
               r -> doc_id, r -> score, r -> title, (i < sr -> results -> size - 1) ? "," : "");
    }
    printf("  ]\n}\n");
}

void freeSearchResults(SearchResults* sr) {
    if ( sr ) {
        if ( sr -> results) {
            vectorFree(sr -> results);
        }
        free(sr);
    }
}