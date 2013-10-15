/****************************************************************************
 * Copyright (C) 2010-2011 GGA Software Services LLC
 *
 * This file is part of KetcherServer toolkit.
 *
 * This file may be distributed and/or modified under the terms of the
 * GNU General Public License version 3 as published by the Free Software
 * Foundation and appearing in the file LICENSE.GPL included in the
 * packaging of this file.
 *
 * This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
 * WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 ***************************************************************************/

package com.ggasoftware.ketcher;

import com.sun.jna.Native;
import com.sun.jna.ptr.IntByReference;

import java.io.*;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.security.NoSuchProviderException;

import com.sun.jersey.api.core.ResourceConfig;
import com.sun.jersey.core.util.Base64;
import com.sun.jersey.multipart.FormDataParam;
import com.sun.jersey.spi.resource.Singleton;

import javax.ws.rs.*;
import javax.ws.rs.core.Context;
import javax.ws.rs.core.MediaType;
import javax.ws.rs.core.Response;

@Singleton
@Path("/")
public class KetcherServer {
    private String _path;
    private static KetcherServerLib _lib;
    public static final int OS_WINDOWS = 1;
    public static final int OS_MACOS = 2;
    public static final int OS_LINUX = 3;
    public static final int OS_SOLARIS = 4;
    private static boolean _library_unloaded = false;
    private static int _os = 0;
    private static String _dllpath = "";

    public KetcherServer(String path) {
        _path = path;
        loadLibraries(path);
    }

    public KetcherServer() {
        this(null);
    }

    private static String getHashString(InputStream input) throws NoSuchProviderException, NoSuchAlgorithmException, IOException {
        String res = "";
        MessageDigest algorithm = MessageDigest.getInstance("MD5");
        algorithm.reset();
        ByteArrayOutputStream buffer = new ByteArrayOutputStream();

        int nRead;
        byte[] data = new byte[4096];

        while ((nRead = input.read(data, 0, data.length)) != -1) {
            buffer.write(data, 0, nRead);
        }
        buffer.flush();

        algorithm.update(buffer.toByteArray());
        byte[] hashArray = algorithm.digest();
        String tmp = "";
        for (int i = 0; i < hashArray.length; i++) {
            tmp = (Integer.toHexString(0xFF & hashArray[i]));
            if (tmp.length() == 1) {
                res += "0" + tmp;
            } else {
                res += tmp;
            }
        }
        return res;
    }

    public static String extractFromJar(Class cls, String path, String filename) {
        InputStream stream = cls.getResourceAsStream(path + "/" + filename);

        if (stream == null)
            return null;

        String tmpdir_path;
        final File tmpdir;
        final File dllfile;

        try {
            // Clone input stream to calculate its hash and copy to temporary folder
            ByteArrayOutputStream baos = new ByteArrayOutputStream();
            byte[] buffer = new byte[4096];
            int len;
            while ((len = stream.read(buffer)) > -1 ) {
                baos.write(buffer, 0, len);
            }
            baos.flush();
            InputStream is1 = new ByteArrayInputStream(baos.toByteArray());
            InputStream is2 = new ByteArrayInputStream(baos.toByteArray());
            baos.close();

            // Calculate md5 hash string to name temporary folder
            String streamHashString = getHashString(is1);
            is1.close();
            tmpdir_path = System.getProperty("java.io.tmpdir") + File.separator + "ketcher" + streamHashString;
            tmpdir = new File(tmpdir_path);
            if (!tmpdir.exists()) {
                if (!tmpdir.mkdir()) {
                    return null;
                }
            }

            // Copy library to temporary folder
            dllfile = new File(tmpdir.getAbsoluteFile() + File.separator + filename);
            if (!dllfile.exists()) {
                FileOutputStream outstream = new FileOutputStream(dllfile);
                byte buf[] = new byte[4096];

                while ((len = is2.read(buf)) > 0)
                    outstream.write(buf, 0, len);

                outstream.close();
                is2.close();
            }
        } catch (IOException e) {
            return null;
        } catch (NoSuchAlgorithmException e) {
            return null;
        } catch (NoSuchProviderException e) {
            return null;
        }

        String p;

        try {
            p = dllfile.getCanonicalPath();
        } catch (IOException e) {
            return null;
        }

        final String fullpath = p;

        return fullpath;
    }

    private static String getPathToBinary(String path, String filename) {
        if (path == null) {
            String res = extractFromJar(KetcherServer.class, "/" + _dllpath, filename);
            if (res != null)
                return res;
            path = "lib";
        }
        path = path + File.separator + _dllpath + File.separator + filename;
        try {
            return (new File(path)).getCanonicalPath();
        } catch (IOException e) {
            return path;
        }
    }

    private synchronized static void loadLibraries(String path) {
        if (_lib != null)
            return;

        if (_os == OS_LINUX || _os == OS_SOLARIS)
            _lib = (KetcherServerLib) Native.loadLibrary(getPathToBinary(path, "libketcher-server.so"), KetcherServerLib.class);
        else if (_os == OS_MACOS)
            _lib = (KetcherServerLib) Native.loadLibrary(getPathToBinary(path, "libketcher-server.dylib"), KetcherServerLib.class);
        else // _os == OS_WINDOWS
        {
            if ((new File(getPathToBinary(path, "msvcr100.dll"))).exists())
                System.load(getPathToBinary(path, "msvcr100.dll"));
            if ((new File(getPathToBinary(path, "msvcp100.dll"))).exists())
                System.load(getPathToBinary(path, "msvcp100.dll"));
            _lib = (KetcherServerLib) Native.loadLibrary(getPathToBinary(path, "ketcher-server.dll"), KetcherServerLib.class);
        }
    }

    static public String getPlatformDependentPath() {
        return _dllpath;
    }

    public static boolean libraryUnloaded() {
        return _library_unloaded;
    }

    public static KetcherServerLib getLibrary() {
        return _lib;
    }

    public static int getOs() {
        String namestr = System.getProperty("os.name");
        if (namestr.matches("^Windows.*"))
            return OS_WINDOWS;
        else if (namestr.matches("^Mac OS.*"))
            return OS_MACOS;
        else if (namestr.matches("^Linux.*"))
            return OS_LINUX;
        else if (namestr.matches("^SunOS.*"))
            return OS_SOLARIS;
        else
            throw new Error("Operating system not recognized");
    }

    private static String getDllPath() {
        String path = "";
        switch (_os) {
            case OS_WINDOWS:
                path += "Win";
                break;
            case OS_LINUX:
                path += "Linux";
                break;
            case OS_SOLARIS:
                path += "Sun";
                break;
            case OS_MACOS:
                path += "Mac";
                break;
            default:
                throw new Error("OS not set");
        }
        path += "/";

        if (_os == OS_MACOS) {
            String version = System.getProperty("os.version");
            int minorVersion = Integer.parseInt(version.split("\\.")[1]);
            Integer usingVersion = null;

            for (int i = minorVersion; i >= 5; i--) {
                if (KetcherServer.class.getResourceAsStream("/" + path + "10." + i + "/libketcher-server.dylib") != null) {
                    usingVersion = i;
                    break;
                }
            }
            if (usingVersion == null) {
                throw new Error("KetcherServer cannot find native libraries for Mac OS X 10." + minorVersion + " at path " + path);
            }
            path += "10." + usingVersion;
        } else if (_os == OS_SOLARIS) {
            String model = System.getProperty("sun.arch.data.model");

            if (model.equals("32"))
                path += "sparc32";
            else
                path += "sparc64";
        } else {
            String archstr = System.getProperty("os.arch");
            if (archstr.equals("x86") || archstr.equals("i386"))
                path += "x86";
            else if (archstr.equals("x86_64") || archstr.equals("amd64"))
                path += "x64";
            else
                throw new Error("architecture not recognized");
        }

        return path;
    }

    static {
        _os = getOs();
        _dllpath = getDllPath();
    }

    public String runCommand(String commandName, int fieldsCount, String[] fields, String[] values, IntByReference outputLen, String[] contentParams) {
        return _lib.ketcherServerRunCommand(commandName, fieldsCount, fields, values, outputLen, contentParams);
    }

    public int getCommandCount() {
        return _lib.ketcherServerGetCommandCount();
    }

    public String getCommandName(int id) {
        return _lib.ketcherServerGetCommandName(id);
    }

    @Path("/knocknock")
    @Produces(MediaType.TEXT_PLAIN)
    @Consumes(MediaType.TEXT_PLAIN)
    @GET
    public String knocknock() {
        String[] fields = new String[0];
        String[] values = new String[0];
        IntByReference outputLen = new IntByReference();
        String[] contentParams = new String[0];
        return runCommand("knocknock", 0, fields, values, outputLen, contentParams);
    }

    @Path("/layout")
    @GET
    @Produces(MediaType.TEXT_PLAIN)
    @Consumes(MediaType.TEXT_PLAIN)
    public String layoutGet(@QueryParam("smiles") String data) {
        return layout(data);
    }

    @Path("/layout")
    @POST
    @Produces(MediaType.TEXT_PLAIN)
    @Consumes(MediaType.APPLICATION_FORM_URLENCODED)
    public String layoutPost(@FormParam("moldata") String data) {
        return layout(data);
    }

    private String layout(String data) {
        String[] fields = new String[1];
        fields[0] = "smiles";
        String[] values = new String[1];
        values[0] = data;
        IntByReference outputLen = new IntByReference();
        String[] contentParams = new String[0];
        return runCommand("layout", 1, fields, values, outputLen, contentParams);
    }

    @Path("/automap")
    @GET
    @Produces(MediaType.TEXT_PLAIN)
    @Consumes(MediaType.TEXT_PLAIN)
    public String automapGet(@QueryParam("mode") @DefaultValue("discard") String mode, @QueryParam("smiles") String data) {
        return automap(mode, data);
    }

    @Path("/automap")
    @POST
    @Produces(MediaType.TEXT_PLAIN)
    @Consumes(MediaType.APPLICATION_FORM_URLENCODED)
    public String automapPost(@FormParam("mode") @DefaultValue("discard") String mode, @FormParam("moldata") String data) {
        return automap(mode, data);
    }

    private String automap(String mode, String data) {
        String[] fields = new String[2];
        fields[0] = "smiles";
        fields[1] = "mode";
        String[] values = new String[2];
        values[0] = data;
        values[1] = mode;
        IntByReference outputLen = new IntByReference();
        String[] contentParams = new String[0];
        return runCommand("automap", 2, fields, values, outputLen, contentParams);
    }

    @Path("/aromatize")
    @POST
    @Produces(MediaType.TEXT_PLAIN)
    @Consumes(MediaType.APPLICATION_FORM_URLENCODED)
    public String aromatizePost(@FormParam("moldata") String data) {
        String[] fields = new String[1];
        fields[0] = "smiles";
        String[] values = new String[1];
        values[0] = data;
        IntByReference outputLen = new IntByReference();
        String[] contentParams = new String[0];
        return runCommand("aromatize", 1, fields, values, outputLen, contentParams);
    }

    @Path("/dearomatize")
    @POST
    @Produces(MediaType.TEXT_PLAIN)
    @Consumes(MediaType.APPLICATION_FORM_URLENCODED)
    public String dearomatizePost(@FormParam("moldata") String data) {
        String[] fields = new String[1];
        fields[0] = "smiles";
        String[] values = new String[1];
        values[0] = data;
        IntByReference outputLen = new IntByReference();
        String[] contentParams = new String[0];
        return runCommand("dearomatize", 1, fields, values, outputLen, contentParams);
    }


    @Path("/save")
    @POST
    @Consumes(MediaType.MULTIPART_FORM_DATA)
    public Response save(@FormDataParam("filedata") String fileData) {
        StringBuilder builder = new StringBuilder();
        String[] lines = fileData.split("\n");
        String valueType = lines[0].trim();
        for (int i = 1; i < lines.length; i++) {
            String line = lines[i].replace("\r", "");
            builder.append(line);
            if (i < (lines.length - 1)) builder.append("\n");
        }
        String value = builder.toString();

        String mimeType = "text/plain";
        if ("smi".equals(valueType)) {
            mimeType = "chemical/x-daylight-smiles";
        } else if ("mol".equals(valueType)) {
            mimeType = "chemical/x-mdl-molfile";
            if (value.startsWith("$RXN")) {
                valueType = "rxn";
                mimeType = "chemical/x-mdl-rxnfile";
            }
        }

        return Response
        .ok()
        .entity(value)
        .type(mimeType)
        .header("Content-Length", value.length())
        .header("Content-Disposition", "attachment; filename=ketcher.".concat(valueType))
        .build();
    }

    @Path("/open")
    @POST
    @Produces(MediaType.TEXT_HTML)
    @Consumes(MediaType.MULTIPART_FORM_DATA)
    public String open(@FormDataParam("filedata") String fileData) {
        String[] fields = new String[1];
        fields[0] = "filedata";
        String[] values = new String[1];
        values[0] = fileData;
        IntByReference outputLen = new IntByReference();
        String[] contentParams = new String[0];
        return runCommand("open", 1, fields, values, outputLen, contentParams);
    }

    @Path("/getinchi")
    @POST
    @Produces(MediaType.TEXT_PLAIN)
    @Consumes(MediaType.APPLICATION_FORM_URLENCODED)
    public String getInchiPost(@FormParam("moldata") String data) {
        String[] fields = new String[1];
        fields[0] = "smiles";
        String[] values = new String[1];
        values[0] = data;
        IntByReference outputLen = new IntByReference();
        String[] contentParams = new String[0];
        return runCommand("getinchi", 1, fields, values, outputLen, contentParams);
    }
}
