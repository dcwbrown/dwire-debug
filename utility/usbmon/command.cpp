#include "command.hpp"
#include "usbmon.hpp"

#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>

#include <getopt.h>
#include <termios.h>
#include <unistd.h>
#include <sys/select.h>

void Debug(const char* f, ...)
{
	va_list a; va_start(a, f);
	vfprintf(stderr, f, a);
	va_end(a);
}

void SysAbort(const char* f, ...)
{
	if (f)
	{
		fprintf(stderr, "SysAbort: ");
		va_list a; va_start(a, f);
		vfprintf(stderr, f, a);
		va_end(a);
		fprintf(stderr, " ");
	}
	abort();
}

void SysAbort_unhandled()
{
	std::cerr << "SysAbort: ";
	std::exception_ptr ep = std::current_exception();
	try {
		std::rethrow_exception(ep);
	} catch (std::exception& ex) {
		const char* error = ex.what();
		std::cerr << (error ? error : "std::exception unhandled");
	} catch (...) {
		std::cerr << "... unhandled";
	}
	std::cerr << '\n';
	abort();
}

int SysConWrite(const void* s)
{
  size_t l = strlen((const char*)s);
  return write(STDOUT_FILENO, s, l) == l;
}

int SysConAvail()
{
	timeval tv = { 0, 0 };
	fd_set fds; FD_ZERO(&fds);
	FD_SET(STDIN_FILENO, &fds);
	return select(STDIN_FILENO+1, &fds, NULL, NULL, &tv);
}

int SysConRead()
{
	unsigned char c;
	int r = read(STDIN_FILENO, &c, sizeof(c));
	return r <= 0 ? 0 : (unsigned int)c;
}

char* opt_device = nullptr;
int opt_power = false;
int opt_reset = false;
int opt_debug = false;
int opt_nomon = false;
char* opt_set_serial = nullptr;
static termios ttystate;
void reset_terminal_mode() { tcsetattr(STDIN_FILENO, TCSANOW, &ttystate); }

void args(int argc, char** argv) {
  // true to print optopt errors to stderr, getopt returns '?' in either case
  opterr = true;
  struct usage { };
  static char usageText[]
    = R"usageText(usage: usbmon [OPTION]...

  -d, --device T#        serial number prefixed by device type character
                         (Usb|usbTiny|Serial)
  -p, --power            cycle (requires dw)
  -r, --reset            (requires usb)
  -l, --load FILE        hex (experimental)
  -n, --no-monitor       don't launch monitor

  --debug                show debug messages
  --set-serial #         update serial number (requires dw)
)usageText";
  try {
    enum {
      lo_flag,
      lo_set_serial,
    };
    static struct option options[] = {
      {"device",         required_argument, 0, 'd' },
      {"power",          no_argument,       0, 'p' },
      {"reset",          no_argument,       0, 'r' },
      {"load",           required_argument, 0, 'l' },
      {"no-monitor",     no_argument,       0, 'n' },
      {"debug",          no_argument,       &opt_debug, true },
      {"set-serial",     required_argument, 0, lo_set_serial },
      {nullptr, 0, nullptr, 0}
    };
    for (int c, i; (c = getopt_long(argc, argv, "d:prl:nt", options, &i)) != -1; ) {
      // optopt gets option character for unknown option or missing required argument
      // optind gets index of the next element of argv to be processed
      // optarg points to value of option's argument
      switch (c) {
      default:
        throw usage();

      case 0: // set a flag
        break;

      case 'd': // device
        opt_device = optarg;
        break;

      case 'p': // power
        opt_power = true;
        break;

      case 'r': // reset
        opt_reset = true;
        break;

      case 'l': //load
        load(optarg);
        break;

      case 'n': // no-monitor
        opt_nomon = true;
        break;

      case lo_set_serial:
        opt_set_serial = optarg;
        break;

      case 't':
        test();
        break;

      }
    }
    if (optind < argc) {

      // process positional arguments
      std::cerr << argv[0] << ": invalid argument ";
      for (int i = optind; i < argc; ++i)
        std::cerr << argv[i] << ' ';
      std::cerr << std::endl;
      exit(3);
    }
  } catch (usage) {
    std::cerr << usageText;
    exit(3);
  }
  if (!opt_device || (*opt_device != 'u' && *opt_device != 't' && *opt_device != 's')) {
    std::cerr << "invalid device\n\n" << usageText;
    exit(3);
  }
}

int main(int argc, char *argv[]) {
  // make unbuffered
  setbuf(stdout, nullptr);
  setbuf(stderr, nullptr);

//http://stackoverflow.com/questions/12625224/can-someone-give-me-an-example-of-how-select-is-alerted-to-an-fd-becoming-rea
//http://stackoverflow.com/questions/448944/c-non-blocking-keyboard-input
//http://cc.byexamples.com/2007/04/08/non-blocking-user-input-in-loop-without-ncurses/

  // get console settings
  tcgetattr(STDIN_FILENO, &ttystate);
  atexit(reset_terminal_mode);
{
  // set console noblock and noecho modes
  termios ttystate_new = ttystate;
  ttystate_new.c_lflag &= ~(ICANON | ECHO);
  ttystate.c_cc[VMIN] = 1; ttystate.c_cc[VTIME] = 0;
  tcsetattr(STDIN_FILENO, TCSANOW, &ttystate_new);
}
  // process arguments
  args(argc, argv);

  // execute commands
  monitor();

  return 0;
}
