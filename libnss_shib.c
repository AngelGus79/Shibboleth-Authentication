#define _GNU_SOURCE
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <syslog.h>
#include <libconfig.h>
#include <sys/types.h>
#include <sys/param.h>

#include <curl/curl.h>
#include <curl/types.h>
#include <curl/easy.h>

#include <pwd.h>
#include <grp.h>
#include <nss.h>
#include "netcurl.h"

BODY *body = NULL;
BODY *body_pwd = NULL;
BODY *body_grp = NULL;
static int last_rownum_pwd = -1;
static int last_rownum_grp = -1;

static const char *config_file = "/etc/libnss.conf";

const char *url = NULL;
const char *cafile = NULL;
const char *sslcheck = NULL;
const char *username = NULL;
const char *password = NULL;

static void readconfig()
{
  config_t cfg;
  const char *val = NULL;

  config_init(&cfg);
  if (!config_read_file(&cfg, config_file))  
  {
    char *message = "Config file not found in /etc/libnss.conf.\n";
    syslog(LOG_ERR, message);

    #ifdef DEBUG
    fprintf(stderr, message);
    fprintf(stderr, "%s\n", config_error_text(&cfg));
    #endif

    config_destroy(&cfg);  
    return;
  }

  config_lookup_string(&cfg, "url", &val);
  url = (const char *)malloc((1+strlen(val))*sizeof(char));
  strcpy((char *)url, val);
  #ifdef DEBUG
  fprintf(stderr, "Read property [url] => %s\n", url);
  #endif
  config_lookup_string(&cfg, "cafile", &val);
  cafile = (const char *)malloc((1+strlen(val))*sizeof(char));
  strcpy((char *)cafile, val);
  #ifdef DEBUG
  fprintf(stderr, "Read property [cafile] => %s\n", cafile);
  #endif
  config_lookup_string(&cfg, "sslcheck", &val);
  sslcheck = (const char *)malloc((1+strlen(val))*sizeof(char));
  strcpy((char *)sslcheck, val);
  #ifdef DEBUG
  fprintf(stderr, "Read property [sslcheck] => %s\n", sslcheck);
  #endif
  config_lookup_string(&cfg, "username", &val);
  username = (const char *)malloc((1+strlen(val))*sizeof(char));
  strcpy((char *)username, val);
  #ifdef DEBUG
  fprintf(stderr, "Read property [username] => %s\n", username);
  #endif
  config_lookup_string(&cfg, "password", &val);
  password = (const char *)malloc((1+strlen(val))*sizeof(char));
  strcpy((char *)password, val);
  #ifdef DEBUG
  fprintf(stderr, "Read property [password] => %s\n", password);
  #endif

  config_destroy(&cfg); 
}

size_t bodycallback(char *ptr, size_t size, size_t nmemb, void *userdata)
{
  char* pstr = (char *)malloc(size*nmemb+1);
  strncpy(pstr, ptr, size*nmemb);
  pstr[size*nmemb] = '\0';

  char **rows = split_str(pstr, "\n");
  int i = 0;
  for (i = 0; rows[i]; i++)
  {
    #ifdef DEBUG
    fprintf(stderr, "Read body line: %s\n", rows[i]);
    #endif

    BODY *curbody = (BODY *) malloc(sizeof(BODY));
    curbody->row = rows[i];
    curbody->next = body;
    body = curbody;
  }

  return nmemb*size;
}

char *getelementfrompasswd(int filter, const char *value, int column)
{
  readconfig();
fprintf(stderr, "---------> %s\n", url);
  char *newurl = (char *) malloc(sizeof(char)*(strlen(url)+8));
  sprintf(newurl, "%s?passwd", url);
  if (!geturl(newurl, username, password, cafile, sslcheck)) return NULL;
  body_pwd = body; body = NULL;
  if (body_pwd == NULL) return NULL;

  BODY *cursor = body_pwd;
  while (cursor)
  {
    int i = 0;
    char **array = split_str(cursor->row, ":");
    if (array != NULL)
    {
      int print = 1;
      for (i = 0; array[i]; i++)
      {
        if (i == filter && strcmp(array[i], value) == 0) print = 0;
      }

      if (print == 0)
      {
        for (i = 0; array[i]; i++)
        {
          if (i == column) return array[i];
        }
      }
    }

    cursor = cursor->next;
  }

  return NULL;
}

enum nss_status _nss_shib_getpwnam_r(const char *name, struct passwd *result, char *buffer, size_t buflen, int *errnop)
{
  #ifdef DEBUG
  fprintf(stderr, "Entering _nss_shib_getpwnam_r with name=%s.\n", name);
  #endif

  readconfig();
  char *newurl = (char *) malloc(sizeof(char)*(strlen(url)+8));
  sprintf(newurl, "%s?passwd", url);
  if (!geturl(newurl, username, password, cafile, sslcheck)) return NSS_STATUS_UNAVAIL;
  body_pwd = body; body = NULL;
  if (body_pwd == NULL) return NSS_STATUS_UNAVAIL;

  BODY *cursor = body_pwd;
  while (cursor)
  {
    char **array = split_str(cursor->row, ":");
    if (array != NULL)
    {
      if (strcmp(array[0], name) == 0)
      {
        result->pw_name = array[0];
        result->pw_passwd = array[1];
        result->pw_uid = atoi(array[2]);
        result->pw_gid = atoi(array[3]);
        result->pw_gecos = array[4];
        result->pw_dir = array[5];
        result->pw_shell = array[6];

        #ifdef DEBUG
        fprintf(stderr, "Found item for name=%s: [uid=%d, gid=%d]\n", name, atoi(array[2]), atoi(array[3]));
        #endif

        return NSS_STATUS_SUCCESS;
      }
    }

    cursor = cursor->next;
  }

  return NSS_STATUS_NOTFOUND;
}

enum nss_status _nss_shib_getpwuid_r(uid_t uid, struct passwd *result, char *buffer, size_t buflen, int *errnop)
{
  #ifdef DEBUG
  fprintf(stderr, "Entering _nss_shib_getpwuid_r with uid=%d.\n", uid);
  #endif

  readconfig();
  char *newurl = (char *) malloc(sizeof(char)*(strlen(url)+8));
  sprintf(newurl, "%s?passwd", url);
  if (!geturl(newurl, username, password, cafile, sslcheck)) return NSS_STATUS_UNAVAIL;
  body_pwd = body; body = NULL;
  if (body_pwd == NULL) return NSS_STATUS_UNAVAIL;

  BODY *cursor = body_pwd;
  while (cursor)
  {
    char **array = split_str(cursor->row, ":");
    if (array != NULL)
    {
      if (atoi(array[2]) == uid)
      {
        result->pw_name = array[0];
        result->pw_passwd = array[1];
        result->pw_uid = atoi(array[2]);
        result->pw_gid = atoi(array[3]);
        result->pw_gecos = array[4];
        result->pw_dir = array[5];
        result->pw_shell = array[6];

        #ifdef DEBUG
        fprintf(stderr, "Found item for uid=d%d: [name=%s, gid=%d]\n", uid, array[0], atoi(array[3]));
        #endif

        return NSS_STATUS_SUCCESS;
      }
    }

    cursor = cursor->next;
  }

  return NSS_STATUS_NOTFOUND;
}

enum nss_status _nss_shib_setpwent(void)
{
  #ifdef DEBUG
  fprintf(stderr, "Entering _nss_shib_setpwent.\n");
  #endif

  syslog(LOG_ERR, "Unable to set passwd entities on Shibboleth.\n");
  return NSS_STATUS_UNAVAIL;
}

enum nss_status _nss_shib_getpwent_r(struct passwd *result, char *buffer, size_t buflen, int *errnop)
{
  #ifdef DEBUG
  fprintf(stderr, "Entering _nss_shib_getpwent_r.\n");
  #endif

  readconfig();
  char *newurl = (char *) malloc(sizeof(char)*(strlen(url)+8));
  sprintf(newurl, "%s?passwd", url);
  if (!geturl(newurl, username, password, cafile, sslcheck)) return NSS_STATUS_UNAVAIL;
  body_pwd = body; body = NULL;
  if (body_pwd == NULL) return NSS_STATUS_UNAVAIL;

  int i = -1;
  BODY *cursor = body_pwd;
  while (cursor)
  {
    if (i == last_rownum_pwd)
    {
      char **array = split_str(cursor->row, ":");
      if (array != NULL)
      {
        result->pw_name = array[0];
        result->pw_passwd = array[1];
        result->pw_uid = atoi(array[2]);
        result->pw_gid = atoi(array[3]);
        result->pw_gecos = array[4];
        result->pw_dir = array[5];
        result->pw_shell = array[6];

        #ifdef DEBUG
        fprintf(stderr, "Found item: [name=%s, uid=%d, gid=%d]\n", array[0], atoi(array[2]), atoi(array[3]));
        #endif

        last_rownum_pwd++;
        return NSS_STATUS_SUCCESS;
      }
    }

    i++;
    cursor = cursor->next;
  }

  return NSS_STATUS_NOTFOUND;
}

enum nss_status _nss_shib_setgrent_r(struct group *result, void *args)
{
  #ifdef DEBUG
  fprintf(stderr, "Entering _nss_shib_setgrent_r.\n");
  #endif

  syslog(LOG_ERR, "Unable to set group entities on Shibboleth.\n");
  return NSS_STATUS_UNAVAIL;
}

enum nss_status _nss_shib_endgrent_r(struct group *result, void* args)
{
  #ifdef DEBUG
  fprintf(stderr, "Entering _nss_shib_endgrent_r.\n");
  #endif

  syslog(LOG_ERR, "Unable to end group entities on Shibboleth.\n");
  return NSS_STATUS_UNAVAIL;
}

enum nss_status _nss_shib_getgrent_r(struct group *result, char *buffer, size_t buflen, int *errnop)
{
  #ifdef DEBUG
  fprintf(stderr, "Entering _nss_shib_getgrent_r.\n");
  #endif

  readconfig();
  char *newurl = (char *) malloc(sizeof(char)*(strlen(url)+7));
  sprintf(newurl, "%s?group", url);
  if (!geturl(newurl, username, password, cafile, sslcheck)) return NSS_STATUS_UNAVAIL;
  body_grp = body; body = NULL;
  if (body_grp == NULL) return NSS_STATUS_UNAVAIL;

  int i = -1;
  BODY *cursor = body_grp;
  while (cursor)
  {
    if (i == last_rownum_grp)
    {
      char **array = split_str(cursor->row, ":");
      if (array != NULL)
      {
        result->gr_name = array[0];
        result->gr_passwd = array[1];
        result->gr_gid = atoi(array[2]);
        result->gr_mem = split_str(array[3], ",");

        #ifdef DEBUG
        fprintf(stderr, "Found item: [grname=%s, gid=%d]\n", array[0], atoi(array[2]));
        #endif

        last_rownum_grp++;
        return NSS_STATUS_SUCCESS;
      }
    }

    i++;
    cursor = cursor->next;
  }  

  return NSS_STATUS_NOTFOUND;
}

enum nss_status _nss_shib_getgrnam_r(const char *name, struct group *result, char *buffer, size_t buflen, int *errnop)
{
  #ifdef DEBUG
  fprintf(stderr, "Entering _nss_shib_getgrnam_r with grname=%s.\n", name);
  #endif

  readconfig();
  char *newurl = (char *) malloc(sizeof(char)*(strlen(url)+7));
  sprintf(newurl, "%s?group", url);
  if (!geturl(newurl, username, password, cafile, sslcheck)) return NSS_STATUS_UNAVAIL;
  body_grp = body; body = NULL;
  if (body_grp == NULL) return NSS_STATUS_UNAVAIL;

  BODY *cursor = body_grp;
  while (cursor)
  {
    char **array = split_str(cursor->row, ":");
    if (array != NULL)
    {
      if (strcmp(array[0], name) == 0)
      {
        result->gr_name = array[0];
        result->gr_passwd = array[1];
        result->gr_gid = atoi(array[2]);
        result->gr_mem = split_str(array[3], ",");

        #ifdef DEBUG
        fprintf(stderr, "Found item for grname=%s: [gid=%d]\n", name, atoi(array[2]));
        #endif

        return NSS_STATUS_SUCCESS;
      }
    }

    cursor = cursor->next;
  }  

  return NSS_STATUS_NOTFOUND;
}

enum nss_status _nss_shib_getgrgid_r(gid_t gid, struct group *result, char *buffer, size_t buflen, int *errnop)
{
  #ifdef DEBUG
  fprintf(stderr, "Entering _nss_shib_getgrnam_r with gid=%d.\n", gid);
  #endif

  readconfig();
  char *newurl = (char *) malloc(sizeof(char)*(strlen(url)+7));
  sprintf(newurl, "%s?group", url);
  if (!geturl(newurl, username, password, cafile, sslcheck)) return NSS_STATUS_UNAVAIL;
  body_grp = body; body = NULL;
  if (body_grp == NULL) return NSS_STATUS_UNAVAIL;

  BODY *cursor = body_grp;
  while (cursor)
  {
    char **array = split_str(cursor->row, ":");
    if (array != NULL)
    {
      if (atoi(array[2]) == gid)
      {
        result->gr_name = array[0];
        result->gr_passwd = array[1];
        result->gr_gid = atoi(array[2]);
        result->gr_mem = split_str(array[3], ",");

        #ifdef DEBUG
        fprintf(stderr, "Found item for gid=%d: [name=%s]\n", gid, array[0]);
        #endif

        return NSS_STATUS_SUCCESS;
      }
    }

    cursor = cursor->next;
  }  

  return NSS_STATUS_NOTFOUND;
}

enum nss_status _nss_shib_getgroupsbymember_r(const char *member, struct group *result, char *buffer, size_t buflen, int *errnop)
{
  #ifdef DEBUG
  fprintf(stderr, "Entering _nss_shib_getgroupsbymember_r with member=%s.\n", member);
  #endif

  readconfig();
  char *newurl = (char *) malloc(sizeof(char)*(strlen(url)+7));
  sprintf(newurl, "%s?group", url);
  if (!geturl(newurl, username, password, cafile, sslcheck)) return NSS_STATUS_UNAVAIL;
  body_grp = body; body = NULL;
  if (body_grp == NULL) return NSS_STATUS_UNAVAIL;

  BODY *cursor = body_grp;
  while (cursor)
  {
    char **array = split_str(cursor->row, ":");
    if (array != NULL)
    {
      if (strcmp(array[0], member) == 0)
      {
        result->gr_name = array[0];
        result->gr_passwd = array[1];
        result->gr_gid = atoi(array[2]);
        result->gr_mem = split_str(array[3], ",");

        #ifdef DEBUG
        fprintf(stderr, "Found item for member=%s: [gid=%d]\n", member, atoi(array[2]));
        #endif

        return NSS_STATUS_SUCCESS;
      }
    }

    cursor = cursor->next;
  }  

  return NSS_STATUS_NOTFOUND;
}

/*
static enum nss_status _nss_shib_passwd_destr(nss_backend_t *pw_context, void *args)
{
  free(be);
  syslog(LOG_DEBUG, "_nss_shib_passwd_destr");
  return NSS_STATUS_SUCCESS;
}

static nss_backend_op_t passwd_ops[] = {
  _nss_shib_passwd_destr,
  _nss_shib_setpwent_r,         // NSS_DBOP_SETENT
  _nss_shib_getpwent_r,         // NSS_DBOP_GETENT
  _nss_shib_getpwnam_r,         // NSS_DBOP_PASSWD_BYNAME
  _nss_shib_getpwuid_r          // NSS_DBOP_PASSWD_BYUID
};

nss_backend_t *_nss_shib_passwd_constr(const char *db_name, const char *src_name, const char *cfg_args)
{
  nss_backend_t *be;

  if(!(be = (nss_backend_t *)malloc(sizeof(nss_backend_t)))) return NULL;
  be->ops = passwd_ops;
  be->n_ops = sizeof(passwd_ops) / sizeof(nss_backend_op_t);

  syslog(LOG_DEBUG, "Initialized nss_shib passwd backend");
  return be;
}
*/

#ifdef DEBUG
int main(int argc, char *argv[])
{
  fprintf(stdout, "Searching uid for user andrea.\n");
  char *uid = getelementfrompasswd(0, "andrea", 2);
  if (uid == NULL) fprintf(stdout, "Unable to find uid in remore passwd file.\n");
  else fprintf(stdout, "Found uid: [andrea] => %s.\n", uid);
  int retval = (uid != NULL);

  return retval;
}
#endif
