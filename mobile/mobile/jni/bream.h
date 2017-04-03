#ifndef JNI_BREAM
#define JNI_BREAM

extern struct bream_t *g_bream_vm;
extern struct _MOpVMInstanceExternal g_vm_instance;

void breamStaticInit(JNIEnv *env);

#endif /* JNI_BREAM */
