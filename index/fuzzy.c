#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#include "fuzzy.h"
#include "../posting.h"

static int levenshtein(const char* s1, const char* s2) {
    int len1 = strlen(s1);
    int len2 = strlen(s2);
    int* dp = (int*)malloc((len2 + 1) * sizeof(int));
    for (int j = 0; j <= len2; j++) {
        dp[j] = j;
    }
    for (int i = 1; i <= len1; i++) {
        int prev_diagonal = dp[0];
        dp[0] = i;
        for (int j = 1; j <= len2; j++) {
            int temp = dp[j];
            int cost = (tolower((unsigned char)s1[i - 1]) == tolower((unsigned char)s2[j - 1])) ? 0 : 1;
            int min = dp[j] + 1;
            if (dp[j - 1] + 1 < min) {
                min = dp[j - 1] + 1;
            }
            if (prev_diagonal + cost < min) {
                min = prev_diagonal + cost;
            }
            prev_diagonal = temp;
            dp[j] = min;
        }
    }
    int result = dp[len2];
    free(dp);
    return result;
}

typedef struct {
    const char* target_term;
    int max_dist;
    Vector* candidates;
} FuzzyContext;

static void fuzzy_visit(const char* key, Vector* postings, void* ctx) {
    FuzzyContext* f_ctx = (FuzzyContext*)ctx;
    int dist = levenshtein(f_ctx->target_term, key);
    if (dist <= f_ctx->max_dist) {
        FuzzyCandidate cand;
        strncpy(cand.term, key, sizeof(cand.term) - 1);
        cand.term[sizeof(cand.term) - 1] = '\0';
        cand.distance = dist;
        cand.postings = postings;
        appendVectorItem(f_ctx->candidates, &cand);
    }
}

Vector* fuzzyFindCandidates(Index* idx, const char* term, int max_distance) {
    Vector* candidates = createVector(sizeof(FuzzyCandidate));
    if (!idx || !term) {
        return candidates;
    }
    FuzzyContext ctx = { term, max_distance, candidates };
    traverseIndex(idx, fuzzy_visit, &ctx);
    return candidates;
}

typedef struct {
    int doc_id;
    char title[MAX_TITLE_LEN];
    int matched_terms;
    int sum_distance;
    int last_matched_token_idx;
    int best_distance_for_token;
} DocAccumulator;

static int compare_fuzzy_results(const void* a, const void* b) {
    SearchResult* r1 = (SearchResult*)a;
    SearchResult* r2 = (SearchResult*)b;
    if (r2->score != r1->score) {
        return r2->score - r1->score;
    }
    return r1->doc_id - r2->doc_id;
}

static void make_lowercase(char* str) {
    for (int i = 0; str[i]; i++) {
        str[i] = tolower((unsigned char)str[i]);
    }
}

SearchResults* fuzzySearch(Index* idx, const char* query, int max_distance) {
    clock_t start_time = clock();

    SearchResults* res = (SearchResults*)malloc(sizeof(SearchResults));
    res->results = createVector(sizeof(SearchResult));
    res->total = 0;
    res->time_ms = 0.0;
    if (!idx || !query || strlen(query) == 0) {
        return res;
    }
    char* query_copy = strdup(query);
    make_lowercase(query_copy);
    int capacity = 8;
    int n_tokens = 0;
    char** tokens = malloc(capacity * sizeof(char*));
    char* token = strtok(query_copy, " \t\n\r");
    while (token != NULL) {
        if (n_tokens >= capacity) {
            capacity *= 2;
            tokens = realloc(tokens, capacity * sizeof(char*));
        }
        tokens[n_tokens++] = strdup(token);
        token = strtok(NULL, " \t\n\r");
    }
    free(query_copy);
    if (n_tokens == 0) {
        free(tokens);
        return res;
    }
    Vector* accumulators = createVector(sizeof(DocAccumulator));
    for (int t = 0; t < n_tokens; t++) {
        Vector* candidates = fuzzyFindCandidates(idx, tokens[t], max_distance);
        for (size_t c = 0; c < candidates->size; c++) {
            FuzzyCandidate* cand = (FuzzyCandidate*)getVectorItem(candidates, c);
            for (size_t p = 0; p < cand->postings->size; p++) {
                PostingEntry* pe = (PostingEntry*)getVectorItem(cand->postings, p);
                int found = 0;
                for (size_t a = 0; a < accumulators->size; a++) {
                    DocAccumulator* acc = (DocAccumulator*)getVectorItem(accumulators, a);
                    if (acc->doc_id == pe->doc_id) {
                        found = 1;
                        if (acc->last_matched_token_idx == t) {
                            if (cand->distance < acc->best_distance_for_token) {
                                acc->sum_distance = acc->sum_distance - acc->best_distance_for_token + cand->distance;
                                acc->best_distance_for_token = cand->distance;
                            }
                        } else {
                            acc->matched_terms++;
                            acc->sum_distance += cand->distance;
                            acc->last_matched_token_idx = t;
                            acc->best_distance_for_token = cand->distance;
                        }
                        break;
                    }
                }
                if (!found) {
                    DocAccumulator acc;
                    acc.doc_id = pe->doc_id;
                    strncpy(acc.title, pe->title, MAX_TITLE_LEN - 1);
                    acc.title[MAX_TITLE_LEN - 1] = '\0';
                    acc.matched_terms = 1;
                    acc.sum_distance = cand->distance;
                    acc.last_matched_token_idx = t;
                    acc.best_distance_for_token = cand->distance;
                    appendVectorItem(accumulators, &acc);
                }
            }
        }
        vectorFree(candidates);
    }
    for (size_t i = 0; i < accumulators->size; i++) {
        DocAccumulator* acc = (DocAccumulator*)getVectorItem(accumulators, i);
        int avg_distance = acc->sum_distance / acc->matched_terms;
        int score = acc->matched_terms * 10 - avg_distance;
        SearchResult sr;
        sr.doc_id = acc->doc_id;
        strncpy(sr.title, acc->title, MAX_TITLE_LEN - 1);
        sr.title[MAX_TITLE_LEN - 1] = '\0';
        sr.score = score;
        appendVectorItem(res->results, &sr);
    }
    res->total = res->results->size;
    qsort(res->results->data, res->results->size, res->results->elem_size, compare_fuzzy_results);
    if (res->results->size > 10) {
        res->results->size = 10;
    }
    vectorFree(accumulators);
    for (int i = 0; i < n_tokens; i++) {
        free(tokens[i]);
    }
    free(tokens);
    res->time_ms = ((double)(clock() - start_time) / CLOCKS_PER_SEC) * 1000.0;
    return res;
}