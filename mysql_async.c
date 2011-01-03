#include "ruby.h"
#include "version.h"

#ifdef HAVE_MYSQL_H
#include <mysql.h>
#include <mysql_com.h>
#include <errmsg.h>
#include <mysqld_error.h>
#else
#include <mysql/mysql.h>
#include <mysql/mysql_com.h>
#include <mysql/errmsg.h>
#include <mysql/mysqld_error.h>
#endif

struct mysql {
  MYSQL handler;
  char connection;
  char query_with_result;
};

#define GetMysqlStruct(obj) (Check_Type(obj, T_DATA), (struct mysql*)DATA_PTR(obj))
#define GetHandler(obj)     (Check_Type(obj, T_DATA), &(((struct mysql*)DATA_PTR(obj))->handler))

VALUE cMysql;
VALUE eMysql;

static void mysql_raise(MYSQL* m) {
  VALUE e = rb_exc_new2(eMysql, mysql_error(m));

  rb_iv_set(e, "errno", INT2FIX(mysql_errno(m)));
#if MYSQL_VERSION_ID >= 40101
  rb_iv_set(e, "sqlstate", rb_tainted_str_new2(mysql_sqlstate(m)));
#endif
  rb_exc_raise(e);
}

static VALUE send_query(VALUE obj, VALUE sql) {
  MYSQL* m = GetHandler(obj);
  Check_Type(sql, T_STRING);

  if (GetMysqlStruct(obj)->connection == Qfalse) {
    rb_raise(eMysql, "query: not connected");
  }

  if (mysql_send_query(m, RSTRING_PTR(sql), RSTRING_LEN(sql)) != 0) {
    mysql_raise(m);
  }

  return Qnil;
}
 
static VALUE get_result(VALUE obj) {
  MYSQL* m = GetHandler(obj);

  if (GetMysqlStruct(obj)->connection == Qfalse) {
    rb_raise(eMysql, "query: not connected");
  }

  return (mysql_read_query_result(m) == 0) ? Qtrue : Qfalse;
}
 
void Init_mysql_async() {
  cMysql = rb_const_get(rb_cObject, rb_intern("Mysql"));
  eMysql = rb_const_get(rb_cObject, rb_intern("MysqlError"));

  if (!NIL_P(cMysql) && !NIL_P(eMysql)) {
    rb_define_method(cMysql, "send_query", send_query, 1);
    rb_define_method(cMysql, "get_result", get_result, 0);
  }
}
