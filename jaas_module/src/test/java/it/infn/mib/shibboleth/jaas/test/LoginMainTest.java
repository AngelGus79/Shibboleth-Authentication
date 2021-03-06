/*
 * @(#)LoginMain.java	1.00 06/06/12
 *
 * Copyright 2012-2012 Andrea Biancini. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or 
 * without modification, are permitted.
 * 
 * This software is provided "AS IS," without a warranty of any 
 * kind. ALL EXPRESS OR IMPLIED CONDITIONS, REPRESENTATIONS AND 
 * WARRANTIES, INCLUDING ANY IMPLIED WARRANTY OF MERCHANTABILITY, 
 * FITNESS FOR A PARTICULAR PURPOSE OR NON-INFRINGEMENT, ARE HEREBY 
 * EXCLUDED. SUN AND ITS LICENSORS SHALL NOT BE LIABLE FOR ANY 
 * DAMAGES OR LIABILITIES  SUFFERED BY LICENSEE AS A RESULT OF  OR 
 * RELATING TO USE, MODIFICATION OR DISTRIBUTION OF THE SOFTWARE OR 
 * ITS DERIVATIVES. IN NO EVENT WILL SUN OR ITS LICENSORS BE LIABLE 
 * FOR ANY LOST REVENUE, PROFIT OR DATA, OR FOR DIRECT, INDIRECT, 
 * SPECIAL, CONSEQUENTIAL, INCIDENTAL OR PUNITIVE DAMAGES, HOWEVER 
 * CAUSED AND REGARDLESS OF THE THEORY OF LIABILITY, ARISING OUT OF 
 * THE USE OF OR INABILITY TO USE SOFTWARE, EVEN IF SUN HAS BEEN 
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.
 * 
 * You acknowledge that Software is not designed, licensed or 
 * intended for use in the design, construction, operation or 
 * maintenance of any nuclear facility. 
 */

package it.infn.mib.shibboleth.jaas.test;

import it.infn.mib.shibboleth.jaas.SAMLLoginModule;
import it.infn.mib.shibboleth.jaas.SAMLConfigFile;
import it.infn.mib.shibboleth.jaas.SAMLPrincipal;
import it.infn.mib.shibboleth.jaas.recognizers.ShibbolethDS;
import it.infn.mib.shibboleth.jaas.recognizers.ShibbolethIDP;
import it.infn.mib.shibboleth.jaas.recognizers.SimpleSAMLIDP;
import it.infn.mib.shibboleth.jaas.test.ws.BackendBindingStub;
import it.infn.mib.shibboleth.jaas.test.ws.BackendServiceLocator;
import it.infn.mib.shibboleth.jaas.test.ws.CookieHandler;

import java.io.IOException;
import java.net.URL;
import java.security.Principal;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import javax.security.auth.login.AppConfigurationEntry;
import javax.security.auth.login.FailedLoginException;
import javax.security.auth.login.LoginContext;
import javax.security.auth.login.LoginException;
import javax.xml.rpc.ServiceException;

import org.apache.axis.EngineConfiguration;
import org.apache.axis.Handler;
import org.apache.axis.SimpleChain;
import org.apache.axis.SimpleTargetedChain;
import org.apache.axis.client.AxisClient;
import org.apache.axis.configuration.SimpleProvider;
import org.apache.axis.transport.http.HTTPSender;
import org.apache.axis.transport.http.HTTPTransport;
import org.junit.Ignore;
import org.junit.Test;

/**
 * Test class that executes a login with the Shibboleth auth module.
 * 
 * @author Andrea Biancini <andrea.biancini@gmail.com>
 * @author Simon Vocella <voxsim@gmail.com>
 * @version 1.0, 07/03/2014
 */
public class LoginMainTest {
	private static String loggedUser = null;
	private static Map<String, String> session = null;

	/**
	 * Method to create the cookie token config to pass cookies to the
	 * webservice
	 * 
	 * @param cookies
	 *            the cookies to be passed to the WS call
	 * @return the engine configuration to manage webservice calls
	 * @exception if
	 *                some method goes an error
	 */
	@Ignore
	private static EngineConfiguration createCookieTokenConfig(
			Map<String, String> cookies) throws LoginException {
		SimpleProvider clientConfig = new SimpleProvider();
		Handler cookieHandler = (Handler) new CookieHandler(cookies);
		SimpleChain reqHandler = new SimpleChain();
		SimpleChain respHandler = new SimpleChain();
		reqHandler.addHandler(cookieHandler);
		Handler pivot = (Handler) new HTTPSender();
		Handler transport = new SimpleTargetedChain(reqHandler, pivot,
				respHandler);
		clientConfig.deployTransport(HTTPTransport.DEFAULT_TRANSPORT_NAME,
				transport);
		return clientConfig;
	}

	/**
	 * Method to get the WS handler with cookie tokens.
	 * 
	 * @param wsEndpoint
	 *            the webservice endpoint URL
	 * @param cookies
	 *            the cookies to be passed to the WS call
	 * @return the stub to call the webservice
	 * @exception if
	 *                some method goes an error
	 */
	@Ignore
	private static BackendBindingStub getCookieTokenBinding(String wsEndpoint,
			Map<String, String> cookies) throws LoginException {
		BackendBindingStub binding = null;

		try {
			BackendServiceLocator loc = new BackendServiceLocator(wsEndpoint);
			EngineConfiguration clientConfig = createCookieTokenConfig(cookies);
			loc.setEngineConfiguration(clientConfig);
			loc.setEngine(new AxisClient(clientConfig));
			binding = (BackendBindingStub) loc.getBackendPort();
		} catch (ServiceException jre) {
			if (jre.getLinkedCause() != null) {
				jre.getLinkedCause().printStackTrace();
			}
			throw new LoginException("JAX-RPC ServiceException caught: " + jre);
		}

		binding.setMaintainSession(true);
		return binding;
	}
	
	@Ignore
	private static String join(List<String> list, String conjunction) {
	   StringBuilder sb = new StringBuilder();
	   boolean first = true;
	   for (String item : list) {
	      if (first) {
	         first = false;
	      }
	      else {
	         sb.append(conjunction);
	      }
	      sb.append(item);
	   }
	   return sb.toString();
	}

	/**
	 * Function to login a user.
	 * 
	 * @throws LoginException
	 *             When the login has an exception
	 * @throws IOException 
	 */
	@Test
	public void loginWithShibbolethIDP() throws LoginException, IOException {
		SAMLConfigFile configFile = new SAMLConfigFile();
		
		String moduleClass = SAMLLoginModule.class.getCanonicalName();
		String applicationName = "Shibboleth";
		HashMap<String, String> options = new HashMap<String, String>();
		//options.put("url", "https://sp-test1.mib.garr.it/secure/pam.php");
		options.put("url", "https://applications.colibrinet.it/pam.php");
		options.put("sslcheck", "false");
		options.put("sess_username", "");
		options.put("truststore", "");
		options.put("truststore_password", "");
		
		List<String> regognizers = new ArrayList<String>();
		regognizers.add(ShibbolethDS.class.getCanonicalName());
		regognizers.add(ShibbolethIDP.class.getCanonicalName());
		regognizers.add(SimpleSAMLIDP.class.getCanonicalName());
		
		options.put("recognizers", join(regognizers, ","));
		
		AppConfigurationEntry entry = new AppConfigurationEntry(
				moduleClass, 
				AppConfigurationEntry.LoginModuleControlFlag.REQUIRED, 
				options);
		
		configFile.appendEntry(applicationName, entry);
		
		LoginContext lc = new LoginContext("Shibboleth", null,
				new FixedTestCallbackHandler("simon", "ciaosimon", "https://idp-test1.mib.infn.it/idp/shibboleth"), configFile);
		
		lc.login();

		// Prints the session values for the logged in user
		for (Principal curPrincipal : lc.getSubject().getPrincipals()) {
			if (curPrincipal instanceof SAMLPrincipal) {
				loggedUser = ((SAMLPrincipal) curPrincipal).getName();
				session = ((SAMLPrincipal) curPrincipal).getSession();
			}
		}

		System.out.println("User logged in successfully.");
		System.out.println("Username for logged user is: " + loggedUser);

		System.out.println("Printing session for logged in user: ");
		for (String curKey : session.keySet())
			System.out.println("[" + curKey + "] => " + session.get(curKey));
	}

	/**
	 * Function to login a user.
	 * 
	 * @throws LoginException
	 *             When the login has an exception
	 */
	@Test(expected = FailedLoginException.class)
	public void wrongLoginWithShibbolethIDP() throws LoginException {
		final URL jaasFile = getClass().getResource("/shibboleth_jaas.config");
		System.setProperty(
				"java.security.auth.login.config",
				jaasFile.getPath());
		LoginContext lc = new LoginContext("Shibboleth",
				new FixedTestCallbackHandler("simon", "ciaosimon2", "https://idp-test1.mib.infn.it/idp/shibboleth"));
		lc.login();

		// Prints the session values for the logged in user
		for (Principal curPrincipal : lc.getSubject().getPrincipals()) {
			if (curPrincipal instanceof SAMLPrincipal) {
				loggedUser = ((SAMLPrincipal) curPrincipal).getName();
				session = ((SAMLPrincipal) curPrincipal).getSession();
			}
		}

		System.out.println("Login User did not log in.");
	}
	
	/**
	 * Function to login a user.
	 * 
	 * @throws LoginException
	 *             When the login has an exception
	 * @throws IOException 
	 */
	@Test(expected = LoginException.class)
	public void wrongEntityidWithShibbolethIDP() throws LoginException {
		SAMLConfigFile configFile;
		try {
			configFile = new SAMLConfigFile();
			
			String moduleClass = SAMLLoginModule.class.getCanonicalName();
			String applicationName = "Shibboleth";
			HashMap<String, String> options = new HashMap<String, String>();
			//options.put("url", "https://sp-test1.mib.garr.it/secure/pam.php");
			options.put("url", "https://applications.colibrinet.it/pam.php");
			options.put("sslcheck", "false");
			options.put("sess_username", "eppn");
            options.put("truststore", "");
            options.put("truststore_password", "");
            options.put("get_from_shared_state", "false");
            options.put("share_state", "true");
			
			List<String> regognizers = new ArrayList<String>();
			regognizers.add(ShibbolethDS.class.getCanonicalName());
			regognizers.add(ShibbolethIDP.class.getCanonicalName());
			regognizers.add(SimpleSAMLIDP.class.getCanonicalName());
			
			options.put("recognizers", join(regognizers, ","));
			
			AppConfigurationEntry entry = new AppConfigurationEntry(
					moduleClass, 
					AppConfigurationEntry.LoginModuleControlFlag.REQUIRED, 
					options);
			
			configFile.appendEntry(applicationName, entry);
			
			LoginContext lc = new LoginContext("Shibboleth", null,
					new FixedTestCallbackHandler("simon", "ciaosimon", "colibri"),
					configFile);
			lc.login();

			// Prints the session values for the logged in user
			for (Principal curPrincipal : lc.getSubject().getPrincipals()) {
				if (curPrincipal instanceof SAMLPrincipal) {
					loggedUser = ((SAMLPrincipal) curPrincipal).getName();
					session = ((SAMLPrincipal) curPrincipal).getSession();
				}
			}

			System.out.println("Login User did not log in.");
		} catch (Exception e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
	}
	
	/**
	 * Function to login a user.
	 * 
	 * @param args
	 *            No arguments to be passed
	 * @throws LoginException
	 *             When the login has an exception
	 */
	@Test
	public void loginWithSimpleSAMLIDP() throws LoginException {
		final URL jaasFile = getClass().getResource("/shibboleth_jaas.config");
		System.setProperty(
				"java.security.auth.login.config",
				jaasFile.getPath());
		LoginContext lc = new LoginContext("Shibboleth",
				new FixedTestCallbackHandler("simon", "ciaosimon", "https://idp-test1.mib.infn.it/idp/shibboleth"));
		lc.login();

		// Prints the session values for the logged in user
		for (Principal curPrincipal : lc.getSubject().getPrincipals()) {
			if (curPrincipal instanceof SAMLPrincipal) {
				loggedUser = ((SAMLPrincipal) curPrincipal).getName();
				session = ((SAMLPrincipal) curPrincipal).getSession();
			}
		}

		System.out.println("User logged in successfully.");
		System.out.println("Username for logged user is: " + loggedUser);

		System.out.println("Printing session for logged in user: ");
		for (String curKey : session.keySet())
			System.out.println("[" + curKey + "] => " + session.get(curKey));
	}

	/**
	 * Function to login a user.
	 * 
	 * @param args
	 *            No arguments to be passed
	 * @throws LoginException
	 *             When the login has an exception
	 */
	@Test(expected = FailedLoginException.class)
	public void wrongLoginWithSimpleSAMLIDP() throws LoginException {
		final URL jaasFile = getClass().getResource("/shibboleth_jaas.config");
		System.setProperty(
				"java.security.auth.login.config",
				jaasFile.getPath());
		LoginContext lc = new LoginContext("Shibboleth",
				new FixedTestCallbackHandler("simon", "ciaosimon2", "https://idp-test1.mib.infn.it/idp/shibboleth"));
		lc.login();

		// Prints the session values for the logged in user
		for (Principal curPrincipal : lc.getSubject().getPrincipals()) {
			if (curPrincipal instanceof SAMLPrincipal) {
				loggedUser = ((SAMLPrincipal) curPrincipal).getName();
				session = ((SAMLPrincipal) curPrincipal).getSession();
			}
		}

		System.out.println("Login User did not log in.");
	}

	/**
	 * Function to login a user.
	 * 
	 * @param args
	 *            No arguments to be passed
	 * @return true if the user session was found in environment, false
	 *         otherwise
	 * 
	 */
	@Test
	public void sso() {
		String sessUsername = "eduPersonPrincipalName";

		// The environment variables should not contain the dash character
		// so the login module may translate the dash with an underline symbol.
		// Here do the opposite translation (replaceAll instructions).
		loggedUser = System.getProperty(sessUsername.replaceAll("-", "_"));
		String shibUnique = System.getProperty("Shib-Session-Unique"
				.replaceAll("-", "_"));
		String shibId = System.getProperty("Shib-Session-ID".replaceAll("-",
				"_"));

		if (loggedUser == null || shibUnique == null || shibId == null) {
			System.err
					.println("Error retrieving user session from environment.");
			assert (false);
		}

		session = new HashMap<String, String>();
		session.put("Shib-Session-Unique", shibUnique);
		session.put("Shib-Session-ID", shibId);

		System.out.println("User logged in successfully.");
		System.out.println("Username for logged user is: " + loggedUser);

		System.out.println("Printing session for logged in user: ");
		for (String curKey : session.keySet()) {
			System.out.println("[" + curKey + "] => " + session.get(curKey));
		}
	}
}
