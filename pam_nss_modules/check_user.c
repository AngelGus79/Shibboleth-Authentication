/*
   You need to add the following (or equivalent) to the /etc/pam.conf file.
   # check authorization
   check_user   auth       required     /usr/lib/security/pam_http.so
   check_user   account    required     /usr/lib/security/pam_http.so
*/

#include <security/pam_appl.h>
#include <security/pam_misc.h>
#include <stdio.h>
#include <stdlib.h>

static struct pam_conv conv = {
	misc_conv,
	NULL
};

int main(int argc, char *argv[])
{
	pam_handle_t *pamh = NULL;
	int retval = 0;
	const char *user = "nobody";
	const void *authenticated_user = NULL;

	if (argc == 2) user = argv[1];
	if (argc == 3)
	{
		if (strcmp(argv[1], "-env") != 0)
		{
			fprintf(stderr, "Usage: check_user [-env] [username]\n");
			exit(1);
		}

		user = argv[2];
	}

	if (argc > 3) {
		fprintf(stderr, "Usage: check_user [-env] [username]\n");
		exit(1);
	}
	retval = pam_start("check_user", user, &conv, &pamh);

	if (retval == PAM_SUCCESS) retval = pam_authenticate(pamh, 0);
	if (retval == PAM_SUCCESS) retval = pam_acct_mgmt(pamh, 0);
	if (retval == PAM_SUCCESS) retval = pam_setcred(pamh, 0);
	if (retval == PAM_SUCCESS) retval = pam_get_item(pamh, PAM_USER, &authenticated_user);

	/* This is where we have been authorized or not. */

	if (retval == PAM_SUCCESS)
	{
		fprintf(stdout, "Authenticated (user: %s).\n", (char *)authenticated_user);

		const char *cur_var_unique = pam_getenv(pamh, "Shib_Session_Unique");
		const char *cur_var_id = pam_getenv(pamh, "Shib_Session_ID");

		fprintf(stdout, "\nExecute these two directives to have the proper envirnoment variables initialized in your session:\n");
		fprintf(stdout, "export Shib_Session_Unique=%s\n", cur_var_unique);
		fprintf(stdout, "export Shib_Session_ID=%s\n", cur_var_id);
	}
	else fprintf(stdout, "Not Authenticated: %s.\n", pam_strerror(pamh, retval));

	if (pam_end(pamh, pam_close_session(pamh, 0)) != PAM_SUCCESS)
	{
		pamh = NULL;
		fprintf(stderr, "check_user: failed to release authenticator\n");
		exit(1);
	}

	return (retval == PAM_SUCCESS ? 0 : 1);
}
