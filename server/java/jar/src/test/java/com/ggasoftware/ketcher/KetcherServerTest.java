package com.ggasoftware.ketcher;

import com.sun.jna.ptr.IntByReference;
import org.junit.Test;
import org.junit.Assert;

public class KetcherServerTest {
	@Test
	public void testLoad() {
		KetcherServer ks = new KetcherServer();
		Assert.assertNotNull(ks);
	}

	@Test
	public void testGetCommandCount() {
		KetcherServer ks = new KetcherServer();
		Assert.assertTrue(ks.getCommandCount() == 10);
	}

	@Test
	public void testGetCommandName() {
		KetcherServer ks = new KetcherServer();
		Assert.assertTrue(ks.getCommandName(1).equals("layout"));
	}

	@Test
	public void testRunCommand() {
		KetcherServer ks = new KetcherServer();
		String[] fields = new String[0];
        String[] values = new String[0];
        IntByReference outputLen = new IntByReference();
        String[] contentParams = new String[0];
        Assert.assertTrue(ks.runCommand("knocknock", 0, fields, values, outputLen, contentParams).equals("You are welcome!"));
	}
}