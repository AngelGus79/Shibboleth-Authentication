<?php

/**********************************************************************************************

PHP Page returning values to create the user session and to obtain a trusthworthy
authentication.

This page must returns a list of items like:
  key1=value1
  key2=value2
  key3=value3
  ...
containing all the relevant information about the user session.

The example provided below, lists from the $_SERVER object a bunch of attributes obtained
via Shibboleth after the user authentication and session creation.

***********************************************************************************************/

$headers = array('Shib-Application-ID', 'Shib-Session-ID', 'Shib-Identity-Provider', 'Shib-Authentication-Instant',
                 'Shib-Authentication-Method', 'Shib-AuthnContext-Class', 'Shib-Session-Index',
                 'eduPersonEntitlement', 'eduPersonPrincipalName', 'eduPersonScopedAffiliation',  'eduPersonTargetedID',
                 'email', 'givenName', 'mail', 'name', 'sn', 'surname', 'uid');

print "authenticated=true\n";

foreach($_SERVER as $key => $value) {
	if(in_array($key, $headers)) {
		print $key."=".$value."\n";
	}
}
?>