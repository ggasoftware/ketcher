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

import com.sun.jersey.api.core.ResourceConfig;
import com.sun.jersey.core.util.Base64;
import com.sun.jersey.multipart.FormDataParam;
import com.sun.jersey.spi.resource.Singleton;

import javax.ws.rs.*;
import javax.ws.rs.core.Context;
import javax.ws.rs.core.MediaType;
import javax.ws.rs.core.Response;

import java.net.URI;

import org.apache.log4j.Logger;

@Singleton
@Path("/")
public class KetcherServerMapping {
    private static KetcherServer ketcherServer = null;

    private static final Logger _logger = Logger.getLogger(KetcherServerMapping.class);

    public KetcherServerMapping() {
        getKetcherServer();
    }

    synchronized public static KetcherServer getKetcherServer ()
    {
        if (ketcherServer == null)
            ketcherServer = new KetcherServer();
        return ketcherServer;
    }

    synchronized public static void unloadKetcherServer ()
    {
        ketcherServer = null;
    }

    @Path("/knocknock")
    @Produces(MediaType.TEXT_PLAIN)
    @Consumes(MediaType.TEXT_PLAIN)
    @GET
    public String knocknock() {
        _logger.info("Ketcher: knocknock");
        String[] fields = new String[0];
        String[] values = new String[0];
        IntByReference outputLen = new IntByReference();
        String[] contentParams = new String[0];
        return getKetcherServer().runCommand("knocknock", 0, fields, values, outputLen, contentParams);
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
        _logger.info("Ketcher: layout");
        String[] fields = new String[1];
        fields[0] = "smiles";
        String[] values = new String[1];
        values[0] = data;
        IntByReference outputLen = new IntByReference();
        String[] contentParams = new String[0];
        return getKetcherServer().runCommand("layout", 1, fields, values, outputLen, contentParams);
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
        _logger.info("Ketcher: automap");
        String[] fields = new String[2];
        fields[0] = "smiles";
        fields[1] = "mode";
        String[] values = new String[2];
        values[0] = data;
        values[1] = mode;
        IntByReference outputLen = new IntByReference();
        String[] contentParams = new String[0];
        return getKetcherServer().runCommand("automap", 2, fields, values, outputLen, contentParams);
    }

    @Path("/aromatize")
    @POST
    @Produces(MediaType.TEXT_PLAIN)
    @Consumes(MediaType.APPLICATION_FORM_URLENCODED)
    public String aromatizePost(@FormParam("moldata") String data) {
        _logger.info("Ketcher: aromatize");
        String[] fields = new String[1];
        fields[0] = "smiles";
        String[] values = new String[1];
        values[0] = data;
        IntByReference outputLen = new IntByReference();
        String[] contentParams = new String[0];
        return getKetcherServer().runCommand("aromatize", 1, fields, values, outputLen, contentParams);
    }

    @Path("/dearomatize")
    @POST
    @Produces(MediaType.TEXT_PLAIN)
    @Consumes(MediaType.APPLICATION_FORM_URLENCODED)
    public String dearomatizePost(@FormParam("moldata") String data) {
        _logger.info("Ketcher: dearomatize");
        String[] fields = new String[1];
        fields[0] = "smiles";
        String[] values = new String[1];
        values[0] = data;
        IntByReference outputLen = new IntByReference();
        String[] contentParams = new String[0];
        return getKetcherServer().runCommand("dearomatize", 1, fields, values, outputLen, contentParams);
    }


    @Path("/save")
    @POST
    @Consumes(MediaType.MULTIPART_FORM_DATA)
    public Response save(@FormDataParam("filedata") String fileData) {
        _logger.info("Ketcher: save");
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
        _logger.info("Ketcher: open");
        String[] fields = new String[1];
        fields[0] = "filedata";
        String[] values = new String[1];
        values[0] = fileData;
        IntByReference outputLen = new IntByReference();
        String[] contentParams = new String[0];
        return getKetcherServer().runCommand("open", 1, fields, values, outputLen, contentParams);
    }

    @Path("/getinchi")
    @POST
    @Produces(MediaType.TEXT_PLAIN)
    @Consumes(MediaType.APPLICATION_FORM_URLENCODED)
    public String getInchiPost(@FormParam("moldata") String data) {
        _logger.info("Ketcher: inchi");
        String[] fields = new String[1];
        fields[0] = "smiles";
        String[] values = new String[1];
        values[0] = data;
        IntByReference outputLen = new IntByReference();
        String[] contentParams = new String[0];
        return getKetcherServer().runCommand("getinchi", 1, fields, values, outputLen, contentParams);
    }

    @Path("/")
    @GET
    public Response redirect() {
        _logger.info("Ketcher: redirecting / to /ketcher.html");
        return Response.seeOther(URI.create("/ketcher.html")).build();
    }
}
