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

int SysConWrite(const char* s)
{
  size_t l = strlen(s);
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

char* serialnumber = nullptr;
static termios ttystate;
void reset_terminal_mode() { tcsetattr(STDIN_FILENO, TCSANOW, &ttystate); }

void args(int argc, char** argv) {
  // true to print optopt errors to stderr, getopt returns '?' in either case
  opterr = true;
  struct usage { };
  try {
    enum {
      lo_flag,
      lo_change_serialnumber,
    };
    static struct option options[] = {
      {"change-serialnumber", required_argument, 0, lo_change_serialnumber},
      {nullptr, 0, nullptr, 0}
    };
    for (int c, i; (c = getopt_long(argc, argv, "s:rpdut", options, &i)) != -1; ) {
      // optopt gets option character for unknown option or missing required argument
      // optind gets index of the next element of argv to be processed
      // optarg points to value of option's argument
      switch (c) {
      default:
        throw usage();

      case 's':
        serialnumber = strdup(optarg);
        break;

      case lo_change_serialnumber:
        change_serialnumber(optarg);
        break;

      case 'r':
        reset();
        break;

      case 'p':
        power();
        break;

      case 'd':
        monitor_dw();
        break;

      case 'u':
        monitor();
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
    std::cerr << R"00here-text00(usage: rpcusbmon [OPTION]...
  -s #   require serial number
  -r     reset (requires usb)
  -p     power cycle (requires dw)
  -d     dw monitor
  -u     usb monitor

  --change-serialnumber #   change to new serial number (requires dw)
)00here-text00";
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

  return 0;
}
