/*-----------------------------------------------------------------------

File  : e_deduction_server.c

Author: Stephan Schulz

Contents

Implementation for the deduction server executable which starts the server with the params given.
  Copyright 2015 by the author.
  This code is released under the GNU General Public Licence and
  the GNU Lesser General Public License.
  See the file COPYING in the main E directory for details..
  Run "eprover -h" for contact information.

Changes

-----------------------------------------------------------------------*/

#include <clb_defines.h>
#include <cio_commandline.h>
#include <cio_output.h>
#include <ccl_relevance.h>
#include <cio_signals.h>
#include <ccl_formulafunc.h>
#include <cco_batch_spec.h>
#include <ccl_sine.h>
#include <e_version.h>
#include <cco_einteractive_mode.h>


/*---------------------------------------------------------------------*/
/*                  Data types                                         */
/*---------------------------------------------------------------------*/

#define NAME         "e_deduction_server"

typedef enum
{
   OPT_NOOPT=0,
   OPT_HELP,
   OPT_VERSION,
   OPT_VERBOSE,
   OPT_PORT,
   OPT_PRINT_STATISTICS,
   OPT_SILENT,
   OPT_OUTPUTLEVEL,
   OPT_GLOBAL_WTCLIMIT,
   OPT_SERVER_LIB,
   OPT_DUMMY
}OptionCodes;



/*---------------------------------------------------------------------*/
/*                        Global Variables                             */
/*---------------------------------------------------------------------*/


OptCell opts[] =
{
   {OPT_HELP,
    'h', "help",
    NoArg, NULL,
    "Print a short description of program usage and options."},

   {OPT_VERSION,
    'V', "version",
    NoArg, NULL,
    "Print the version number of the prover. Please include this"
    " with all bug reports (if any)."},

   {OPT_VERBOSE,
    'v', "verbose",
    OptArg, "1",
    "Verbose comments on the progress of the program. This differs "
    "from the output level (below) in that technical information is "
    "printed to stderr, while the output level determines which "
    "logical manipulations of the clauses are printed to stdout."},

   {OPT_PORT,
    'p', "port",
    ReqArg, NULL,
    "The port on which the server will receive connections. Only effective "
    "when interactive mode is on. If not given stdin/stdout will be used."},

   {OPT_SILENT,
    's', "silent",
    NoArg, NULL,
    "Equivalent to --output-level=0."},

   {OPT_OUTPUTLEVEL,
    'l', "output-level",
    ReqArg, NULL,
    "Select an output level, greater values imply more verbose "
    "output. Level 0 produces "
    "nearly no output, level 1 will output each clause as it is "
    "processed, level 2 will output generating inferences, "
    "level 3 will give a full protocol including rewrite steps and "
    "level 4 will include some internal clause renamings. Levels >= 2"
    " also imply PCL2 or TSTP formats (which can be post-processed"
    " with suitable tools)."},

   {OPT_GLOBAL_WTCLIMIT,
    'w', "wtc-limit",
    ReqArg, NULL,
    "Set the global wall-clock limit for each batch (if any)."},

   {OPT_SERVER_LIB,
    'L', "lib",
    ReqArg, NULL,
    "Set the axioms library directory of the server."},

   {OPT_NOOPT,
    '\0', NULL,
    NoArg, NULL,
    NULL}
};

char              *outname        = NULL;
char              *server_lib     = NULL;
long              total_wtc_limit = 0;
int               port            = -1;

/*---------------------------------------------------------------------*/
/*                      Forward Declarations                           */
/*---------------------------------------------------------------------*/

CLState_p process_options(int argc, char* argv[]);
void print_help(FILE* out);

/*---------------------------------------------------------------------*/
/*                         Internal Functions                          */
/*---------------------------------------------------------------------*/

int main(int argc, char* argv[])
{
   CLState_p        state;
   BatchSpec_p      spec;
   StructFOFSpec_p   ctrl;
   char             *prover    = "eprover";
   int oldsock,sock_fd,pid;

   assert(argv[0]);


   // TODO Set a default problem time limit
   if( !total_wtc_limit )
     total_wtc_limit = 30;

   InitIO(NAME);
   DocOutputFormat = tstp_format;
   OutputFormat = TSTPFormat;

   state = process_options(argc, argv);

   OpenGlobalOut(outname);

   if(state->argc >= 1)
   {
      prover = state->argv[0];
   }

   spec = BatchSpecAlloc(prover, TSTPFormat);
   spec->category = SecureStrdup("dummy");
   spec->total_wtc_limit = total_wtc_limit;
   spec->res_proof = BODesired;

   ctrl = StructFOFSpecAlloc();
   BatchStructFOFSpecInit(spec, ctrl);

   //Creating Socket Server
   if(port != -1)
   {
     struct sockaddr cli_addr;
     socklen_t       cli_len;
     oldsock = CreateServerSock(port);
     Listen(oldsock);
     while(1)
     {
       sock_fd = accept(oldsock, &cli_addr, &cli_len);
       if ((pid = fork()) == -1)
       {
         close(sock_fd);
         continue;
       }
       else if(pid > 0)
       {
         close(sock_fd);
         fprintf(stdout, "Client connected ..\n");
         fflush(stdout);
         continue;
       }
       else if(pid == 0)
       {
         StartDeductionServer(spec, ctrl, server_lib, NULL, sock_fd);
         close(sock_fd);
         break;
       }
     }
   }else{
      StartDeductionServer(spec, ctrl, server_lib, stdout, -1);
   }

   StructFOFSpecFree(ctrl);
   BatchSpecFree(spec);

   CLStateFree(state);

   OutClose(GlobalOut);
   ExitIO();
#ifdef CLB_MEMORY_DEBUG
   MemFlushFreeList();
   MemDebugPrintStats(stdout);
#endif

   return 0;
}


/*-----------------------------------------------------------------------
//
// Function: process_options()
//
//   Read and process the command line option, return (the pointer to)
//   a CLState object containing the remaining arguments.
//
// Global Variables: opts, Verbose, TBPrintInternalInfo
//
// Side Effects    : Sets variables, may terminate with program
//                   description if option -h or --help was present
//
/----------------------------------------------------------------------*/

CLState_p process_options(int argc, char* argv[])
{
   Opt_p handle;
   CLState_p state;
   char*  arg;

   state = CLStateAlloc(argc,argv);

   while((handle = CLStateGetOpt(state, &arg, opts)))
   {
      switch(handle->option_code)
      {
      case OPT_VERBOSE:
       Verbose = CLStateGetIntArg(handle, arg);
       break;
      case OPT_HELP:
       print_help(stdout);
       exit(NO_ERROR);
       break;
      case OPT_VERSION:
       fprintf(stdout, NAME " " VERSION " " E_NICKNAME "\n");
       exit(NO_ERROR);
       break;
      case OPT_PORT:
            port = CLStateGetIntArg(handle, arg);
            break;
      case OPT_SILENT:
       OutputLevel = 0;
       break;
      case OPT_OUTPUTLEVEL:
       OutputLevel = CLStateGetIntArg(handle, arg);
       break;
      case OPT_GLOBAL_WTCLIMIT:
       total_wtc_limit = CLStateGetIntArg(handle, arg);
       break;
      case OPT_SERVER_LIB:
       server_lib = arg;
       break;
      default:
       assert(false && "Unknown option");
       break;
      }
   }
   return state;
}

void print_help(FILE* out)
{
   fprintf(out, "\n"
NAME " " VERSION " \"" E_NICKNAME "\"\n\
\n\
Usage: " NAME " [options] [files]\n\
\n\
Read a CASC 24 LTB batch specification file and process it.\n\
\n");
   PrintOptions(stdout, opts, "Options:\n\n");
   fprintf(out, "\n\
"STS_COPYRIGHT", " STS_MAIL "\n\
\n\
You can find the latest version of E and additional information at\n"
E_URL
"\n\n"
"This program is free software; you can redistribute it and/or modify\n\
it under the terms of the GNU General Public License as published by\n\
the Free Software Foundation; either version 2 of the License, or\n\
(at your option) any later version.\n\
\n\
This program is distributed in the hope that it will be useful,\n\
but WITHOUT ANY WARRANTY; without even the implied warranty of\n\
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n\
GNU General Public License for more details.\n\
\n\
You should have received a copy of the GNU General Public License\n\
along with this program (it should be contained in the top level\n\
directory of the distribution in the file COPYING); if not, write to\n\
the Free Software Foundation, Inc., 59 Temple Place, Suite 330,\n\
Boston, MA  02111-1307 USA\n"
"\n\
The original copyright holder can be contacted as\n\
\n"
STS_SNAIL
"\n");

}


/*---------------------------------------------------------------------*/
/*                        End of File                                  */
/*---------------------------------------------------------------------*/


