#pragma once

namespace nix {

/* Some ANSI escape sequences. */
#ifndef _WIN32

#define ANSI_NORMAL "\e[0m"
#define ANSI_BOLD "\e[1m"
#define ANSI_FAINT "\e[2m"
#define ANSI_ITALIC "\e[3m"
#define ANSI_RED "\e[31;1m"
#define ANSI_GREEN "\e[32;1m"
#define ANSI_YELLOW "\e[33;1m"
#define ANSI_BLUE "\e[34;1m"
#define ANSI_MAGENTA "\e[35;1m"
#define ANSI_CYAN "\e[36;1m"

#else

#define ANSI_NORMAL ""
#define ANSI_BOLD ""
#define ANSI_FAINT ""
#define ANSI_ITALIC ""
#define ANSI_RED ""
#define ANSI_GREEN ""
#define ANSI_YELLOW ""
#define ANSI_BLUE ""
#define ANSI_MAGENTA ""
#define ANSI_CYAN ""

#endif

}
