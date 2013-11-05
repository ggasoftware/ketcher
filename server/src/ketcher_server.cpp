#include "base_c/defs.h"

#include <stdio.h>
#include <stdarg.h>
#include "indigo.h"
#include "indigo-inchi.h"
#include "indigo-renderer.h"
#include "base_cpp/tlscont.h"
#include <vector>
#include <map>
#include <string>
#include <climits>
#include <sstream>
#include <iostream>
#include <stdint.h>
#include <stdlib.h>

using namespace indigo;
using namespace std;

typedef pair<string , string> KSField;
typedef map<string , string> KSFieldMap;

#ifdef _WIN32
   #define KETCHER_SERVER_TL __declspec(thread)
#elif (defined __GNUC__ || defined __APPLE__)
   #define KETCHER_SERVER_TL __thread
#endif

static const int def_bond_length = 100;
static KETCHER_SERVER_TL char ks_output[10000] = {0};
static KETCHER_SERVER_TL size_t ks_output_length = 0;
static KETCHER_SERVER_TL char ks_content_params[10000] = {0};

static const char encoding_table[] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
                                      'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
                                      'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
                                      'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
                                      'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
                                      'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
                                      'w', 'x', 'y', 'z', '0', '1', '2', '3',
                                      '4', '5', '6', '7', '8', '9', '+', '/'};

void convertToBase64(std::string &data, std::string &encoded_data)
{
   size_t input_length = data.size();
   size_t output_length;
   int mod_table[] = {0, 2, 1};

   output_length = 4 * ((input_length + 2) / 3);

   encoded_data.clear();
   encoded_data.resize(output_length);
   
   int i = 0;
   int j = 0;
   while (i < input_length)
   {
      uint32_t octet_a = i < input_length ? data[i++] : 0;
      uint32_t octet_b = i < input_length ? data[i++] : 0;
      uint32_t octet_c = i < input_length ? data[i++] : 0;

      uint32_t triple = (octet_a << 0x10) + (octet_b << 0x08) + octet_c;

      encoded_data[j++] = encoding_table[(triple >> 3 * 6) & 0x3F];
      encoded_data[j++] = encoding_table[(triple >> 2 * 6) & 0x3F];
      encoded_data[j++] = encoding_table[(triple >> 1 * 6) & 0x3F];
      encoded_data[j++] = encoding_table[(triple >> 0 * 6) & 0x3F];
   }

   for (int i = 0; i < mod_table[input_length % 3]; i++)
      encoded_data[output_length - 1 - i] = '=';
}

static int loadObject( KSFieldMap &fields, bool &is_rxn, bool &is_query )
{
   int obj;
   is_rxn = 0;
   is_query = 0;

   const char *mol_str = 0;
   KSFieldMap::iterator a = fields.find("smiles"), b = fields.end();
   if (fields.find("smiles") != fields.end())
      mol_str = fields["smiles"].c_str();
   else if (fields.find("moldata") != fields.end())
      mol_str = fields["moldata"].c_str();
   else
      throw Exception("Loading object: incorrect input field");

   if (strncmp(mol_str, "InChI=", 6) == 0)
   {
      obj = indigoInchiLoadMolecule(mol_str);
   }
   else if (strstr(mol_str, ">>") || (strstr(mol_str, "$RXN") == mol_str))
   {
      is_rxn = 1;
      obj = indigoLoadReactionFromString(mol_str);
      if (obj == -1)
      {
         obj = indigoLoadQueryReactionFromString(mol_str);
         is_query = 1;
      }
   }
   else
   {
      obj = indigoLoadMoleculeFromString(mol_str);
      if (obj == -1)
      {
         obj = indigoLoadQueryMoleculeFromString(mol_str);
         is_query = 1;
      }
   }

   if (obj == -1)
      throw Exception(indigoGetLastError());

   return obj;
}

static const char * objMolfile ( int obj, bool is_rxn )
{
   const char *res = 0;
   if (is_rxn)
      res = indigoRxnfile(obj);
   else
      res = indigoMolfile(obj);

   if (res == 0)
      throw Exception(indigoGetLastError());

   return res;
}

static void knocknock( KSFieldMap &fields )
{
   strcat(ks_output, "You are welcome!");
   ks_output_length = strlen(ks_output);
}

static void layout( KSFieldMap &fields )
{
   bool is_query, is_rxn;
   int obj = loadObject(fields, is_rxn, is_query);

   indigoLayout(obj);
   const char *res = objMolfile(obj, is_rxn);

   indigoFree(obj);
   strcat(ks_output, "Ok.\n");
   strcat(ks_output, res);
   ks_output_length = strlen(ks_output);
}

static void automap( KSFieldMap &fields )
{
   bool is_query, is_rxn;
   int rxn = loadObject(fields, is_rxn, is_query);

   if (fields.find("mode") == fields.end())
      throw Exception("automap: incorrect input field\n");

   int ret = indigoAutomap(rxn, fields["mode"].c_str());

   if (ret == -1)
      throw Exception(indigoGetLastError());

   const char *res = objMolfile(rxn, is_rxn);

   indigoFree(rxn);
   strcat(ks_output, "Ok.\n");
   strcat(ks_output, res);
   ks_output_length = strlen(ks_output);
}

static void open( KSFieldMap &fields )
{
   strcat(ks_content_params, "Content-Type\n"
                                  "text/html\n");

   if (fields.find("filedata") == fields.end())
      throw Exception("open: incorrect input field\n");

   strcat(ks_output, "<html><body onload=\"parent.ui.loadMoleculeFromFile()\" title=\"");

   string b64str;
   string instr = string("Ok.\n");
   convertToBase64(instr, b64str);
   strcat(ks_output, b64str.c_str());

   instr = string(fields["filedata"]);
   convertToBase64(instr, b64str);
   strcat(ks_output, b64str.c_str());

   strcat(ks_output, "\"></body></html>");
   ks_output_length = strlen(ks_output);
}

static void save( KSFieldMap &fields )
{
   if (fields.find("filedata") == fields.end())
      throw Exception("save: incorrect input field\n");

   string &filedata = fields["filedata"];

   int ln_pos = (int)filedata.find_first_of('\n');

   if (ln_pos < 0)
      throw Exception("save: incorrect filedata value\n");

   string type(filedata.begin(), filedata.begin() + ln_pos - 1);
   string data(filedata.begin() + ln_pos + 1, filedata.end());

   strcat(ks_content_params, "Content-Type\n");
   if (type.compare("smi") == 0)
      strcat(ks_content_params, "chemical/x-daylight-smiles\n");
   else if (type.compare("mol") == 0)
   {
      if (data.find_first_of("$RXN") == 0)
      {
         type.assign("rxn");
         strcat(ks_content_params, "chemical/x-mdl-rxnfile\n");
      }
      else
         strcat(ks_content_params, "chemical/x-mdl-molfile\n");
   }
   else
      strcat(ks_content_params, "\n");

   std::ostringstream s_stream;
   s_stream << "Content-Length\n" << data.length() << "\n";
   strcat(ks_content_params, s_stream.str().c_str());

   s_stream.str("");

   s_stream << "Content-Disposition" << "\n" << "attachment; filename=\"ketcher." << type << "\"\n";
   strcat(ks_content_params, s_stream.str().c_str());

   strcat(ks_output, data.c_str());
   ks_output_length = strlen(ks_output);
}

static void aromatize( KSFieldMap &fields )
{
   bool is_rxn = 0, is_query = 0;
   int obj = loadObject(fields, is_rxn, is_query);

   int ret = indigoAromatize(obj);

   if (ret == -1)
      throw Exception(indigoGetLastError());

   const char *res = objMolfile(obj, is_rxn);

   indigoFree(obj);
   strcat(ks_output, "Ok.\n");
   strcat(ks_output, res);
   ks_output_length = strlen(ks_output);
}

static void dearomatize( KSFieldMap &fields )
{
   bool is_rxn = 0, is_query = 0;
   int obj = loadObject(fields, is_rxn, is_query);

   int ret = indigoDearomatize(obj);

   if (ret == -1)
      throw Exception(indigoGetLastError());

   const char *res = objMolfile(obj, is_rxn);

   indigoFree(obj);
   strcat(ks_output, "Ok.\n");
   strcat(ks_output, res);
   ks_output_length = strlen(ks_output);
}

static void getInchi( KSFieldMap &fields )
{
   bool is_rxn = 0, is_query = 0;
   int obj = loadObject(fields, is_rxn, is_query);

   const char *res = indigoInchiGetInchi(obj);

   if (res == 0)
      throw Exception(indigoGetLastError());

   indigoFree(obj);
   strcat(ks_output, "Ok.\n");
   strcat(ks_output, res);
   ks_output_length = strlen(ks_output);
}

static void getSmiles( KSFieldMap &fields )
{
   bool is_rxn = 0, is_query = 0;

   int obj = loadObject(fields, is_rxn, is_query);

   const char *res = indigoSmiles(obj);

   if (res == 0)
      throw Exception(indigoGetLastError());

   indigoFree(obj);
   strcat(ks_output, res);
   ks_output_length = strlen(ks_output);
}

static void render( KSFieldMap &fields )
{
   bool is_rxn = 0, is_query = 0;
   int obj = loadObject(fields, is_rxn, is_query);

   std::string format("png");
   if (fields.find("format") != fields.end())
      format.assign(fields["format"]);

   std::ostringstream s_stream;

   s_stream << "Content-Type\n" << "image/";
   if (format.compare("png") == 0)
      s_stream << "png\n";
   else if (format.compare("svg") == 0)
      s_stream << "svg+xml\n";
   else
      throw Exception("render: only png and svg formats are supported now\n");

   strcat(ks_content_params, s_stream.str().c_str());

   indigoSetOption("render-output-format", format.c_str());
   indigoSetOption("render-background-color", "255, 255, 255");
   std::stringstream bondlenss;
   bondlenss << def_bond_length;
   indigoSetOption("render-bond-length", bondlenss.str().c_str());

   KSFieldMap::iterator size_it = fields.find("size");
   KSFieldMap::iterator perc_it = fields.find("coef");

   if ((size_it != fields.end()) && (perc_it != fields.end()))
      throw Exception("render: only one of 'size' and 'coef' fields is allowed\n");

   if (size_it != fields.end())
      indigoSetOption("render-image-size", size_it->second.c_str());

   if (perc_it != fields.end())
   {
      unsigned long perc = strtoul(perc_it->second.c_str(), NULL, 10);
      if (perc <= 0 || perc == ULONG_MAX)
         throw Exception("render: incorrect percentage field\n");

      float coef = perc / 100.0f;
      std::stringstream bondlensscoeff;
      bondlensscoeff << def_bond_length * coef;
      indigoSetOption("render-bond-length", bondlensscoeff.str().c_str());
   }

   int indigo_buffer = indigoWriteBuffer();

   int ret = indigoRender(obj, indigo_buffer);

   if (ret == -1)
      throw Exception(indigoGetLastError());

   char *buf;
   int buf_size;
   indigoToBuffer(indigo_buffer, &buf, &buf_size);

   memcpy(ks_output, buf, buf_size);
   ks_output_length = buf_size;

   indigoRenderReset();

   indigoClose(indigo_buffer);
   indigoFree(indigo_buffer);
   indigoFree(obj);
}

struct KetcherServerCommand
{
   const char *name;
   void (*cmd_proc)(KSFieldMap &fields);

   KetcherServerCommand( const char *new_name, void (*new_cmd_proc)(KSFieldMap &fields) ) :
                         name(new_name), cmd_proc(new_cmd_proc)
   {
   }
};

static const KetcherServerCommand ks_commands_buf[] = {
   KetcherServerCommand("knocknock", knocknock),
   KetcherServerCommand("layout", layout),
   KetcherServerCommand("automap", automap),
   KetcherServerCommand("open", open),
   KetcherServerCommand("save", save),
   KetcherServerCommand("aromatize", aromatize),
   KetcherServerCommand("dearomatize", dearomatize),
   KetcherServerCommand("getinchi", getInchi),
   KetcherServerCommand("getsmiles", getSmiles),
   KetcherServerCommand("render", render)};

static vector<KetcherServerCommand> ks_commands(ks_commands_buf, 
   ks_commands_buf + sizeof(ks_commands_buf) / sizeof(ks_commands_buf[0]));

static int getCommandId( const char * command_name )
{
   int cmd_idx;
   for (cmd_idx = 0; cmd_idx < ks_commands.size(); cmd_idx++)
   {
      if (strcmp(command_name, ks_commands[cmd_idx].name) == 0)
         break;
   }

   if (cmd_idx == ks_commands.size())
      cmd_idx = -1;

   return cmd_idx;
}

static const char * runCommand( int cmd_idx, KSFieldMap &fields, int *output_len, const char **content_params )
{
   // Set Indigo options
   indigoSetOption("ignore-stereochemistry-errors", "true");

   ks_output[0] = 0;
   ks_content_params[0] = 0;

   try
   {
      ks_commands[cmd_idx].cmd_proc(fields);
   }
   catch (Exception &ex)
   {
      strcat(ks_output, ex.message());
   }

   *output_len = (int)ks_output_length;
   *content_params = ks_content_params;
   return ks_output;
}

CEXPORT const char * ketcherServerRunCommand( const char *command_name, int fields_count,
                                              const char **fields, const char **values,
                                              int *output_len,
                                              const char **content_params )
{
   KSFieldMap fields_map;
   fields_map.clear();

   for (int i = 0; i < fields_count; i++)
      if ((fields[i] != 0) && (values[i] != 0))
         fields_map.insert(KSField(fields[i], values[i]));

   int id = getCommandId(command_name);

   return runCommand(id, fields_map, output_len, content_params);
}

CEXPORT int ketcherServerGetCommandCount ()
{
   return (int)ks_commands.size();
}

CEXPORT const char * ketcherServerGetCommandName( int id )
{
   if ((id < 0) || (id >= ks_commands.size()))
      return 0;

   return ks_commands[id].name;
}
