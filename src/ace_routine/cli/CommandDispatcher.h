/*
MIT License

Copyright (c) 2018 Brian T. Park

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#ifndef ACE_ROUTINE_COMMAND_DISPATCHER_H
#define ACE_ROUTINE_COMMAND_DISPATCHER_H

#include <Arduino.h> // Print
#include <AceRoutine.h>
#include "StreamReader.h"

namespace ace_routine {
namespace cli {

/** Signature for a command handler. */
typedef void (*CommandHandler)(Print& printer, int argc, const char** argv);

/**
 * A record of the command name and its handler. The helpString is the
 * "usage" string, excluding the name of the command itself to save space.
 * The name of the command will be automatically added by the 'help
 * command' handler.
 */
struct DispatchRecord {
  CommandHandler command;
  const char* name;
  const char* helpString;
};

/**
 * A coroutine that reads lines from the Serial port, tokenizes the line on
 * whitespace boundaries, and calls the appropriate command handler to handle
 * the command. Command have the form "command arg1 arg2 ...", where the 'arg*'
 * can be any string.
 *
 * The calling code is expected to provide a mapping of the command string
 * (e.g. "list") to its command handler (CommandHandler). The CommandHandler is
 * called with the number of arguments (argc) and the array of tokens (argv),
 * just like the arguments of the C-language main() function.
 */
class CommandDispatcher: public Coroutine {
  public:
    /**
     * Constructor.
     *
     * @param streamReader An instance of StreamReader.
     * @param printer The output object, normally the global Serial object.
     * @param dispatchTable An array of DispatchRecords.
     * @param numCommands Number of entries in the dispatchTable.
     * @param argv Array of (const char*) that will be used to hold the word
     * tokens of a command line string.
     * @param argvSize The size of the argv array. Tokens which are beyond this
     * limit will be silently dropped.
     */
    CommandDispatcher(
            StreamReader& streamReader,
            Print& printer,
            const DispatchRecord* dispatchTable,
            uint8_t numCommands,
            const char** argv,
            uint8_t argvSize):
        mStreamReader(streamReader),
        mPrinter(printer),
        mDispatchTable(dispatchTable),
        mNumCommands(numCommands),
        mArgv(argv),
        mArgvSize(argvSize) {}

    /**
     * Tokenize the line, and fill argv with each token until argvSize is
     * reached. Return the number of tokens. VisibleForTesting.
     */
    static uint8_t tokenize(char* line, const char** argv, uint8_t argvSize);

    /**
     * Find the dispatch record of the given command name. VisibleForTesting.
     *
     * NOTE: this is currently a linear O(N) scan which is good enough for
     * small number of commands. If we sorted the handlers, we could do a
     * binary search for O(log(N)) and handle larger number of commands.
     */
    static const DispatchRecord* findCommand(
        const DispatchRecord* dispatchTable,
        uint8_t numCommands, const char* cmd);

  private:
    static const uint8_t STATUS_SUCCESS = 0;
    static const uint8_t STATUS_BUFFER_OVERFLOW = 1;
    static const uint8_t STATUS_FLUSH_TO_EOL = 2;
    static const char DELIMS[];

    /** Print the error caused by the given line. */
    void printLineError(const char* line, uint8_t statusCode);

    /** Handle the 'help' command. */
    void helpCommandHandler(Print& printer, int argc, const char** argv);

    /** Tokenize the given line and run the command handler. */
    void runCommand(char* line);

    virtual int run() override;

    StreamReader& mStreamReader;
    Print& mPrinter;
    const DispatchRecord* const mDispatchTable;
    const uint8_t mNumCommands;
    const char** const mArgv;
    const uint8_t mArgvSize;
};

}
}

#endif
