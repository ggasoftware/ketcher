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
#include <boost/thread/tss.hpp>
#include <boost/assign/list_of.hpp>
#include <boost/archive/iterators/base64_from_binary.hpp>
#include <boost/archive/iterators/transform_width.hpp>
#include <boost/archive/iterators/ostream_iterator.hpp>
#include <sstream>
#include <iostream>

using namespace indigo;
using namespace std;

typedef pair<string , string> KSField;
typedef map<string , string> KSFieldMap;

struct ThreadLocalString
{
private:
   boost::thread_specific_ptr<string> _tsp_str;
public:
   string & get( void )
   {
      if (_tsp_str.get() == 0)
         _tsp_str.reset(new string());
      return *(_tsp_str.get());
   }

   ~ThreadLocalString( void )
   {
      if (_tsp_str.get() != 0)
      {
         string *str = _tsp_str.get();
         delete str;
      }
      _tsp_str.release();
   }
};

static const int def_bond_length = 100;
static ThreadLocalString ks_output;
static ThreadLocalString ks_content_params;

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

   if (strstr(mol_str, ">>") || (strstr(mol_str, "$RXN") == mol_str))
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
   ks_output.get().append("You are welcome!");
}

static void layout( KSFieldMap &fields )
{
   bool is_query, is_rxn;
   int obj = loadObject(fields, is_rxn, is_query);

   indigoLayout(obj);
   const char *res = objMolfile(obj, is_rxn);

   indigoFree(obj);
   ks_output.get().append("Ok.\n");
   ks_output.get().append(res);
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
   ks_output.get().append("Ok.\n");
   ks_output.get().append(res);
}

void convertToBase64( std::string &in, std::string &out )
{
    std::stringstream os;
    typedef boost::archive::iterators::base64_from_binary<    // convert binary values ot base64 characters
               boost::archive::iterators::transform_width<    // retrieve 6 bit integers from a sequence of 8 bit bytes
                  const char *, 6, 8 
               >
            > base64_text; // compose all the above operations in to a new iterator

    std::copy(
        base64_text(in.c_str()),
        base64_text(in.c_str() + in.size()),
        ostream_iterator<char>(os)
    );

    out.clear();
    out.assign(os.str());
    
    for (int i = 0; i < out.length() % 4; i++)
       out.append("=");
}

static void open( KSFieldMap &fields )
{
   ks_content_params.get().append("Content-Type\n"
                                  "text/html\n");

   if (fields.find("filedata") == fields.end())
      throw Exception("open: incorrect input field\n");

   string &ks_out = ks_output.get();
   ks_out.append("<html><body onload=\"parent.ui.loadMoleculeFromFile()\" title=\"");
   
   string b64str;
   string instr = string("Ok.\n");
   convertToBase64(instr, b64str);
   ks_out.append(b64str);

   instr = string(fields["filedata"]);
   convertToBase64(instr, b64str);
   ks_out.append(b64str);

   ks_out.append("\"></body></html>");
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

   ks_content_params.get().append("Content-Type\n");
   if (type.compare("smi") == 0)
      ks_content_params.get().append("chemical/x-daylight-smiles\n");
   else if (type.compare("mol") == 0)
   {
      if (data.find_first_of("$RXN") == 0)
      {
         type.assign("rxn");
         ks_content_params.get().append("chemical/x-mdl-rxnfile\n");
      }
      else
         ks_content_params.get().append("chemical/x-mdl-molfile\n");
   }
   else
      ks_content_params.get().append("\n");

   std::ostringstream s_stream;
   s_stream << "Content-Length\n" << data.length() << "\n";
   ks_content_params.get().append(s_stream.str());

   s_stream.str("");

   s_stream << "Content-Disposition" << "\n" << "attachment; filename=\"ketcher." << type << "\"\n";
   ks_content_params.get().append(s_stream.str());
   
   string &ks_out = ks_output.get();
   ks_out.append(data);
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
   ks_output.get().append("Ok.\n");
   ks_output.get().append(res);
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
   ks_output.get().append("Ok.\n");
   ks_output.get().append(res);
}

static void getInchi( KSFieldMap &fields )
{
   bool is_rxn = 0, is_query = 0;
   int obj = loadObject(fields, is_rxn, is_query);

   const char *res = indigoInchiGetInchi(obj);

   if (res == 0)
      throw Exception(indigoGetLastError());
   
   indigoFree(obj);
   ks_output.get().append("Ok.\n");
   ks_output.get().append(res);
}

static void getSmiles( KSFieldMap &fields )
{
   bool is_rxn = 0, is_query = 0;

   int obj = loadObject(fields, is_rxn, is_query);

   const char *res = indigoSmiles(obj);

   if (res == 0)
      throw Exception(indigoGetLastError());
   
   indigoFree(obj);
   ks_output.get().append(res);
}

static void render( KSFieldMap &fields )
{
   bool is_rxn = 0, is_query = 0;
   int obj = loadObject(fields, is_rxn, is_query);
   char num_buf[32];

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

   ks_content_params.get().append(s_stream.str());

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

   ks_output.get().assign(buf, buf_size);

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

static vector<KetcherServerCommand> ks_commands = boost::assign::list_of<KetcherServerCommand>
   ("knocknock", knocknock)
   ("layout", layout)
   ("automap", automap)
   ("open", open)
   ("save", save)
   ("aromatize", aromatize)
   ("dearomatize", dearomatize)
   ("getinchi", getInchi)
   ("getsmiles", getSmiles)
   ("render", render);

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
   ks_output.get().clear();
   ks_content_params.get().clear();

   try
   {
      ks_commands[cmd_idx].cmd_proc(fields);
   }
   catch (Exception &ex)
   {
      ks_output.get().append(ex.message());
   }

   *output_len = (int)ks_output.get().length();
   *content_params = ks_content_params.get().data();
   return ks_output.get().data();
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
