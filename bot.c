#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <signal.h>
#include <ctype.h>
#include <time.h>
#include <regex.h>

#include <libircclient/libircclient.h>

#ifdef WITH_MPD
#include <mpd/client.h>
#include <mpd/status.h>
#include <mpd/tag.h>
#endif

#include "bot.h"
#include "config.h"
#include "commands.h"
extern Cmd cmds[];

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

void debug_print_log()
{
    int i, pos = mod(log_pos-1, LOG_MAX_LINES);
    printf("log_lines_nb : %d\n", log_lines_nb);
    printf("log_pos : %d\n", log_pos);
    for(i = log_lines_nb; i > 0; i--, pos = mod(pos-1, LOG_MAX_LINES)) {
        printf("log: <%s> %s\n", log[pos].author, log[pos].content);
    }
}

void free_log()
{
    int i, pos = mod(log_pos-1, LOG_MAX_LINES);
    for(i = log_lines_nb; i > 0; i--, pos = mod(pos-1, LOG_MAX_LINES)) {
        free(log[pos].author);
        free(log[pos].content);
    }
}

void die(int sig)
{
    if(sig) {
        printf("Signal %d reçu. ", sig);
    }
    printf("Exiting...");
    free_log();
    exit(0);
}

/* Exécution d'une potentielle commande contenue dans la chaine
en entrée.
Retourne 1 si une commande était présente et a été exécutée,
0 si la chaine ne contenait pas de commande */

int run_cmd(char* raw_line, irc_session_t* session,
                            const char* origin)
{
    int cmd_nb = get_cmd_nb();
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

    char* raw_line = tolower_str(skip_blanks(params[1]));
    if(!run_cmd(raw_line, session, origin)) {
        char nick[20];
        int i, answered;
        Answer* ans;
        regex_t preg;

        irc_target_get_nick(origin, nick, 20);
        log_and_print_said(nick, params[1]);

        if(!silent) {
            for(i=0, answered=0; i<answers_nb && !answered; i++) {
                ans = &answers[i];
                regcomp(&preg, ans->regex, REG_ICASE|REG_NOSUB|REG_EXTENDED);

                if(regexec(&preg, raw_line, 0, NULL, 0) == 0) {
                    char* answer_str = pick_string(ans->answers, ans->answers_nb);
                    log_and_print_said(botnick, answer_str);
                    irc_cmd_msg(session, channel, answer_str);
                    answered = 1;
                }
                regfree(&preg);
            }
        }
    }
    free(raw_line);
}

int main(int argc, char** argv)
{
    irc_callbacks_t callbacks;
    irc_session_t* s;

    signal(SIGINT, die);
    signal(SIGTERM, die);
    signal(SIGSTOP, die);
    signal(SIGHUP, die);

    memset(&callbacks, 0, sizeof(callbacks));

    callbacks.event_connect = event_connect;
    callbacks.event_join = event_join;
    callbacks.event_channel = event_channel;
    callbacks.event_privmsg = event_privmsg;
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

    die(0);
    return 1;
}
