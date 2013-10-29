package com.ggasoftware.ketcher;

import org.apache.log4j.Logger;

import javax.servlet.ServletContextEvent;
import javax.servlet.ServletContextListener;

public class ContextListener  implements ServletContextListener {
    private static final Logger _logger = Logger.getLogger(ContextListener.class);

    @Override
    public void contextDestroyed(ServletContextEvent arg0) {
        _logger.info("Ketcher ServletContextListener destroyed");
        KetcherServerMapping.unloadKetcherServer();
        System.gc();
        System.gc();
        _logger.info("Unloading Ketcher shared library");
    }

    @Override
    public void contextInitialized(ServletContextEvent arg0) {
        _logger.info("Ketcher ServletContextListener started");
    }
}
