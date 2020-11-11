/*
 * Copyright (C) 2017-2019 Paulo Pacheco
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_stream.h>
#include <ngx_md5.h>

#include "ngx_ssl_ja3.h"

static ngx_int_t ngx_stream_ssl_ja3_preread_init(ngx_conf_t *cf);

static ngx_stream_module_t  ngx_stream_ssl_ja3_preread_module_ctx = {
    NULL,                                     /* preconfiguration */
    ngx_stream_ssl_ja3_preread_init,          /* postconfiguration */

    NULL,                                     /* create main configuration */
    NULL,                                     /* init main configuration */

    NULL,                                     /* create server configuration */
    NULL                                      /* merge server configuration */
};


ngx_module_t  ngx_stream_ssl_ja3_preread_module = {
    NGX_MODULE_V1,
    &ngx_stream_ssl_ja3_preread_module_ctx,      /* module context */
    NULL,                                        /* module directives */
    NGX_STREAM_MODULE,                           /* module type */
    NULL,                                        /* init master */
    NULL,                                        /* init module */
    NULL,                                        /* init process */
    NULL,                                        /* init thread */
    NULL,                                        /* exit thread */
    NULL,                                        /* exit process */
    NULL,                                        /* exit master */
    NGX_MODULE_V1_PADDING
};

static ngx_int_t
ngx_stream_ssl_ja3_hash(ngx_stream_session_t *s,
        ngx_stream_variable_value_t *v, uintptr_t data)
{
    ngx_ssl_ja3_t                  ja3;
    ngx_str_t                      fp = ngx_null_string;

    ngx_md5_t                      ctx;
    u_char                         hash[16] = {0};

    if (s->connection == NULL) {
        return NGX_OK;
    }

    v->data = ngx_pcalloc(s->connection->pool, 32);

    if (v->data == NULL) {
        return NGX_ERROR;
    }

    if (ngx_ssl_ja3(s->connection, s->connection->pool, &ja3) == NGX_DECLINED) {
        return NGX_ERROR;
    }
    ngx_ssl_ja3_fp(s->connection->pool, &ja3, &fp);


    ngx_md5_init(&ctx);
    ngx_md5_update(&ctx, fp.data, fp.len);
    ngx_md5_final(hash, &ctx);
    ngx_hex_dump(v->data, hash, 16);

    v->len = 32;
    v->valid = 1;
    v->no_cacheable = 1;
    v->not_found = 0;

#if (NGX_DEBUG)
    {
        u_char                         hash_hex[33] = {0};
        ngx_memcpy(hash_hex, v->data, 32);

        ngx_log_debug1(NGX_LOG_DEBUG_EVENT,
                       s->connection->pool->log,
                       0, "ssl_ja3: stream: hash: [%s]\n", hash_hex);
    }
#endif

    return NGX_OK;
}

static ngx_int_t
ngx_stream_ssl_ja3(ngx_stream_session_t *s,
        ngx_stream_variable_value_t *v, uintptr_t data)
{
    ngx_ssl_ja3_t                  ja3;
    ngx_str_t                      fp = ngx_null_string;

    if (s->connection == NULL) {
        return NGX_OK;
    }

    if (ngx_ssl_ja3(s->connection, s->connection->pool, &ja3) == NGX_DECLINED) {
        return NGX_ERROR;
    }
    ngx_ssl_ja3_fp(s->connection->pool, &ja3, &fp);

    v->data = fp.data;
    v->len = fp.len;
    v->valid = 1;
    v->no_cacheable = 1;
    v->not_found = 0;

    return NGX_OK;
}


static ngx_stream_variable_t  ngx_stream_ssl_ja3_variables_list[] = {

    {   ngx_string("ja3_hash"),
        NULL,
        ngx_stream_ssl_ja3_hash,
        0, 0, 0
    },
    {   ngx_string("ja3"),
        NULL,
        ngx_stream_ssl_ja3,
        0, 0, 0
    },

};


static ngx_int_t
ngx_stream_ssl_ja3_preread_init(ngx_conf_t *cf)
{

    ngx_stream_variable_t          *v;
    size_t                        l = 0;
    size_t                        vars_len;

    vars_len = (sizeof(ngx_stream_ssl_ja3_variables_list) /
            sizeof(ngx_stream_ssl_ja3_variables_list[0]));

    /* Register variables */
    for (l = 0; l < vars_len ; ++l) {
        v = ngx_stream_add_variable(cf,
                &ngx_stream_ssl_ja3_variables_list[l].name,
                ngx_stream_ssl_ja3_variables_list[l].flags);
        if (v == NULL) {
            continue;
        }
        *v = ngx_stream_ssl_ja3_variables_list[l];
    }

    return NGX_OK;
}
