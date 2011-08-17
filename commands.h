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
            if(regexec(&preg, log[pos].content, nmatch, pmatch, 0) == 0) {
                char* match_str = NULL;
                int start = pmatch[0].rm_so;
                int end = pmatch[0].rm_eo;
                size_t size = end - start;

                match_str = malloc((size + 1) * sizeof(char));
                if(match_str) {
                    strncpy(match_str, &(log[pos].content[start]), size);
                    match_str[size] = '\0';

                    char* new_str = malloc((strlen(log[pos].content)
                                            - size + replace_str_len + 1) * sizeof(char));
                    if(new_str) {
                        int k;
                        char* matched_pos = strstr(log[pos].content, match_str);
                        for(k=0; &log[pos].content[k] != matched_pos; k++)
                            new_str[k] = log[pos].content[k];
                        new_str[k] = '\0';
                        strcat(new_str, replace_str);
                        strcat(new_str, &log[pos].content[k+size]);

                        char* what_to_say = malloc((strlen(log[pos].author)
                                                    + 3 + strlen(new_str) + 1)
                                                    * sizeof(char));
                        if(what_to_say) {
                            sprintf(what_to_say, "<%s> %s", log[pos].author, new_str);
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

Cmd cmds[] = {
                 { "!quit", cmd_quit },
                 { "!say", cmd_say },
                 { "!silent", cmd_silent },
                 { "s/", cmd_replace }
             };
