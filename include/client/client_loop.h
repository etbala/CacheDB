#pragma once
#include "transport.h"
#include <iosfwd>

// A function that runs the interactive loop (REPL).
// It depends on a transport and streams that can be swapped in tests.
int run_client_repl(ITransport& transport,
                    std::istream& in,
                    std::ostream& out,
                    std::ostream& err);
