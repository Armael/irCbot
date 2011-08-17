#include <libircclient/libircclient.h>

#ifndef H_BOT
#define H_BOT

#define MAX_ANS 20

typedef struct Answer {
    /* La regex représentant la condition de réponse */
    char* regex;
    /* Un tableau contenant les réponses possibles dans
    le cas où la chaine en entrée convient.
    Si answers_nb > 1, la réponse est tirée au hasard
    parmi celles disponibles */
    int answers_nb;
    char* answers[MAX_ANS];
} Answer;

/* Structure définissant une ligne dans l'historique */

typedef struct Line {
    char* author;
    char* content;
} Line;

/* Contexte */
typedef struct {
    char* channel;
} irc_ctx_t;

/* Prototypes */
int mod(int a, int b);
char* tolower_str(char* str);
char* skip_blanks(const char* str);
char* pick_string(char** strings, int strings_nb);
void addlog (const char * fmt, ...);
void dump_event(irc_session_t* session,
                const char* event,
                const char* origin,
                const char **params,
                unsigned int count);
void log_and_print(const char* fmt, ...);
void log_and_print_said(const char* name, const char* fmt, ...);
void debug_print_log();
void die(int sig);
void parse_answers();
int run_cmd(char* raw_line, irc_session_t* session,
                            const char* origin);
void event_join(irc_session_t* session,
                const char* event,
                const char* origin,
                const char **params,
                unsigned int count);
void event_part(irc_session_t* session,
                const char* event,
                const char* origin,
                const char **params,
                unsigned int count);
void event_connect(irc_session_t* session,
                   const char* event,
                   const char* origin,
                   const char **params,
                   unsigned int count);
void event_privmsg(irc_session_t* session,
                   const char* event,
                   const char* origin,
                   const char **params,
                   unsigned int count);
void event_channel(irc_session_t* session,
                   const char* event,
                   const char* origin,
                   const char **params,
                   unsigned int count);


#define LOG_MAX_LINES 10
extern Line log_array[];
extern int log_pos; /* Indicateur de la prochaine
ligne à remplir du log, indice pour log[] */
extern int log_lines_nb; /* Nombre de lignes
qui sont dans le log */

#endif
