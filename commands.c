#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libircclient/libircclient.h>
#include <regex.h>

#include "bot.h"
#include "config.h"
#include "commands.h"

Cmd cmds[] = {
                 { "!help", "Liste les commandes ou donne la description d'une commande particulière passée en argument", cmd_help },
                 { "!quit", "Fait quitter le bot", cmd_quit },
                 { "!say", "Fait dire son argument au bot", cmd_say },
                 { "!silent", "Arguments : \"on\" ou \"off\" : passe le bot en mode silencieux ou bavard", cmd_silent },
                 { "s/", "Remplace une chaine dans ce qui a été dit (sed-like)", cmd_replace },
#ifdef WITH_MPD
                 { "!np", "Affiche la piste en cours de lecture", cmd_np },
                 { "!next", "Passe à la piste suivante", cmd_next },
                 { "!prev", "Passe à la piste précédente", cmd_prev },
                 { "!pause", "Met en pause", cmd_pause },
                 { "!play", "Lit le morceau en cours si la lecture a été arrêtée", cmd_play },
#endif
                 { NULL, NULL, NULL }
             };

int get_cmd_nb() {
    int i;
    for(i = 0; cmds[i].cmd_str != NULL; i++);
    return i;
}

void cmd_help(irc_session_t* session, const char* cmd, const char* origin, char* args) {
    int cmd_nb = get_cmd_nb();
    int i;

    if(args[0] == '\0') {
        int out_len = 1 + 12 /* "Commandes : " */;
        char* out_str;

        for(i=0; i<cmd_nb; i++)
            out_len += strlen(cmds[i].cmd_str) + 1 /* ' ' */;

        out_str = malloc(out_len * sizeof(char));
        out_str[0] = '\0';
        for(i=0; i<cmd_nb; i++) {
            strcat(out_str, cmds[i].cmd_str);
            strcat(out_str, " ");
        }
        irc_cmd_msg(session, channel, out_str);
        log_and_print_said(botnick, out_str);
        free(out_str);
    } else {
        for(i=0; i<cmd_nb; i++) {
            if(strcmp(cmds[i].cmd_str, &args[1]) == 0) {
                irc_cmd_msg(session, channel, cmds[i].cmd_description);
                log_and_print_said(botnick, cmds[i].cmd_description);
            }
        }
    }
}

void cmd_say(irc_session_t* session, const char* cmd, const char* origin, char* args) {
    if(args[0] != '\0') {
        char* what_to_say = &args[1]; /* l'espace après 'say' */
        irc_cmd_msg(session, channel, what_to_say);
        char nick[20];
        irc_target_get_nick(origin, nick, 20);
        log_and_print("%s m'a dit de dire : %s", nick, what_to_say);
        log_and_print_said(botnick, what_to_say);
    }
}

void cmd_silent(irc_session_t* session, const char* cmd, const char* origin, char* args) {
    if(!strcmp(origin, master_origin)) {
        if(args[0] != '\0') {
            if(!strcmp(&args[1], "on")) {
                silent = 1;
                log_and_print("Mode silencieux");
            } else if(!strcmp(&args[1], "off")) {
                silent = 0;
                log_and_print("Mode bavard");
            }
        }
    }
}

void cmd_quit(irc_session_t* session, const char* cmd, const char* origin, char* args) {
    if(!strcmp(origin, master_origin)) {
        if(args[0] == '\0') {
            irc_cmd_msg(session, channel, "bien sûr, Maître !");
            irc_cmd_quit(session, NULL);
        }
    } else {
        char nick[20];
        irc_target_get_nick(origin, nick, 20);
        char msg[100];
        sprintf(msg, "%s: bien essayé :>", nick);
        irc_cmd_msg(session, channel, msg);
    }
}

void cmd_replace(irc_session_t* session, const char* cmd, const char* origin, char* args) {
    char* regex = malloc((strlen(args) + 2) * sizeof(char));
    char* replace_str = malloc(strlen(args) * sizeof(char));
    int i, j, replace_str_len;
    regex_t preg;
    size_t nmatch = 0;
    regmatch_t* pmatch = NULL;

    regex[0] = '(';
    for(i = 0; (args[i] != '/') || (i>0 && args[i-1] == '\\' && args[i] == '/'); i++)
        regex[i+1] = args[i];
    regex[i++ + 1] = ')';
    regex[i+1] = '\0';

    for(j = 0; ((args[i] != '/') || (args[i-1] == '\\' && args[i] == '/')) && (args[i] != '\0'); i++, j++)
        replace_str[j] = args[i];
    replace_str[j] = '\0';
    replace_str_len = j;

    regcomp(&preg, regex, REG_EXTENDED|REG_ICASE);
    nmatch = preg.re_nsub;
    pmatch = malloc(sizeof(*pmatch) * nmatch);

    if(pmatch) {
        int j;
        int pos = mod(log_pos-1, LOG_MAX_LINES);
        /* Backlog pour trouver une ligne qui matche */
        for(j = log_lines_nb; j > 0; j--, pos = mod(pos-1, LOG_MAX_LINES)) {
            if(regexec(&preg, log_array[pos].content, nmatch, pmatch, 0) == 0) {
                char* match_str = NULL;
                int start = pmatch[0].rm_so;
                int end = pmatch[0].rm_eo;
                size_t size = end - start;

                match_str = malloc((size + 1) * sizeof(char));
                if(match_str) {
                    strncpy(match_str, &(log_array[pos].content[start]), size);
                    match_str[size] = '\0';

                    char* new_str = malloc((strlen(log_array[pos].content)
                                            - size + replace_str_len + 1) * sizeof(char));
                    if(new_str) {
                        int k;
                        char* matched_pos = strstr(log_array[pos].content, match_str);
                        for(k=0; &log_array[pos].content[k] != matched_pos; k++)
                            new_str[k] = log_array[pos].content[k];
                        new_str[k] = '\0';
                        strcat(new_str, replace_str);
                        strcat(new_str, &log_array[pos].content[k+size]);

                        char* what_to_say = malloc((strlen(log_array[pos].author)
                                                    + 3 + strlen(new_str) + 1)
                                                    * sizeof(char));
                        if(what_to_say) {
                            sprintf(what_to_say, "< %s> %s", log_array[pos].author, new_str);
                            irc_cmd_msg(session, channel, what_to_say);
                            log_and_print_said(botnick, what_to_say);
                            free(what_to_say);
                        }
                        free(new_str);
                    }
                    free(match_str);
                }
                break;
            }
        }
        free(pmatch);
    }
    regfree(&preg);
    free(regex);
    free(replace_str);
}

#ifdef WITH_MPD
void cmd_np(irc_session_t* session, const char* cmd, const char* origin, char* args) {
    struct mpd_connection* conn;
    struct mpd_song* song;
    char master_nick[20];
    const char* title;
    const char* album;
    const char* artist;

    int out_len = 20 + 18 /* master_nick + " is now playing: "*/;
    char* out_str;

    irc_target_get_nick(master_origin, master_nick, 20);

    conn = mpd_connection_new(NULL, 0, 30000);

    if(mpd_connection_get_error(conn) != MPD_ERROR_SUCCESS) {
        log_and_print("Impossible de se connecter à MPD");
        mpd_connection_free(conn);

        return;
    }

    mpd_send_current_song(conn);
    song = mpd_recv_song(conn);
    if(song == NULL) {
        out_len += strlen("THE GAME");
        out_str = malloc(out_len * sizeof(char));
        strcpy(out_str, master_nick);
        strcat(out_str, " is now playing: THE GAME");
        irc_cmd_msg(session, channel, out_str);
        free(out_str);
        return;
    }

    title = mpd_song_get_tag(song, MPD_TAG_TITLE, 0);
    album = mpd_song_get_tag(song, MPD_TAG_ALBUM, 0);
    artist = mpd_song_get_tag(song, MPD_TAG_ARTIST, 0);
    if(title)
        out_len += strlen(title) + 2;
    if(album)
        out_len += strlen(album) + 2;
    if(artist)
        out_len += strlen(artist) + 2;

    out_str = malloc(out_len * sizeof(char));
    strcpy(out_str, master_nick);
    strcat(out_str, " is now playing: ");
    if(title)
        strcat(out_str, title);
    if(artist) {
        strcat(out_str, " - ");
        strcat(out_str, artist);
    }
    if(album) {
        strcat(out_str, " - ");
        strcat(out_str, album);
    }
    irc_cmd_msg(session, channel, out_str);
    free(out_str);
    mpd_song_free(song);
    mpd_connection_free(conn);
}

void cmd_next(irc_session_t* session, const char* cmd, const char* origin, char* args) {
    struct mpd_connection* conn;

    conn = mpd_connection_new(NULL, 0, 30000);

    if(mpd_connection_get_error(conn) != MPD_ERROR_SUCCESS) {
        log_and_print("Impossible de se connecter à MPD");
        mpd_connection_free(conn);
        return;
    }

    mpd_run_next(conn);
    mpd_connection_free(conn);
}

void cmd_prev(irc_session_t* session, const char* cmd, const char* origin, char* args) {
    struct mpd_connection* conn;

    conn = mpd_connection_new(NULL, 0, 30000);

    if(mpd_connection_get_error(conn) != MPD_ERROR_SUCCESS) {
        log_and_print("Impossible de se connecter à MPD");
        mpd_connection_free(conn);
        return;
    }

    mpd_run_previous(conn);
    mpd_connection_free(conn);
}

void cmd_pause(irc_session_t* session, const char* cmd, const char* origin, char* args) {
    struct mpd_connection* conn;

    conn = mpd_connection_new(NULL, 0, 30000);

    if(mpd_connection_get_error(conn) != MPD_ERROR_SUCCESS) {
        log_and_print("Impossible de se connecter à MPD");
        mpd_connection_free(conn);
        return;
    }

    mpd_run_pause(conn, 1);
    mpd_connection_free(conn);
}

void cmd_play(irc_session_t* session, const char* cmd, const char* origin, char* args) {
    struct mpd_connection* conn;

    conn = mpd_connection_new(NULL, 0, 30000);

    if(mpd_connection_get_error(conn) != MPD_ERROR_SUCCESS) {
        log_and_print("Impossible de se connecter à MPD");
        mpd_connection_free(conn);
        return;
    }

    mpd_run_play(conn);
    mpd_connection_free(conn);
}

#endif
