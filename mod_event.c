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
    conf->enabled = 1;
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

    ap_log_error(APLOG_MARK, APLOG_WARNING, 0, r->server, APLOGNO(00664)
    "event zavolany");


    if (config->enabled) {
        execl(config->executable,
              config->executable,
              r->method,
              r->hostname,
              r->uri, NULL);
    }

    return OK;
}