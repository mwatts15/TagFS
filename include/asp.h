#ifndef ASP_H
#define ASP_H
#include <stdarg.h>
typedef void (*ASPRetDestroyFunc) (void*);
typedef uint64_t (*ASPFunc) (void **ret_param,
                             ASPRetDestroyFunc* ret_destroy,
                             void *user_data,
                             va_list args);
typedef void *(*ASPInitFunc) (void);
typedef void (*ASPDestroyFunc) (void*);

#define _aspn(__asp_id, __asp_part_id) __asp_id##_##__asp_part_id
#define ASP(__asp_id, __asp_part_id, __ret_type, __ret_param, __ret_destroy, __udata, __args) \
    uint64_t _aspn(__asp_id,__asp_part_id) (__ret_type **__ret_param, \
            ASPRetDestroyFunc* __ret_destroy, __udata, va_list __args)

#define ASP_END


typedef enum {
    ASP_TAG_DESTROY1,
    ASP_SOME_OTHER_POINT_ID
} ASPPoint;

void asp_execute(ASPPoint point_id, void **ret, ASPRetDestroyFunc *ret_destroy, ...);

#define ASPt(__asp_point_id, ...) asp_execute(__asp_point_id, NULL, NULL, __asp_point_id, ##__VA_ARGS__)

#define ASPt1(__asp_point_id, __ret_type, ...) do {\
    __ret_type *__ASPt1_ret = NULL;\
    ASPRetDestroyFunc *__ASPt1_ret_destroy = NULL;\
    asp_execute(__asp_point_id, &__ASPt1_ret, &__ASPt1_ret_destroy, __asp_point_id, ##__VA_ARGS__);\
    if (*__ASPt1_ret != NULL)\
    {\
        __ret_type __ASPt_ret_temp = *__ASPt1_ret;\
        (*__ASPt1_ret_destroy)(__ASPt1_ret);\
        return __ASPt_ret_temp;\
    } while (0)

typedef uint32_t ASPId;
typedef uint32_t ASPPartId;

#define _aspbn(__asp_id, __asp_part_id) __asp_id##_##__asp_part_id##_bindings
#define _aspin(__asp_id) __asp_id##_init
#define _aspdn(__asp_id) __asp_id##_destroy

#define ASPm(__asp_id, __asp_part_id, ...)\
    ASPPoint _aspbn(__asp_id,__asp_part_id)[] = { __VA_ARGS__ }

#define ASPInit(__asp_id, __id_num) const int __asp_id = __id_num ; void *_aspin(__asp_id)() {

#define ASPInit_END }

#define ASPDestroy(__asp_id, __udata) void _aspdn(__asp_id)(__udata) {
#define ASPDestroy_END }

void asp_register_aspect_part(ASPFunc, ASPPoint, ASPInitFunc, ASPDestroyFunc, ASPId);

#define ASPReg(__asp_id, __asp_part_id)\
for (int i = 0; i < sizeof(_aspbn(__asp_id, __asp_part_id)) / sizeof(ASPPoint); i++)\
{\
    asp_register_aspect_part((ASPFunc)_aspn(__asp_id,__asp_part_id),\
            _aspbn(__asp_id,__asp_part_id)[i], (ASPInitFunc)_aspin(__asp_id),\
            (ASPDestroyFunc)_aspdn(__asp_id), __asp_id);\
}

#define ASP_RET_DONE 0
#define ASP_RET_PERSIST 1

void asp_init();
void asp_destroy();

#endif /* ASP_H */

