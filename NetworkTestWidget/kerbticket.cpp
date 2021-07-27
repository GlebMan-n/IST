#include "kerbticket.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <poll.h>
#include <netdb.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <krb5.h>
#include <pwd.h>
#include <algorithm>

#include <QDebug>

int KerbTicket::get_token(const char *service_name, OM_uint32 deleg_flag, gss_OID oid, gss_ctx_id_t *gss_context, OM_uint32 *ret_flags, QByteArray &token)
{
    gss_buffer_desc send_tok, recv_tok, *token_ptr;
    gss_name_t target_name;
    OM_uint32 maj_stat, min_stat, init_sec_min_stat;
    send_tok.value = (void *)service_name;
    send_tok.length = strlen(service_name) + 1;
    maj_stat = gss_import_name(&min_stat, &send_tok, (gss_OID) GSS_C_NT_HOSTBASED_SERVICE, &target_name);
    if (maj_stat != GSS_S_COMPLETE) {
        qDebug() << "maj_stat != GSS_S_COMPLETE";
        return -1;
    }
    token_ptr = GSS_C_NO_BUFFER;
    *gss_context = GSS_C_NO_CONTEXT;
    do {
        maj_stat = gss_init_sec_context(&init_sec_min_stat,
                        GSS_C_NO_CREDENTIAL,
                        gss_context,
                        target_name,
                        oid,
                        GSS_C_MUTUAL_FLAG | GSS_C_REPLAY_FLAG | deleg_flag,
                        0,
                        NULL,
                        token_ptr,
                        NULL,
                        &send_tok,
                        ret_flags,
                        NULL);

        if (token_ptr != GSS_C_NO_BUFFER)
            gss_release_buffer(&min_stat, &recv_tok);

        if (send_tok.length != 0) {
            token = QByteArray((char*)send_tok.value, send_tok.length);
        }
        if (maj_stat!=GSS_S_COMPLETE && maj_stat!=GSS_S_CONTINUE_NEEDED) {
            qDebug() << "maj_stat!=GSS_S_COMPLETE && maj_stat!=GSS_S_CONTINUE_NEEDED";
            gss_release_name(&min_stat, &target_name);
            if (*gss_context == GSS_C_NO_CONTEXT)
                gss_delete_sec_context(&min_stat, gss_context, GSS_C_NO_BUFFER);
            return -1;
        }
        //qDebug() << "Ok: sending init_sec_context token:" << m_token.toBase64();
        return 1;
    } while (maj_stat == GSS_S_CONTINUE_NEEDED);
    gss_release_name(&min_stat, &target_name);
    return -1;
}

KerbTicket::KerbTicket(const QString &serviceName)
{
    m_serviceName = serviceName;
}

QByteArray KerbTicket::getKrbTicket()
{
    OM_uint32 deleg_flag = 0;
    gss_OID oid = GSS_C_NULL_OID;
    gss_ctx_id_t context;
    OM_uint32 ret_flags;
    const char *service_name = m_serviceName.toUtf8().data();
    QByteArray token;
    if (get_token(service_name, deleg_flag, oid, &context, &ret_flags, token) == 1)
    {
        qDebug() << "Init token is OK";
        return token;
    }
    return token;
}
