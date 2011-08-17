#include <libircclient/libircclient.h>
#ifdef WITH_MPD
#include <mpd/client.h>
#include <mpd/status.h>
#include <mpd/tag.h>
#endif

#ifndef H_CMD
#define H_CMD

/* Structures décrivant les commandes prédéfinies */

typedef struct Cmd {
    char* cmd_str;
    char* cmd_description;
    void (*f)(irc_session_t* session,
              const char* cmd,
              const char* origin,
              char* args);
} Cmd;

int get_cmd_nb();

void cmd_help(irc_session_t* session, const char* cmd, const char* origin, char* args);
void cmd_say(irc_session_t* session, const char* cmd, const char* origin, char* args);
void cmd_silent(irc_session_t* session, const char* cmd, const char* origin, char* args);
void cmd_quit(irc_session_t* session, const char* cmd, const char* origin, char* args);
void cmd_replace(irc_session_t* session, const char* cmd, const char* origin, char* args);

#ifdef WITH_MPD
void cmd_np(irc_session_t* session, const char* cmd, const char* origin, char* args);
void cmd_next(irc_session_t* session, const char* cmd, const char* origin, char* args);
void cmd_prev(irc_session_t* session, const char* cmd, const char* origin, char* args);

void cmd_pause(irc_session_t* session, const char* cmd, const char* origin, char* args);
void cmd_play(irc_session_t* session, const char* cmd, const char* origin, char* args);
#endif

#endif

