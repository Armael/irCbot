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

Cmd cmds[] = {
                 { "!quit", cmd_quit },
                 { "!say", cmd_say },
                 { "!silent", cmd_silent }
             };
