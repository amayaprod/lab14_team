#pragma once

#include "index.h"
#include "search.h"

typedef struct {
    char    term[256];
    int     distance;
    Vector* postings;
} FuzzyCandidate;

Vector* fuzzyFindCandidates(Index* idx, const char* term, int max_distance);

SearchResults* fuzzySearch(Index* idx, const char* query, int max_distance);