#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#include <libircclient/libircclient.h>

/* Structures décrivant les réponses prédéfinies */
typedef enum Where {
    STARTS_WITH,
    DOESNT_START_WITH,
    CONTAINS,
    DOESNT_CONTAIN,
    ENDS_WITH,
    DOESNT_END_WITH
} Where;

/* Une condition de filtrage sur une chaine de
caractères. La casse de str ainsi que de la chaine
qui sera donnée en entrée n'importent pas */
typedef struct Match_cond {
    Where pos;
    char* str;
} Match_cond;

typedef struct Answer {
    /* L'ensemble de conditions représenté par
    une condition Match_cond que doit satisfaire la chaine
    en entrée. Si cond_nb == 0, la chaine en entrée
    convient systématiquement */
    Match_cond* conditions;
    int cond_nb;
    /* Un tableau contenant les réponses possibles dans
    le cas où la chaine en entrée convient.
    Si answers_nb > 1, la réponse est tirée au hasard
    parmi celles disponibles */
    char** answers_str;
    int answers_nb;
} Answer;

/* Structures décrivant les commandes prédéfinies */

typedef struct Cmd {
    char* cmd_str;
    void (*f)(irc_session_t* session,
              const char* cmd,
              const char* origin,
              char* args);
} Cmd;

/* Structure définissant une ligne dans l'historique */

typedef struct Line {
    char* author;
    char* content;
} Line;

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

#include "config.h"
#include "commands.h"

static int answers_nb = 0;
static Answer* answers;

#define LOG_MAX_LINES 10
static Line log[LOG_MAX_LINES];
static int log_pos = 0; /* Indicateur de la prochaine
ligne à remplir du log, indice pour log[] */
static int log_lines_nb = 0; /* Nombre de lignes
qui sont dans le log */

/* Modulo envoyant les négatifs en positif */
int mod(int a, int b)
{
    return (a<0) ? mod(a+10, b) : a%b;
}

/* Fonctions de manipulation de chaines */

char* tolower_str(char* str)
{
    int i;
    for(i=0; str[i] != '\0'; str[i] = tolower(str[i]), i++);
    return str;
}

char* skip_blanks(const char* str)
{
    int i;
    for(i = strlen(str)-1; i >= 0 && (str[i] == ' ' || str[i] == '\n') ; i--);

    int j;
    for(j=0; (str[j] == ' ' || str[j] == '\n') && str[j] != '\0'; j++);

    char* out_str = calloc((i-j+2), sizeof(char));
    strncpy(out_str, &str[j], i-j+1);
    return out_str;
}

char* pick_string(char** strings, int strings_nb)
{
    srand(time(NULL));
    return strings[(rand() % strings_nb)];
}

/* Fonctions de log/affichage */


void addlog (const char * fmt, ...)
{
    FILE * fp;
    char buf[1024];
    va_list va_alist;

    va_start (va_alist, fmt);
    vsnprintf(buf, sizeof(buf), fmt, va_alist);
    va_end(va_alist);

    if((fp = fopen(log_file, "ab")) != 0) {
        fprintf(fp, "%s\n", buf);
        fclose(fp);
    }
}

void dump_event(irc_session_t* session,
                const char* event,
                const char* origin,
                const char **params,
                unsigned int count)
{
    char buf[512];
    int i;

    buf[0] = '\0';
    for(i=0; i<count; i++) {
        if(i) {
            strcat(buf, "|");
        }

        strcat(buf, params[i]);
    }

    addlog("Évenement \"%s\", origine: \"%s\", paramètres: %d [%s]", event, origin ? origin : "NULL", i, buf);
}

void log_and_print(const char* fmt, ...)
{
    va_list va_alist;
    char buf[1024];

    va_start(va_alist, fmt);
    vsnprintf(buf, sizeof(buf), fmt, va_alist);
    va_end(va_alist);

    printf("%s\n", buf);
    addlog("%s\n", buf);
}

void log_and_print_said(const char* name, const char* fmt, ...)
{
    va_list va_alist;
    char buf[1024];

    va_start(va_alist, fmt);
    vsnprintf(buf, sizeof(buf), fmt, va_alist);
    va_end(va_alist);

    printf("<%s> %s\n", name, buf);
    addlog("<%s> %s\n", name, buf);
    if(log_lines_nb == LOG_MAX_LINES) {
        free(log[log_pos].author);
        free(log[log_pos].content);
        log_lines_nb--;
    }
    log[log_pos].author = strdup(name);
    log[log_pos].content = strdup(buf);
    log_lines_nb++;
    log_pos = (log_pos+1)%LOG_MAX_LINES;
}

/* Parsage du tableau answers_str de la conf */

void parse_answers()
{
    int str_nb = sizeof(answers_str)/sizeof(char*);

    if(answers_str != NULL) {
        answers_nb = 1;
    }
    int i, n = 0, new_line = 0;
    for(i=0; i<str_nb; i++) {
        if(answers_str[i] == NULL) {
            n++, new_line = 1;
        } else if(new_line && n%2 == 0) {
            answers_nb++;
            new_line = 0;
        }
    }

    answers = calloc(answers_nb, sizeof(Answer));

    int first_part = 1; /* Savoir dans quelle partie
    on se situe; match_cond ou answers_str */
    int ans_id;
    Answer* cur_ans = &answers[ans_id = 0];
    int cond_nb = 0, ans_nb = 0;
    /* Parcours pour remplir cond_nb et ans_nb */
    for(i=0; i<str_nb; i++) {
        if(answers_str[i] == NULL) {
            first_part = !first_part;
            if(!first_part == 0) {
                cur_ans->cond_nb = cond_nb;
                cur_ans->answers_nb = ans_nb;
                cur_ans = &answers[++ans_id];
                cond_nb = 0, ans_nb = 0;
            }
        } else {
            if(first_part) {
                cond_nb++;
            } else {
                ans_nb++;
            }
        }
    }
    cur_ans->cond_nb = cond_nb;
    cur_ans->answers_nb = ans_nb;

    first_part = 1;
    cur_ans = &answers[ans_id = 0];
    int cond_id = 0;
    Match_cond* cur_cond;
    int ans_str_id = 0;
    char** cur_ans_str;
    for(i=0; i < str_nb; i++) {
        if(answers_str[i] == NULL) {
            first_part = !first_part;
            if(!first_part == 0 && ans_id < answers_nb) {
                cur_ans = &answers[++ans_id];
                cond_id = 0, ans_str_id = 0;
            }
        } else {
            if(cur_ans->conditions == NULL) {
                cur_ans->conditions = malloc(cur_ans->cond_nb
                                      *sizeof(Match_cond));
                cur_cond = &(cur_ans->conditions[cond_id = 0]);
            }
            if(cur_ans->answers_str == NULL) {
                cur_ans->answers_str = malloc(cur_ans->answers_nb
                                       *sizeof(char*));
                cur_ans_str = &(cur_ans->answers_str[ans_str_id = 0]);
            }
            if(first_part) {
                if(strstr(answers_str[i], "STARTS_WITH") == answers_str[i]) {
                    cur_cond->pos = STARTS_WITH;
                    cur_cond->str = tolower_str(strdup(&answers_str[i][strlen("STARTS_WITH ")]));
                } else if(strstr(answers_str[i], "DOESNT_START_WITH") == answers_str[i]) {
                    cur_cond->pos = DOESNT_START_WITH;
                    cur_cond->str = tolower_str(strdup(&answers_str[i][strlen("DOESNT_START_WITH ")]));
                } else if(strstr(answers_str[i], "CONTAINS") == answers_str[i]) {
                    cur_cond->pos = CONTAINS;
                    cur_cond->str = tolower_str(strdup(&answers_str[i][strlen("CONTAINS ")]));
                } else if(strstr(answers_str[i], "DOESNT_CONTAIN") == answers_str[i]) {
                    cur_cond->pos = DOESNT_CONTAIN;
                    cur_cond->str = tolower_str(strdup(&answers_str[i][strlen("DOESNT_CONTAIN ")]));
                } else if(strstr(answers_str[i], "ENDS_WITH") == answers_str[i]) {
                    cur_cond->pos = ENDS_WITH;
                    cur_cond->str = tolower_str(strdup(&answers_str[i][strlen("ENDS_WITH ")]));
                } else if(strstr(answers_str[i], "DOESNT_END_WITH") == answers_str[i]) {
                    cur_cond->pos = DOESNT_END_WITH;
                    cur_cond->str = tolower_str(strdup(&answers_str[i][strlen("DOESNT_END_WITH ")]));
                } else {
                    printf("Erreur de syntaxe\n");
                    exit(1);
                }
                if(cond_id < cur_ans->cond_nb)
                    cur_cond = &(cur_ans->conditions[++cond_id]);
            } else {
                *cur_ans_str = strdup(answers_str[i]);
                if(ans_str_id < cur_ans->answers_nb)
                    cur_ans_str = &(cur_ans->answers_str[++ans_str_id]);
            }
        }
    }
}

/* Exécution d'une potentielle commande contenue dans la chaine
en entrée.
Retourne 1 si une commande était présente et a été exécutée,
0 si la chaine ne contenait pas de commande */

int run_cmd(char* raw_line, irc_session_t* session,
                            const char* origin)
{
    int cmd_nb = sizeof(cmds)/sizeof(Cmd);
    int i;
    Cmd* cur_cmd = NULL;
    for(i=0; i<cmd_nb; i++) {
        if(strstr(raw_line, cmds[i].cmd_str) == raw_line) {
            cur_cmd = &cmds[i];
            break;
        }
    }
    if(cur_cmd == NULL) {
        return 0;
    }

    (cur_cmd->f)(session, cur_cmd->cmd_str, origin,
                 &raw_line[strlen(cur_cmd->cmd_str)]);
    return 1;
}

/* Fonctions de gestion des évenements */

void event_join(irc_session_t* session,
                const char* event,
                const char* origin,
                const char **params,
                unsigned int count)
{
    char nick[20];
    irc_target_get_nick(origin, nick, 20);
    log_and_print("-> %s a rejoint %s", nick, params[0]);
    if(!strcmp(nick, botnick) && !silent) {
        irc_cmd_msg(session, params[0], "Salut !");
        log_and_print_said(botnick, "Salut !");
    }
}

void event_part(irc_session_t* session,
                const char* event,
                const char* origin,
                const char **params,
                unsigned int count)
{
    char nick[20];
    irc_target_get_nick(origin, nick, 20);
    log_and_print("<- %s a quitté %s", nick, params[0]);
}

void event_connect(irc_session_t* session,
                   const char* event,
                   const char* origin,
                   const char **params,
                   unsigned int count)
{
    log_and_print("Connecté au serveur %s avec succès", server);
    if(start_msg_dest != NULL && start_msg_content != NULL) {
        irc_cmd_msg(session, start_msg_dest, start_msg_content);
    }
    irc_cmd_join(session, channel, 0);
}

void event_privmsg(irc_session_t* session,
                   const char* event,
                   const char* origin,
                   const char **params,
                   unsigned int count)
{
    if(count == 1) {
       return;
    }
    char* line = tolower_str(skip_blanks(params[1]));
    if(!run_cmd(line, session, origin)) {
        if(strcmp(origin, master_origin) != 0) {
            char nick[20];
            irc_target_get_nick(origin, nick, 20);
            log_and_print("PrivMsg de %s : %s", origin, params[1]);
            if(!silent) {
                irc_cmd_msg(session, nick, "Tu aimes les cachotteries, toi, hein ?");
                char msg[512];
                sprintf(msg, "hé, y a %s qui me dit des trucs pas propres en query :d", nick);
                log_and_print_said(botnick, msg);
                irc_cmd_msg(session, channel, msg);
            }
        }
    }
    free(line);
}

void event_channel(irc_session_t* session,
                   const char* event,
                   const char* origin,
                   const char **params,
                   unsigned int count)
{
    if(count != 2) {
        return;
    }

    char nick[20];
    irc_target_get_nick(origin, nick, 20);
    log_and_print_said(nick, params[1]);

    char* raw_line = tolower_str(skip_blanks(params[1]));
    if(!run_cmd(raw_line, session, origin) && !silent) {
        int i, answered;
        Answer* ans;
        for(i=0, answered=0; i<answers_nb && !answered; i++) {
            int match;
            int j;
            ans = &answers[i];
            Match_cond* conditions = ans->conditions;
            for(j=0, match=1; j < ans->cond_nb && match; j++) {

                int offset;
                switch (conditions[j].pos) {
                    case STARTS_WITH :
                    if(strstr(raw_line, conditions[j].str) != raw_line) {
                        match=0;
                    }
                    break;
                    case DOESNT_START_WITH :
                    if(strstr(raw_line, conditions[j].str) == raw_line) {
                        match=0;
                    }
                    break;
                    case CONTAINS :
                    if(strstr(raw_line, conditions[j].str) == NULL) {
                        match = 0;
                    }
                    break;
                    case DOESNT_CONTAIN :
                    if(strstr(raw_line, conditions[j].str) != NULL) {
                        match = 0;
                    }
                    break;
                    case ENDS_WITH :
                    offset = strlen(raw_line) - strlen(conditions[j].str);
                    if(strstr(raw_line, conditions[j].str) != &raw_line[offset]) {
                        match = 0;
                    }
                    break;
                    case DOESNT_END_WITH :
                    offset = strlen(raw_line) - strlen(conditions[j].str);
                    if(strstr(raw_line, conditions[j].str) == &raw_line[offset]) {
                        match = 0;
                    }
                    break;
                }
            }
            if(match) {
                char* answer_str = pick_string(ans->answers_str, ans->answers_nb);
                log_and_print_said(botnick, answer_str);
                irc_cmd_msg(session, channel, answer_str);
                answered = 1;
            }
        }
    }
    free(raw_line);
}

void event_numeric(irc_session_t* session,
                   unsigned int event,
                   const char* origin,
                   const char **params,
                   unsigned int count)
{
    return;
}

int main(int argc, char** argv)
{
    irc_callbacks_t callbacks;
    irc_session_t* s;

    memset(&callbacks, 0, sizeof(callbacks));

    callbacks.event_connect = event_connect;
    callbacks.event_join = event_join;
    callbacks.event_channel = event_channel;
    callbacks.event_privmsg = event_privmsg;
    callbacks.event_numeric = event_numeric;
    callbacks.event_part = event_part;
    callbacks.event_quit = event_part;

    callbacks.event_mode = dump_event;
    callbacks.event_invite = dump_event;
    callbacks.event_umode = dump_event;
    callbacks.event_ctcp_rep = dump_event;
    callbacks.event_ctcp_action = dump_event;
    callbacks.event_unknown = dump_event;
    callbacks.event_nick = dump_event;
    callbacks.event_topic = dump_event;
    callbacks.event_kick = dump_event;
    callbacks.event_notice = dump_event;

    parse_answers();

    s = irc_create_session(&callbacks);

    if(!s) {
        printf("Impossible d'initialiser la session\n");
        return 1;
    }

    if(irc_connect(s, server, 6667, 0, botnick, botnick, botnick)) {
        printf("Impossible de se connecter : %s\n", irc_strerror(irc_errno(s)));
        return 1;
    }

    irc_run(s);

    return 1;
}
