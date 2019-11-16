#include "httpd.h"
#include "http_protocol.h"
#include "apr_strings.h"
#include "apr_hash.h"
#include "util_script.h"
#include <stdio.h>
#include <http_log.h>

typedef struct {
    char *executable;
    char context[256];
    int enabled;
} event_config;

static void register_hooks(apr_pool_t *pool);
static int event_handler(request_rec *r);
void *create_dir_conf(apr_pool_t *pool, char *context);
void *merge_dir_conf(apr_pool_t *pool, void *BASE, void *ADD);
const char *event_set_executable(cmd_parms *cmd, void *cfg, const char *arg);
const char *event_set_enabled(cmd_parms *cmd, void *cfg, int flag);

static const command_rec event_directives[] =
        {
                AP_INIT_FLAG("eventEnabled", event_set_enabled, NULL, ACCESS_CONF, "Enable or disable mod_event"),
                AP_INIT_TAKE1("eventExecutable", event_set_executable, NULL, ACCESS_CONF,
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

const char *event_set_executable(cmd_parms *cmd, void *cfg, const char *arg) {
    event_config *conf = (event_config *) cfg;

    if (conf) {
        conf->executable = apr_pstrdup(cmd->pool, arg);
    }

    return NULL;
}

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
    return conf;
}

void *merge_dir_conf(apr_pool_t *pool, void *BASE, void *ADD) {
    event_config *base = (event_config *) BASE;
    event_config *add = (event_config *) ADD;
    event_config *conf = (event_config *) create_dir_conf(pool, "Merged configuration");

    conf->enabled = (add->enabled == 0) ? base->enabled : add->enabled;
    conf->executable = apr_pstrdup(pool, strlen(add->executable) ? add->executable : base->executable);

    return conf;
}

static void register_hooks(apr_pool_t *pool) {
    ap_hook_log_transaction(event_handler, NULL, NULL, APR_HOOK_LAST);
}

static int event_handler(request_rec *r) {
    event_config *config = (event_config *) ap_get_module_config(r->per_dir_config, &event_module);

    if (config->enabled) {
        // executable + args + 3 spaces between them + trailing NULL
        int size = strlen(config->executable) + strlen(r->method) + strlen(r->hostname) + strlen(r->unparsed_uri) + 3;
        char *path = apr_palloc(r->pool, size);
        snprintf(path, size, "%s %s %s %s", config->executable, r->method, r->hostname, r->unparsed_uri);

        FILE *fp;
        fp = popen(path, "r");
        if (fp == NULL) {
            ap_log_error(APLOG_MARK, APLOG_WARNING, 0, r->server, APLOGNO(00100)
                         "unable to open %s", (const char*) config->executable);
        }
        if (errno) {
            char path[PATH_MAX];
            // if error occurred, first row of the output is usually enough
            if (fgets(path, PATH_MAX, fp) != NULL) {
                char *errstr = strerror(errno);
                ap_log_error(APLOG_MARK, APLOG_WARNING, 0, r->server, APLOGNO(00200)
                             "error while executing %s: %s, output: %s", config->executable, errstr, path);
            }
        }

        int status;
        status = pclose(fp);
        if (status == -1) {
            ap_log_error(APLOG_MARK, APLOG_WARNING, 0, r->server, APLOGNO(00101)
                         "unable to close %s", config->executable);
        }
    }

    return OK;
}