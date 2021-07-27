#ifndef KERBTICKET_H
#define KERBTICKET_H
#include <QString>
#include <gssapi/gssapi.h>
#include <gssapi/gssapi_ext.h>
#include <krb5.h>

/*Класс получения ключа керберос*/
class KerbTicket
{
public:
    KerbTicket(const QString& serviceName);
    QByteArray getKrbTicket();
private:
    int get_token(const char *service_name,
                          OM_uint32 deleg_flag,
                          gss_OID oid,
                          gss_ctx_id_t *gss_context,
                          OM_uint32 *ret_flags,
                          QByteArray &token);
    QString m_serviceName;
};

#endif // KERBTICKET_H
