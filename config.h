#ifndef H_CONFIG
#define H_CONFIG

static const char* server        = "irc.freenode.net";
static const char* channel       = "#arch-fr-off";
static const char* botnick       = "Merena";
static const char* master_origin = "Armael!~Armael@unaffiliated/armael";

static int  silent = 0;

static const char* log_file = "merena.log";

static const char* start_msg_dest = "Nickserv";
static const char* start_msg_content = "identify xxxxxx";
static const char* join_msg = "Salut";

static Answer answers[] = { { "Merena.*bot",
                              1,
                              { "hé, j'suis pas un bot !" }
                            },
/*                            { "Merena.*Merena",
                              4,
                              { "hm ?", "wut ?", "on parle de moi ?", "kesketatuveuxtebattre ?" }
			     },*/
                            { "^Merena.*\\?$",
                              4,
                              { "oui", "non", "je sais pas trop", "42" }
                            },
                            { "^Merena",
                              4,
                              { "Humpf", "hé, attend un peu, je fais autre chose là", "mer il et fou", "wtf" }
                            }
                          };
static const int answers_nb = 3;

#endif
