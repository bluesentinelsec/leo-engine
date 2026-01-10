#ifndef GETOPT_H
#define GETOPT_H

#ifdef __cplusplus
extern "C" {
#endif

extern int   opterr;   // if error message should be printed
extern int   optind;   // index into parent argv vector
extern int   optopt;   // character checked for validity
extern char *optarg;   // argument associated with option

struct option {
    const char *name;
    int         has_arg;
    int        *flag;
    int         val;
};

enum { no_argument = 0, required_argument = 1, optional_argument = 2 };

int getopt(int argc, char * const argv[], const char *optstring);

int getopt_long(int argc, char * const argv[],
    const char *optstring, const struct option *longopts, int *longindex);

#ifdef __cplusplus
}
#endif

#endif // GETOPT_H

