#include "getopt.h"
#include <stdio.h>
#include <string.h>

int   opterr  = 1; // error message printing
int   optind  = 1; // index into argv
int   optopt  = '?';
char *optarg  = NULL;

static char *next = NULL;

int getopt(int argc, char * const argv[], const char *optstring)
{
    char c;
    char *cp;

    if (next == NULL || *next == '\0') {
        if (optind >= argc || argv[optind][0] != '-' || argv[optind][1] == '\0')
            return -1;
        if (strcmp(argv[optind], "--") == 0) {
            optind++;
            return -1;
        }
        next = argv[optind] + 1;
        optind++;
    }
    c = *next++;
    optopt = c;
    if ((cp = strchr(optstring, c)) == NULL) {
        if (opterr)
            fprintf(stderr, "illegal option -- %c\n", c);
        return '?';
    }
    if (cp[1] == ':') {
        if (*next != '\0') {
            optarg = next;
            next = NULL;
        } else if (optind < argc) {
            optarg = argv[optind];
            optind++;
        } else {
            if (opterr)
                fprintf(stderr, "option requires an argument -- %c\n", c);
            return '?';
        }
    } else {
        optarg = NULL;
    }
    return c;
}

// Minimal getopt_long implementation for --long options
int getopt_long(int argc, char * const argv[],
    const char *optstring, const struct option *longopts, int *longindex)
{
    if (optind >= argc)
        return -1;
    if (strncmp(argv[optind], "--", 2) == 0) {
        // long option
        const char *arg = argv[optind] + 2;
        for (int i = 0; longopts[i].name; ++i) {
            if (strcmp(arg, longopts[i].name) == 0) {
                if (longindex) *longindex = i;
                if (longopts[i].has_arg == required_argument) {
                    if (optind + 1 < argc) {
                        optarg = argv[++optind];
                    } else {
                        if (opterr)
                            fprintf(stderr, "option '--%s' requires an argument\n", longopts[i].name);
                        optind++;
                        return '?';
                    }
                } else {
                    optarg = NULL;
                }
                optind++;
                if (longopts[i].flag) {
                    *(longopts[i].flag) = longopts[i].val;
                    return 0;
                } else {
                    return longopts[i].val;
                }
            }
        }
        if (opterr)
            fprintf(stderr, "unrecognized option '--%s'\n", arg);
        optind++;
        return '?';
    }
    // fall back to getopt for short options
    return getopt(argc, argv, optstring);
}

