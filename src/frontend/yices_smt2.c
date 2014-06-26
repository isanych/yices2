/*
 * Yices solver: input in the SMT-LIB 2.0 language
 */

#if defined(CYGWIN) || defined(MINGW)
#ifndef __YICES_DLLSPEC__
#define __YICES_DLLSPEC__ __declspec(dllexport)
#endif
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <signal.h>
#include <inttypes.h>
// EXPERIMENT
#include <locale.h>

#include "utils/command_line.h"


#include "frontend/smt2/smt2_lexer.h"
#include "frontend/smt2/smt2_parser.h"
#include "frontend/smt2/smt2_term_stack.h"
#include "frontend/smt2/smt2_commands.h"

#include "include/yices.h"
#include "include/yices_exit_codes.h"


/*
 * Global objects:
 * - lexer/parser/stack: for processing the SMT2 input
 * - incremental: if this flag is true, support for push/pop
 *   and multiple check_sat is enabled. Otherwise, the solver
 *   is configured to handle a set of declarations/assertions
 *   followed by a single call to (check_sat).
 * - interactive: if this flag is true, print a prompt before
 *   parsing commands. Also set the option :print-success to true.
 *
 * - filename = name of the input file (NULL means read stdin)
 */
static lexer_t lexer;
static parser_t parser;
static tstack_t stack;

static bool incremental;
static bool interactive;
static bool show_stats;
static int32_t verbosity;
static char *filename;


/****************************
 *  COMMAND-LINE ARGUMENTS  *
 ***************************/

typedef enum optid {
  show_version_opt,     // print version and exit
  show_help_opt,        // print help and exit
  show_stats_opt,       // show statistics after all commands are processed
  verbosity_opt,        // set verbosity on the command line
  incremental_opt,      // enable incremental mode
  interactive_opt,      // enable interactive mode
} optid_t;

#define NUM_OPTIONS (interactive_opt+1)

/*
 * Option descriptors
 */
static option_desc_t options[NUM_OPTIONS] = {
  { "version", 'V', FLAG_OPTION, show_version_opt },
  { "help", 'h', FLAG_OPTION, show_help_opt },
  { "stats", 's', FLAG_OPTION, show_stats_opt },
  { "verbosity", 'v', MANDATORY_INT, verbosity_opt },
  { "incremental", '\0', FLAG_OPTION, incremental_opt },
  { "interactive", '\0', FLAG_OPTION, interactive_opt },
};


/*
 * Processing of command-line
 */
static void print_version(void) {
  printf("Yices %s\n"
	 "Copyright SRI International.\n"
         "Linked with GMP %s\n"
	 "Copyright Free Software Foundation, Inc.\n"
         "Build date: %s\n"
         "Platform: %s (%s)\n",
         yices_version, gmp_version,
         yices_build_date, yices_build_arch, yices_build_mode);
  fflush(stdout);
}

static void print_help(const char *progname) {
  printf("Usage: %s [option] filename\n"
         "    or %s [option]\n", progname, progname);
  printf("Option summary:\n"
	 "    --version, -V           Show version and exit\n"
	 "    --help, -h              Print this message and exit\n"
	 "    --verbosity=<level>     Set verbosity level (default = 0)\n"
	 "             -v <level>\n"
	 "    --stats, -s             Print statistics once all commands have been processed\n"
	 "    --incremental           Enable support for push/pop\n"
	 "    --interactive           Run in interactive mode (ignored if a filename is given)\n"
	 "\n"
	 "For bug reports and ohter information, please see http://yices.csl.sri.com/\n");
  fflush(stdout);
}

/*
 * Message for unrecognized options or other errors on the command line.
 */
static void print_usage(const char *progname) {
  fprintf(stderr, "Usage: %s [options] filename\n", progname);
  fprintf(stderr, "Try '%s --help' for more information\n", progname);
}


/*
 * Parse the command line and process options
 */
static void parse_command_line(int argc, char *argv[]) {
  cmdline_parser_t parser;
  cmdline_elem_t elem;
  optid_t k;
  int32_t v;

  filename = NULL;
  incremental = false;
  interactive = false;
  show_stats = false;
  verbosity = 0;

  init_cmdline_parser(&parser, options, NUM_OPTIONS, argv, argc);

  for (;;) {
    cmdline_parse_element(&parser, &elem);
    switch (elem.status) {
    case cmdline_done:
      goto done;

    case cmdline_argument:
      if (filename == NULL) {
	filename = elem.arg;
      } else {
	fprintf(stderr, "%s: too many arguments\n", parser.command_name);
	print_usage(parser.command_name);
	exit(YICES_EXIT_USAGE);
      }
      break;

    case cmdline_option:
      k = elem.key;
      switch (k) {
      case show_version_opt:
	print_version();
	exit(YICES_EXIT_SUCCESS);

      case show_help_opt:
	print_help(parser.command_name);
	exit(YICES_EXIT_SUCCESS);

      case show_stats_opt:
	show_stats = true;
	break;

      case verbosity_opt:
	v = elem.i_value;
	if (v < 0) {
	  fprintf(stderr, "%s: the verbosity level must be non-negative\n", parser.command_name);
	  print_usage(parser.command_name);
	  exit(YICES_EXIT_USAGE);
	}
	verbosity = v;
	break;

      case incremental_opt:
	incremental = true;
	break;

      case interactive_opt:
	interactive = true;
	break;
      }
      break;

    case cmdline_error:
      cmdline_print_error(&parser, &elem);
      fprintf(stderr, "Try %s --help for more information\n", parser.command_name);
      exit(YICES_EXIT_USAGE);
    }
  }

 done:
  // force interactive to false if there's a filename
  if (filename != NULL) {
    interactive = false;
  }
  return;
}

/********************
 *  SIGNAL HANDLER  *
 *******************/

/*
 * We call exit on SIGINT/ABORT and XCPU
 * - we could try to handle SIGINT more gracefully in interactive mode
 * - this will do for now.
 */
static void default_handler(int signum) {
  if (verbosity > 0) {
    fprintf(stderr, "\nInterrupted by signal %d\n", signum);
    fflush(stderr);
  }
  exit(YICES_EXIT_INTERRUPTED);
}


/*
 * Initialize the signal handlers
 */
static void init_handlers(void) {
  signal(SIGINT, default_handler);
  signal(SIGABRT, default_handler);
#ifndef MINGW
  signal(SIGXCPU, default_handler);
#endif
}


/*
 * Reset the default handlers
 */
static void reset_handlers(void) {
  signal(SIGINT, SIG_DFL);
  signal(SIGABRT, SIG_DFL);
#ifndef MINGW
  signal(SIGXCPU, SIG_DFL);
#endif
}



/**********
 *  MAIN  *
 *********/

#define HACK_FOR_UTF 0

#if HACK_FOR_UTF

/*
 * List of locales to try
 */
#define NUM_LOCALES 3

static const char *const locales[NUM_LOCALES] = {
  "C.UTF-8", "en_US.utf8", "en_US.UTF-8",
};

// HACK TO FORCE UTF8
static void force_utf8(void) {
  uint32_t i;

  for (i=0; i<NUM_LOCALES; i++) {
    if (setlocale(LC_CTYPE, locales[i]) != NULL) {
      if (verbosity > 1) {
	fprintf(stderr, "Switched to locale '%s'\n", setlocale(LC_CTYPE, NULL));
	fflush(stderr);
      }
      return;
    }
  }

  fprintf(stderr, "Failed to switch locale to UTF-8. Current locale is '%s'\n", setlocale(LC_CTYPE, NULL));
  fflush(stderr);
}

#else

static void force_utf8(void) {
  // Do nothing
}

#endif

int main(int argc, char *argv[]) {
  int32_t code;

  parse_command_line(argc, argv);
  force_utf8();

  if (filename != NULL) {
    // read from file
    if (init_smt2_file_lexer(&lexer, filename) < 0) {
      perror(filename);
      exit(YICES_EXIT_FILE_NOT_FOUND);
    }
  } else {
    // read from stdin
    init_smt2_stdin_lexer(&lexer);
  }

  init_handlers();

  yices_init();
  init_smt2(!incremental, interactive);
  init_smt2_tstack(&stack);
  init_parser(&parser, &lexer, &stack);
  if (verbosity > 0) {
    smt2_set_verbosity(verbosity);
  }

  while (smt2_active()) {
    if (interactive) {
      // prompt
      fputs("yices> ", stderr);
      fflush(stderr);
    }
    code = parse_smt2_command(&parser);
    if (code < 0) {
      // syntax error
      if (interactive) {
	flush_lexer(&lexer);
      } else {
	break; // exit
      }
    }
  }

  if (show_stats) {
    smt2_show_stats();
  }

  delete_parser(&parser);
  close_lexer(&lexer);
  delete_tstack(&stack);
  delete_smt2();
  yices_exit();

  reset_handlers();

  return YICES_EXIT_SUCCESS;
}
