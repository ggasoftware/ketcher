#include <stdio.h>
#include <iostream>
#include <vector>
#include "base_cpp/array.h"
#include "ketcher_server.h"

#include <boost/thread/thread.hpp>
#include <boost/assign/list_of.hpp>

using namespace indigo;

struct KsOutput
{
   std::vector<char> data;
   std::string content_params;

   bool compare( KsOutput &another )
   {
      if (content_params.compare(another.content_params) != 0)
         return false;

      if (data.size() != another.data.size())
         return false;

      if (!std::equal(data.begin(), data.end(), another.data.begin()))
         return false;

      return true;
   }
};

void runCmd( const char *command_name, int fields_count, const char ** fields, const char ** values, KsOutput &out )
{
   out.data.clear();
   out.content_params.clear();

   int out_data_len;
   const char *out_data;
   const char *content_params;
   out_data = ketcherServerRunCommand(command_name, fields_count, fields, values, &out_data_len, &content_params);

   out.data.assign(out_data, out_data + out_data_len);
   out.content_params.append(content_params);

   return;
}

bool testKsOutput( int thread_count, const char *command_name, int fields_count,
                   const char ** fields, const char ** values )
{
   boost::thread_group ks_threads;
   std::vector<KsOutput> ks_outputs(thread_count);

   std::cout << command_name << " " << fields_count << " " << fields << " " << values << std::endl;

   for (int i = 0; i < thread_count; i++)
      ks_threads.create_thread(boost::bind(&runCmd, command_name, fields_count, fields, values, boost::ref(ks_outputs[i])));

   ks_threads.join_all();

   bool res = true;
   KsOutput &et = ks_outputs[0];
   for (int i = 0; i < thread_count; i++)
      if (!et.compare(ks_outputs[i]))
      {
         std::cout << "FAIL: Results in different threads doesn't match\n";
         res = false;
         break;
      }

   if (res)
   {
      std::cout << "Output=<<<";
      for(int i = 0; i < et.data.size(); i++)
         std::cout << et.data[i];
      std::cout << ">>>\n";
      std::cout << "ContentParams=<<<";
      std::cout << et.content_params.c_str() << ">>>\n";
   }

   std::cout << std::endl;

   return res;
}

typedef const char * str;

struct KsInput
{
   const char *command_name;
   int fields_count;
   str fields[10];
   str values[10];

   KsInput(const char *cmd_name) :
           command_name(cmd_name)
   {
      fields_count = 0;
   }

   KsInput(const char *cmd_name, const char * f1, const char * v1) :
           command_name(cmd_name)
   {
      fields_count = 1;
      fields[0] = f1;
      values[0] = v1;
   }

   KsInput(const char *cmd_name, const char * f1, const char * v1,
                                 const char * f2, const char * v2) :
           command_name(cmd_name)
   {
      fields_count = 2;
      fields[0] = f1;
      fields[1] = f2;
      values[0] = v1;
      values[1] = v2;

   }

   KsInput(const char *cmd_name, const char * f1, const char * v1,
                                 const char * f2, const char * v2,
                                 const char * f3, const char * v3) :
           command_name(cmd_name)
   {
      fields_count = 3;
      fields[0] = f1;
      fields[1] = f2;
      fields[2] = f3;
      values[0] = v1;
      values[1] = v2;
      values[2] = v3;

   }

   ~KsInput()
   {
   }
};

int main(int argc, char* argv[])
{
   const char *filedata = "mol\r\n"
                    "  Mrv0541 02051321222D                                                \n"
                    "                                                                      \n"
                    " 13 12  0  0  0  0            999 V2000                               \n"
                    "    3.3000    1.4289    0.0000 C   0  0  0  0  0  0  0  0  0  0  0  0 \n"
                    "    2.4750    1.4289    0.0000 N   0  0  0  0  0  0  0  0  0  0  0  0 \n"
                    "    2.0625    0.7145    0.0000 C   0  0  0  0  0  0  0  0  0  0  0  0 \n"
                    "    2.4750    0.0000    0.0000 O   0  0  0  0  0  0  0  0  0  0  0  0 \n"
                    "    1.2375    0.7145    0.0000 O   0  0  0  0  0  0  0  0  0  0  0  0 \n"
                    "    0.8250    0.0000    0.0000 N   0  0  0  0  0  0  0  0  0  0  0  0 \n"
                    "    0.0000    0.0000    0.0000 C   0  0  0  0  0  0  0  0  0  0  0  0 \n"
                    "   -0.4125   -0.7145    0.0000 C   0  0  0  0  0  0  0  0  0  0  0  0 \n"
                    "   -1.1270   -0.3020    0.0000 C   0  0  0  0  0  0  0  0  0  0  0  0 \n"
                    "    0.3020   -1.1270    0.0000 C   0  0  0  0  0  0  0  0  0  0  0  0 \n"
                    "   -0.8250   -1.4289    0.0000 S   0  0  0  0  0  0  0  0  0  0  0  0 \n"
                    "   -0.4125   -2.1434    0.0000 C   0  0  0  0  0  0  0  0  0  0  0  0 \n"
                    "   -1.6500   -1.4289    0.0000 O   0  0  0  0  0  0  0  0  0  0  0  0 \n"
                    "  1  2  1  0  0  0  0                                                 \n"
                    "  2  3  1  0  0  0  0                                                 \n"
                    "  3  4  2  0  0  0  0                                                 \n"
                    "  3  5  1  0  0  0  0                                                 \n"
                    "  6  5  1  4  0  0  0                                                 \n"
                    "  6  7  2  0  0  0  0                                                 \n"
                    "  7  8  1  0  0  0  0                                                 \n"
                    "  8  9  1  0  0  0  0                                                 \n"
                    "  8 10  1  0  0  0  0                                                 \n"
                    "  8 11  1  0  0  0  0                                                 \n"
                    " 11 12  1  0  0  0  0                                                 \n"
                    " 11 13  2  0  0  0  0                                                 \n"
                    "M  END                                                                \n";

   const char *null_arg = 0;

   const char *render_fields[2] = {"smiles", "format"};
   const char *render_values[2] = {"N1=C(O)C2NC=NC=2N(CCC)C1=O", "png"};
   std::vector<KsInput> input_args = boost::assign::list_of<KsInput>
      ("knocknock")
      ("render", "smiles", "CCC>>CCN", "format", "png")
      ("layout", "smiles", "CCC>>CCN")
      ("aromatize", "smiles", "N1=C(O)C2NC=NC=2N(CCC)C1=O")
      ("dearomatize", "smiles", "N1=C(O)c2[n]c[n]c2N(CCC)C1=O")
      ("open", "filedata", filedata)
      ("save", "filedata", filedata)
      ("getinchi", "smiles", "N1=C(O)c2[n]c[n]c2N(CCC)C1=O")
      ("automap", "smiles", "CCC>>CCN", "mode", "discard");

   int thr_count = 20;
   std::vector<KsInput>::iterator arg_it;
   for (arg_it = input_args.begin(); arg_it != input_args.end(); arg_it++)
      if (!testKsOutput(thr_count, arg_it->command_name, arg_it->fields_count, arg_it->fields, arg_it->values))
         return 1;

   std::cout << "success\n";


   return 0;
}
