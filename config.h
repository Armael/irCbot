static const char* server        = "irc.freenode.net";
static const char* channel       = "#arch-fr-off";
static const char* botnick       = "Merena";
static const char* master_origin = "Armael!~Armael@unaffiliated/armael";

static int  silent = 0;

static const char* log_file = "merena.log";

static const char* start_msg_dest = "Nickserv";
static const char* start_msg_content = "identify fgrqrr";

static const char* answers_str[] = { "CONTAINS Merena",
                                     "CONTAINS bot",
                                     NULL,
                                     "hé, j'suis pas un bot !",
                                     NULL,
                                     "STARTS_WITH Merena",
                                     "ENDS_WITH ?",
                                     NULL,
                                     "oui", "non", "je sais pas trop", "42",
                                     NULL,
                                     "DOESNT_START_WITH Merena",
                                     "CONTAINS Merena",
                                     NULL,
                                     "hm ?", "wut ?", "on parle de moi ?", "kesketatuveuxtebattre ?",
                                     NULL,
                                     "STARTS_WITH Merena",
                                     NULL,
                                     "Humpf", "hé, attend un peu, je fais autre chose là", "mer il et fou", "wtf" };
