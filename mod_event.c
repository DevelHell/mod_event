#include "httpd.h"
#include "http_core.h"
#include "http_protocol.h"
#include "apr_strings.h"
#include "apr_hash.h"
#include <unistd.h>
#include "util_script.h"
#include <stdio.h>
#include <http_log.h>

typedef struct {
    char *executable;
    char **executableArgv;
    int executableArgc;
    char context[256];
    int enabled;
} event_config;

static void register_hooks(apr_pool_t *pool);
static int event_handler(request_rec *r);
void *create_dir_conf(apr_pool_t *pool, char *context);
void *merge_dir_conf(apr_pool_t *pool, void *BASE, void *ADD);
const char *event_set_executable(cmd_parms *cmd, void *cfg, int argc, char *const argv[]);
const char *event_set_enabled(cmd_parms *cmd, void *cfg, int flag);

static const command_rec event_directives[] =
        {
                AP_INIT_FLAG("eventEnabled", event_set_enabled, NULL, ACCESS_CONF, "Enable or disable mod_event"),
//                AP_INIT_TAKE1("eventExecutable", event_set_executable, NULL, ACCESS_CONF,
                AP_INIT_TAKE_ARGV("eventExecutable", event_set_executable, NULL, ACCESS_CONF,
                                  "The path to event executable"),
                {NULL}
        };

module AP_MODULE_DECLARE_DATA
        event_module =
        {
                STANDARD20_MODULE_STUFF,
                create_dir_conf,            // Per-directory configuration handler
                merge_dir_conf,            // Merge handler for per-directory configurations
                NULL,            // Per-server configuration handler
                NULL,            // Merge handler for per-server configurations
                event_directives,            // Any directives we may have for httpd
                register_hooks   // Our hook registering function
        };


const char *event_set_executable(cmd_parms *cmd, void *cfg, int argc, char *const argv[]) {
    event_config *conf = (event_config *) cfg;


    ap_log_error(APLOG_MARK, APLOG_ERR, 0, cmd->server, APLOGNO(00664)
            "eventExecutable requires at least one argument");
    if (argc < 1) {
        ap_log_error(APLOG_MARK, APLOG_ERR, 0, cmd->server, APLOGNO(00664)
                "eventExecutable requires at least one argument");
    }

    if (conf) {
        conf->executable = apr_pstrdup(cmd->pool, argv[0]);
        conf->executableArgc = argc;
        conf->executableArgv = (char **) apr_array_make(cmd->pool, argc, sizeof(char **));
        for (int i = 0; i < argc; i++) {
            conf->executableArgv[i] = apr_pstrdup(cmd->pool, argv[i]);
        }
    }


    return NULL;
}

//const char *event_set_executable(cmd_parms *cmd, void *cfg, const char *arg) {
//    event_config *conf = (event_config *) cfg;
//
//    if (conf) {
//        conf->executable = apr_pstrdup(cmd->pool, arg);
//    }
//
//    return NULL;
//}

const char *event_set_enabled(cmd_parms *cmd, void *cfg, int flag) {
    event_config *conf = (event_config *) cfg;

    if (conf) {
        conf->enabled = flag;
    }

    return NULL;
}

void *create_dir_conf(apr_pool_t *pool, char *context) {
    context = context ? context : "(undefined context)";
    event_config *conf = apr_pcalloc(pool, sizeof(event_config));

    strcpy(conf->context, context);
    conf->enabled = 0;
    conf->executableArgc = 0;
    return conf;
}

void *merge_dir_conf(apr_pool_t *pool, void *BASE, void *ADD) {
    event_config *base = (event_config *) BASE;
    event_config *add = (event_config *) ADD;
    event_config *conf = (event_config *) create_dir_conf(pool, "Merged configuration");

    conf->enabled = (add->enabled == 0) ? base->enabled : add->enabled;
    conf->executableArgc = (add->executableArgc == 0) ? base->executableArgc : add->executableArgc;
    conf->executable = apr_pstrdup(pool, strlen(add->executable) ? add->executable : base->executable);

    int argc = base->executableArgc;
    char **argv = base->executableArgv;
    if (add->executableArgc > 0) {
        argc = add->executableArgc;
        argv = add->executableArgv;
    }

    conf->executableArgv = (char **) apr_array_make(pool, argc, sizeof(char **));
    for (int i = 0; i < argc; i++) {
        conf->executableArgv[i] = apr_pstrdup(pool, argv[i]);
    }

    return conf;
}

static void register_hooks(apr_pool_t *pool) {
    ap_hook_log_transaction(event_handler, NULL, NULL, APR_HOOK_LAST);
}

static int event_handler(request_rec *r) {
    event_config *config = (event_config *) ap_get_module_config(r->per_dir_config, &event_module);

//    ap_log_error(APLOG_MARK, APLOG_WARNING, 0, r->server, APLOGNO(00664)
//    "event zavolany");


    FILE *fp;
    FILE *fp2;

    fp2 = fopen("/tmp/test.txt", "w+");
//    fprintf(fp, "@argc: %d\n", config->executableArgc);
//    fprintf(fp, "@argv0: %s\n", config->executableArgv[0]);
//    fprintf(fp, "@argv1: %s\n", config->executableArgv[1]);
//    fclose(fp);

    fp = popen(config->executable, "r");
//    if (fp == NULL)
//        /* Handle error */;


        char path[PATH_MAX];
    while (fgets(path, PATH_MAX, fp) != NULL) {
        fprintf(fp2, "output: %s\n", path);
    }
    fclose(fp2);

    int status;
    status = pclose(fp);
    if (status == -1) {
        /* Error reported by pclose() */
    } else {
        /* Use macros described under wait() to inspect `status' in order
           to determine success/failure of command executed by popen() */
    }


    if (config->enabled) {
//        execl(config->executable,
//              config->executable,
//              r->method,
//              r->hostname,
//              r->uri, NULL);
//    }
        int res = execv(config->executable,
                      config->executableArgv);
        ap_log_error(APLOG_MARK, APLOG_WARNING, 0, r->server, APLOGNO(00664)
                "event zavolany %d", res);
    }

    return OK;
}