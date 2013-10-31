/****************************************************************************
 * Copyright (C) 2011 GGA Software Services LLC
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

import com.sun.jna.Library;
import com.sun.jna.ptr.IntByReference;

public interface KetcherServerLib extends Library
{
    String ketcherServerRunCommand(String commandName, int fieldsCount, String[] fields, String[] values, IntByReference outputLen, String[] contentParams);
    int ketcherServerGetCommandCount();
    String ketcherServerGetCommandName(int id);
}
