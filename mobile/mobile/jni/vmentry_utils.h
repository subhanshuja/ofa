#ifndef VMENTRY_UTILS_H
#define VMENTRY_UTILS_H

bream_exception_t *wrap_string(JNIEnv *env, bream_t *vm, jstring jstr, bream_ref_t ref);
bream_exception_t *wrap_intarray(JNIEnv *env, bream_t *vm, jintArray array, bream_ref_t ref);
bream_exception_t *wrap_bytearray(JNIEnv *env, bream_t *vm, jbyteArray array, bream_ref_t ref);
bream_exception_t *wrap_stringarray(JNIEnv *env, bream_t *vm, jobjectArray array, bream_ref_t ref);
bream_exception_t *wrap_bytearrayarray(JNIEnv *env, bream_t *vm, jobjectArray array, bream_ref_t ref);
bream_exception_t *wrap_object(JNIEnv *env, bream_t *vm, jobject obj, bream_ref_t ref);
bream_exception_t *wrap_font(JNIEnv *env, bream_t *vm, jobject obj, bream_ref_t ref);
/* obj must extend VMInvokes.Wrapper */
struct _MOpObjectWrapper* get_object_wrapper(JNIEnv *env, jobject obj);
/* MOpObjectWrapper release callback, don't call this one. */
void releaseWrappedObject(void* obj);
/* obj must extend Bream.Handle */
bream_handle_t *get_object_handle(JNIEnv *env, jobject obj);

jstring load_string(JNIEnv *env, bream_t *vm, bream_ptr_t ptr);
jintArray load_intarray(JNIEnv *env, bream_t *vm, bream_ptr_t ptr);
jbyteArray load_bytearray(JNIEnv *env, bream_t *vm, bream_ptr_t ptr);
jobjectArray load_stringarray(JNIEnv *env, bream_t *vm, bream_ptr_t ptr);
jobject load_object(JNIEnv *env, bream_t *vm, bream_ptr_t ptr);
jobject load_font(JNIEnv *env, bream_t *vm, bream_ptr_t ptr);

/* o must be a VMEntry object */
bream_t *VMEntry_getVM(JNIEnv *env, jobject o);
void throw_exception(JNIEnv *env, bream_t *vm, bream_exception_t *e);

void throw_java_exception(JNIEnv *env, const char *clazz, const char *message);

/* Check if an exception has occurred, if so,
 * throw a bream exception and return false */
bool check_exception(JNIEnv* env, bream_t *vm);

#endif /* VMENTRY_UTILS_H */
