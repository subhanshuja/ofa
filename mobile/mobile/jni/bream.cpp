/* -*- Mode: c++; indent-tabs-mode: nil; c-basic-offset: 4; -*-
 *
 * Copyright (C) 2012 Software ASA.  All rights reserved.
 *
 * This file is part of the Opera web browser.  It may not be distributed
 * under any circumstances.
 */

#include "pch.h"

#include <dlfcn.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <jni.h>
#include "jni_obfuscation.h"
#include "jniutils.h"

#include "bream.h"
#include "favorites.h"
#include "font.h"
#include "obmlview.h"
#include "platform.h"
#include "pushedcontent.h"
#include "socket.h"
#include "resolve.h"
#include "vminstance_ext.h"
#include "VMCxxInvokes.h"

#include "com_opera_android_bream_Bream.h"
#include "com_opera_android_bream_Bream_Handle.h"

#include "bream/vm/c/inc/bream_host.h"
#include "bream/vm/c/inc/bream_const.h"
#include "bream/vm/c/graphics/internal_io.h"
#include "bream/vm/c/graphics/thread.h"

#include "common/favorites/favorites_delegate.h"

#include "mobile/src/chill/browser/favorites/thumbnail_request_interceptor_helper.h"
#include "mobile/src/chill/browser/net/turbo_zero_rating_delegate.h"

extern "C" {
#include "mini/c/shared/core/components/DataStore.h"
#include "mini/c/shared/core/components/TurboRouter.h"
#include "mini/c/shared/core/hardcore/GlobalMemory.h"
#include "mini/c/shared/core/pi/SystemInfo.h"
#include "mini/c/shared/core/protocols/obsp/ObspConnection.h"
#include "mini/c/shared/core/protocols/obsp/ObspProperties.h"
#include "mini/c/shared/core/settings/SettingsManager.h"
#include "mini/c/shared/core/softcore/softcore.h"
#include "mini/c/shared/core/softcore/Timer.h"
#include "mini/c/shared/generic/api_impl/GenMessages.h"
#include "mini/c/shared/generic/vm_integ/ObjectWrapper.h"
#include "mini/c/shared/generic/vm_integ/ThumbnailData.h"
#include "mini/c/shared/generic/vm_integ/TilesContainer.h"
#include "mini/c/shared/generic/vm_integ/VMEntries.h"
#include "mini/c/shared/generic/vm_integ/VMHelpers.h"

#ifdef MOP_FEATURE_KEYCHAIN
#include "mini/c/shared/core/pi/Keychain.h"
#endif /* MOP_FEATURE_KEYCHAIN */
}

#ifdef BREAM_ENTRY_THREAD_CHECK
#include <pthread.h>
static pthread_t bream_entry_thread_check;

BOOL breamEntryThreadCheck()
{
    return pthread_equal(bream_entry_thread_check, pthread_self());
}
#endif

bream_t *g_bream_vm;
MOpVMInstanceExternal g_vm_instance;

extern "C" BOOL handle_invoke(MOpVMInstance *inst, bream_t *vm, int function);

static void init_callbacks(bream_callback_t* callbacks, void* data);

static void MOpVMInstanceExternal_timerExpired(MOpTimer* _this,
                                               MOpVMInstanceExternal* inst);

#ifdef MOP_FEATURE_LOCK_VM
static void MOpVMInstanceExternal_lockVM(MOpVMInstanceExternal* _this);
static void MOpVMInstanceExternal_unlockVM(MOpVMInstanceExternal* _this);
#error "Lock VM? Check if we really need this..."
#endif /* MOP_FEATURE_LOCK_VM */

static BOOL MOpVMInstanceExternal_isCallable(MOpVMInstanceExternal* _this UNUSED)
{
    /* This object is only alive as long as we have a running VM. */
    return TRUE;
}

static void MOpVMInstanceExternal_afterEntry(MOpVMInstanceExternal *_this UNUSED)
{
    // No afterEntry in the nativeui code
    //MOpVMEntry_afterEntry(&_this->base);
}

static void MOpVMInstanceExternal_delayEntry(MOpVMInstanceExternal *_this UNUSED, int entry, void* args)
{
    MOP_ASSERT(args);
    MOpMessageLoop_postMessage(MOpMessageLoop_get(), MOP_MSG_GENERIC_VM_ENTRY,
                               entry, args);
}

static MOpDataStore* MOpVMInstanceExternal_getDataStore(MOpVMInstanceExternal* _this)
{
    return _this->ds;
}

static bream_t *MOpVMInstanceExternal_getVM(MOpVMInstanceExternal *)
{
    return g_bream_vm;
}

static MOP_STATUS MOpVMInstanceExternal_init(MOpVMInstanceExternal* _this,
                                             const char* dspath,
                                             const char* gogi_saved_page_path)
{
    struct _MOpKeychain* keychain = NULL;
#ifdef MOP_FEATURE_KEYCHAIN
    keychain = MOpKeychain_getInstance();
#endif /* MOP_FEATURE_KEYCHAIN */

    MOP_RETURNIF(MOpVMInstance_init(&_this->base));

    GlobalMemory_get()->mopApplication = _this;

    MOP_STATUS status;
    status = MOpDataStore_create(&_this->ds, dspath, NULL, keychain);
    if (status == MOP_STATUS_OK) {
        MOpDataStore_setSecureType(_this->ds, ENUM_BREAM_IO_API_NATIVEDATASTORE_TYPE_WAND);
        MOpDataStore_setSecureType(_this->ds, ENUM_BREAM_IO_API_NATIVEDATASTORE_TYPE_LINK);
    } else {
        MOpDebug("Error creating data store: %d", status);
    }

    _this->base.vtbl.getVM = (MOpVMInstance_GetVM*) MOpVMInstanceExternal_getVM;
#ifdef MOP_FEATURE_LOCK_VM
    _this->base.vtbl.lockVM = (MOpVMInstance_LockVM*) MOpVMInstanceExternal_lockVM;
    _this->base.vtbl.unlockVM = (MOpVMInstance_UnlockVM*) MOpVMInstanceExternal_unlockVM;
#endif /* MOP_FEATURE_LOCK_VM */
    _this->base.vtbl.isCallable = (MOpVMInstance_IsCallable*) MOpVMInstanceExternal_isCallable;
    _this->base.vtbl.afterEntry = (MOpVMInstance_AfterEntry *) MOpVMInstanceExternal_afterEntry;
    _this->base.vtbl.delayEntry = (MOpVMInstance_DelayEntry *) MOpVMInstanceExternal_delayEntry;
    _this->base.vtbl.getDataStore = (MOpVMInstance_GetDataStore *) MOpVMInstanceExternal_getDataStore;

    // TODO Set this to something?
    //_this->base.gContext = NULL;

    MOP_RETURNIF(MOpTimer_create(&_this->timer));
    MOpTimer_setListener(_this->timer, (TimeOutPtr) MOpVMInstanceExternal_timerExpired, _this);

    _this->gogi_saved_page_path = mop_strdup(gogi_saved_page_path);
    MOP_RETURNIF_NOMEM(_this->gogi_saved_page_path);

    MOP_RETURNIF(AndroidFont::CreateCollection(&_this->base.fonts));

    return MOP_STATUS_OK;
}

static void MOpVMInstanceExternal_release(MOpVMInstanceExternal* _this)
{
    bream_t* tmp = g_bream_vm;
    mop_free(_this->gogi_saved_page_path);
    MOpTimer_release(_this->timer);
    MOpDataStore_release(_this->ds);
    /* Hack to make sure g_bream_vm is valid during bream_delete */
    bream_delete(&tmp);
    g_bream_vm = tmp;
    MOpVMInstance_destruct(&_this->base);
}

void MOpVMInstanceExternal_setTimer(MOpVMInstanceExternal* _this, UINT32 target)
{
    UINT32 current = MOpSystemInfo_getTickCount();
    target = (target > current) ? target - current : 0;
    MOpTimer_start(_this->timer, target);
}

void MOpVMInstanceExternal_timerExpired(MOpTimer* _this UNUSED,
                                        MOpVMInstanceExternal* inst)
{
    MOpVMEntry_bream_util_timerExpired((MOpVMInstance*) inst);
}


static jclass breamClass;
static jfieldID id_invokes;
static jfieldID id_handle_ptr;

static void setUseObspConnectionFeature(ObspConnectionRequestFeatureSet featureSet,
                                        UINT32 features,
                                        BOOL use)
{
    ObspConnection *conn = ObspConnection_getInstance();
    if (conn)
    {
        ObspConnection_useFeatures(conn, featureSet, features, use);
        ObspConnection_release(conn);
    }
}

static void HandleMessage(UINT32 msg, UINT32 param1, void* param2)
{
    switch (msg)
    {
    case MOP_MSG_GENERIC_VM_ENTRY:
        MOpVMEntryDispatch(&g_vm_instance.base, param1, param2);
        return;
    }

    MOpDebug("unhandled message: %lu", msg);
}

JNIEXPORT jlong JNICALL Java_com_opera_android_bream_Bream_nativeInit(JNIEnv *env, jobject o, jstring dspath, jstring gogi_saved_page_path)
{
    Core_registerGenericMessageHandler(HandleMessage);

    g_vm_instance.bream = env->NewGlobalRef(o);

    init_callbacks(&g_vm_instance.callback, &g_vm_instance);

    if (bream_new(&g_bream_vm, &g_vm_instance.callback) != BREAM_OK)
    {
        // TODO Throw something?
        return 0;
    }

    bream_enable_debug(g_bream_vm, BREAM_DEBUG_VM, TRUE);
    bream_enable_debug(g_bream_vm, BREAM_DEBUG_VM_ERROR, TRUE);
    bream_enable_debug(g_bream_vm, BREAM_DEBUG_VM_MEM, TRUE);
    bream_enable_debug(g_bream_vm, BREAM_DEBUG_VM_STACKTRACE, TRUE);
    bream_enable_debug(g_bream_vm, BREAM_DEBUG_VM_INIT, TRUE);
    bream_enable_debug(g_bream_vm, BREAM_DEBUG_VM_DEBUGGER, TRUE);
    bream_enable_debug(g_bream_vm, BREAM_DEBUG_VM_DEBUGINFO, TRUE);

    jnistring dspath_str = GetStringUTF8Chars(env, dspath);
    jnistring gogi_saved_page_path_str = GetStringUTF8Chars(env, gogi_saved_page_path);
    MOP_STATUS stat = MOpVMInstanceExternal_init(
        &g_vm_instance, dspath_str.string(), gogi_saved_page_path_str.string());
    if (stat != MOP_STATUS_OK)
    {
        MOpVMInstanceExternal_release(&g_vm_instance);
        return 0;
    }
    return reinterpret_cast<jlong>(&g_vm_instance);
}

static void noopUpdateContext(void* userdata UNUSED,
                              BOOL repaint UNUSED,
                              BOOL forcedRenderDone UNUSED)
{
}

static void noopInvalidateArea(void* userdata UNUSED,
                               struct MOpObmlDocument* context UNUSED,
                               int x UNUSED, int y UNUSED,
                               int w UNUSED, int h UNUSED)
{
}

JNIEXPORT void JNICALL Java_com_opera_android_bream_Bream_nativeRunUnittest(JNIEnv *env, jclass, jbyteArray program, jbyteArray debuginfo NDEBUG_UNUSED)
{
    bream_t *oldvm, *testvm;
    bgContext* context;

    scoped_localref<jobject> invokes(
        env, env->GetObjectField(g_vm_instance.bream, id_invokes));
    g_vm_instance.invokes = env->NewGlobalRef(*invokes);

    oldvm = g_bream_vm;
    bream_new(&testvm, &g_vm_instance.callback);
    g_bream_vm = testvm;

    context = (bgContext *)swCreateEmptyContext(&g_bg_platform, BG_ARGB_8888);
    context->width = context->cw = 100;
    context->height = context->ch = 100;

    GlobalMemory* gm = GlobalMemory_get();
    MOpTilesContainer* container;
    MOpTilesContainer_create(&container, context, NULL, noopUpdateContext, noopInvalidateArea);
    MOpTilesContainer_setVMInstance(container, &g_vm_instance.base);
    gm->tilesContainer = container;

    do
    {
        size_t prog_length = env->GetArrayLength(program);
        void* prog_bytes = bream_malloc(prog_length);
        env->GetByteArrayRegion(program, 0, (jsize)prog_length, (jbyte*)prog_bytes);
        bream_error_t res = bream_load_program(testvm, prog_bytes, prog_length);
        bream_free(prog_bytes);
        if (res != BREAM_OK)
        {
            MOpDebug("Loading unittests failed");
            break;
        }

#if BREAM_DEBUGINFO_SUPPORT
        if (debuginfo != NULL)
        {
            size_t debuginfo_length = env->GetArrayLength(debuginfo);
            void* debuginfo_bytes = bream_malloc(debuginfo_length);
            env->GetByteArrayRegion(debuginfo, 0, (jsize)debuginfo_length, (jbyte*)debuginfo_bytes);
            bream_load_debuginfo(testvm, debuginfo_bytes, debuginfo_length);
            bream_free(debuginfo_bytes);
        }
#endif // BREAM_DEBUGINFO_SUPPORT

        MOpDebug("Starting unittests...");
        bream_start_program(testvm);

        bream_invoke_t* invoke;
        bream_create_invoke_entry(testvm, ENTRY_MOBILE_API_MAIN, NULL, NULL, NULL, NULL, &invoke);
        bream_invoke(testvm, invoke);

        while (!g_vm_instance.exited)
        {
            int delay;
            Socket_slice();
            Resolve_slice();
            delay = Core_slice();
            if (delay > 0)
            {
                usleep(500000);
            }
        }
    } while (false);

    MOpDebug("Finished unittests...");

    /* Clear bream state for loading of the real program */
    MOpTimer_stop(g_vm_instance.timer);

    context->dispose(context);

    bream_delete(&testvm);
    g_bream_vm = oldvm;
    g_vm_instance.exited = FALSE;

    MOpTilesContainer_dispose(container);
    gm->tilesContainer = NULL;

    env->DeleteGlobalRef(g_vm_instance.invokes);
    g_vm_instance.invokes = NULL;
}

JNIEXPORT void JNICALL Java_com_opera_android_bream_Bream_nativeLoadProgram(
    JNIEnv *env, jclass, jbyteArray program)
{
    scoped_localref<jobject> invokes(
        env, env->GetObjectField(g_vm_instance.bream, id_invokes));
    g_vm_instance.invokes = env->NewGlobalRef(*invokes);

    bream_error_t res = BREAM_ERR;
    if (jbyte* prog_bytes = env->GetByteArrayElements(program, NULL))
    {
        size_t prog_length = env->GetArrayLength(program);
        res = bream_load_program(g_bream_vm, prog_bytes, prog_length);
        env->ReleaseByteArrayElements(program, prog_bytes, JNI_ABORT);
    }
    if (res != BREAM_OK)
    {
        // TODO Throw something
    }
    MOpDebug("Program loaded.");
}

#if BREAM_DEBUGINFO_SUPPORT
JNIEXPORT void JNICALL Java_com_opera_android_bream_Bream_nativeLoadDebugInfo(
    JNIEnv* env, jclass, jbyteArray debuginfo)
{
    MOpDebug("Loading debug info...");
    size_t debuginfo_length = env->GetArrayLength(debuginfo);
    void* debuginfo_bytes = bream_malloc(debuginfo_length);
    env->GetByteArrayRegion(debuginfo, 0, (jsize)debuginfo_length, (jbyte*)debuginfo_bytes);
    bream_load_debuginfo(g_bream_vm, debuginfo_bytes, debuginfo_length);
    bream_free(debuginfo_bytes);
}
#else
JNIEXPORT void JNICALL Java_com_opera_android_bream_Bream_nativeLoadDebugInfo(
    JNIEnv*, jclass, jbyteArray debuginfo UNUSED)
{
}
#endif // BREAM_DEBUGINFO_SUPPORT

JNIEXPORT void JNICALL Java_com_opera_android_bream_Bream_nativeStart(
    JNIEnv*, jclass)
{
#ifdef BREAM_ENTRY_THREAD_CHECK
    bream_entry_thread_check = pthread_self();
#endif

    MOpDebug("Starting program...");
    bream_start_program(g_bream_vm);

    bream_invoke_t* invoke;
    bream_create_invoke_entry(g_bream_vm, ENTRY_MOBILE_API_MAIN, NULL, NULL, NULL, NULL, &invoke);
    bream_invoke(g_bream_vm, invoke);

    MOpVMInstance_bootComplete(&g_vm_instance.base);
    onBreamInitialized();
}

JNIEXPORT void JNICALL Java_com_opera_android_bream_Bream_setOBMLSetting(
    JNIEnv *env, jclass, jstring jkey, jint value)
{
    jnistring key = GetStringUTF8Chars(env, jkey);
    MOpVMInstance_generalSettingsHandler(&g_vm_instance.base, (char*)key.string(), value);
}

JNIEXPORT void JNICALL Java_com_opera_android_bream_Bream_setSyncPollingEnabled(JNIEnv *env UNUSED,
                                                                                jclass clazz UNUSED,
                                                                                jboolean enabled)
{
    setUseObspConnectionFeature(OBSPCONNECTION_EXTENDED_FEATURES_2,
            OBSP_REQUEST_FEATURE_EXT2_CLIENT_SYNC_POLLING, enabled == JNI_TRUE ? TRUE : FALSE);
}

#ifdef MOP_TWEAK_ENABLE_TURBO2_ROUTER
class BreamTurboZeroRatingDelegate : public turbo::zero_rating::Delegate
{
  public:
    int MatchUrl(const char* url)
    {
        MOpTurboRouter* router = MOpTurboRouter_get();

        UINT16 slot = 0;
        MOpTurboRouter_slotNumber(router, &slot, url, -1, "", -1);

        return slot;
    }
};
#endif

extern bool VMInvokes_dispatch(JNIEnv *env, jobject o, bream_t *vm, int function);
extern void VMInvokes_init(JNIEnv *env, jclass clazz);

static BOOL cb_invoke(bream_callback_t *, int function)
{
    if (!bream::dispatch(g_bream_vm, function) &&
        !handle_invoke(&g_vm_instance.base, g_bream_vm, function) &&
        !VMInvokes_dispatch(getEnv(), g_vm_instance.invokes, g_bream_vm,
                            function))
    {
        return FALSE;
    }
    return TRUE;
}

static void cb_program_terminated(bream_callback_t *cb UNUSED, int error UNUSED)
{
    // TODO Should tell the UI we got hosed. Or remove the callback...
}

static void cb_init_properties(bream_callback_t *cb UNUSED)
{
    bream_ptr_t ptr = 0;
    bream_exception_t *exception = bream_wrap_string_utf8(
        g_bream_vm, g_vm_instance.gogi_saved_page_path, &ptr);
    if (!exception)
        bream_put_static_field_ptr(
            g_bream_vm, PROPERTY_BREAM_UTIL_NATIVESYSTEM_GOGISAVEDPAGEPATH, ptr);

    bream_put_static_field_int(g_bream_vm,
                               PROPERTY_BREAM_UTIL_NATIVESYSTEM_SCREENPPI,
                               MIN(g_screen_dpi_x, g_screen_dpi_y));
}

static void cb_on_object_moved(bream_callback_t *, bream_ptr_t current_address, bream_ptr_t new_address)
{
    MOpObjectWrapper_moved(g_bream_vm, current_address, new_address);
}

static void cb_on_object_destroyed(bream_callback_t *, bream_ptr_t address)
{
    MOpObjectWrapper_destroyed(g_bream_vm, address);
}

static void cb_log_message(bream_callback_t *cb UNUSED, bream_string_t scope NDEBUG_UNUSED, bream_string_t message NDEBUG_UNUSED)
{
    MOpDebug("%s: %s", scope.data, message.data);
}

static void cb_log_scope(bream_callback_t *cb UNUSED, bream_debug_scope_t scope NDEBUG_UNUSED, const char *msg NDEBUG_UNUSED)
{
    MOpDebug("%s: %s", VMHelpers_tdebugScope(scope), msg);
}

void init_callbacks(bream_callback_t* cb, void* userdata)
{
    cb->context = userdata;

    cb->invoke = cb_invoke;
    cb->program_terminated = cb_program_terminated;
    cb->init_properties = cb_init_properties;
    cb->on_object_moved = cb_on_object_moved;
    cb->on_object_destroyed = cb_on_object_destroyed;
    cb->log_message = cb_log_message;
    cb->log_scope = cb_log_scope;
#if BREAM_DEBUGGER_SUPPORT
#error "Missing callbacks for debugger support"
#endif /* BREAM_DEBUGGER_SUPPORT */
}

void breamStaticInit(JNIEnv *env)
{
    {
        scoped_localref<jclass> clazz(env, env->FindClass(C_com_opera_android_bream_Bream));
        breamClass = (jclass)env->NewGlobalRef(*clazz);
        MOP_ASSERT(breamClass);
        id_invokes = env->GetFieldID(*clazz, F_com_opera_android_bream_Bream_invokes_ID);
    }

    scoped_localref<jclass> handleClazz(
        env, env->FindClass(C_com_opera_android_bream_Bream$Handle));
    id_handle_ptr = env->GetFieldID(*handleClazz, F_com_opera_android_bream_Bream$Handle_mPtr_ID);

    scoped_localref<jclass> invokesClazz(
        env, env->FindClass(C_com_opera_android_bream_VMInvokes));
    VMInvokes_init(env, *invokesClazz);
}

#if 0
void Java_com_opera_android_browser_obml_OBMLView_nativeStaticShutdown(JNIEnv *env, jclass clazz)
{
    Core_shutdown();
}
#endif

void Java_com_opera_android_bream_Bream_onPause(JNIEnv *env UNUSED,
                                                jclass clazz UNUSED)
{
    MOpSettingsManager_updateDownloadData(MOpSettingsManager_getSingleton());
    MOpSettingsManager_saveToFile(MOpSettingsManager_getSingleton(), MOP_SETTINGS_FILE);
}

void Java_com_opera_android_bream_Bream_onResume(JNIEnv *env UNUSED,
                                                 jclass clazz UNUSED)
{
#ifdef MOP_FEATURE_NETWORK_TESTS
    ObspConnection *connection = ObspConnection_getInstance();
    if (connection)
    {
        ObspConnection_prepareNetworkTests(connection, FALSE);
        ObspConnection_release(connection);
    }
#endif // MOP_FEATURE_NETWORK_TESTS
}

void Java_com_opera_android_bream_Bream_setupDelegates(JNIEnv *env,
                                                       jclass,
                                                       jstring libpath)
{
    jnistring path = GetStringUTF8Chars(env, libpath);
    void *handle = dlopen(path.string(), RTLD_LAZY | RTLD_LOCAL);
    if (handle)
    {
        ThumbnailRequestInterceptor_SetFavoritesDelegateSignature setFavoritesDelegate;
        setFavoritesDelegate = (ThumbnailRequestInterceptor_SetFavoritesDelegateSignature)dlsym(handle, "ThumbnailRequestInterceptor_SetFavoritesDelegate");
        if (setFavoritesDelegate)
        {
            mobile::FavoritesDelegate *delegate =
                mobile::setupFavoritesDelegate(mobile::partnerContentHandler());
            setFavoritesDelegate(delegate);
        }

#ifdef MOP_TWEAK_ENABLE_TURBO2_ROUTER
        SetTurboZeroRatingDelegateSignature setTurboZeroRatingDelegate;
        setTurboZeroRatingDelegate = (SetTurboZeroRatingDelegateSignature)dlsym(
            handle, "SetTurboZeroRatingDelegate");
        if (setTurboZeroRatingDelegate)
        {
            turbo::zero_rating::Delegate* delegate = new BreamTurboZeroRatingDelegate();
            setTurboZeroRatingDelegate(delegate);
        }
#endif
        dlclose(handle);
    }
}

void Java_com_opera_android_bream_Bream_00024Handle_nativeInit(JNIEnv *env, jobject o, jlong handle)
{
    bream_handle_t *h = new bream_handle_t;
    bream_init_handle(h);
    h->ptr = handle;
    bream_register_handle(g_bream_vm, h);
    env->SetLongField(o, id_handle_ptr, (jlong)h);
}

jint Java_com_opera_android_bream_Bream_00024Handle_get(JNIEnv *env, jobject o)
{
    bream_handle_t *h = (bream_handle_t*)env->GetLongField(o, id_handle_ptr);
    return h ? h->ptr : 0;
}

void Java_com_opera_android_bream_Bream_00024Handle_nativeFinish(JNIEnv *env, jobject o)
{
    bream_handle_t *h = (bream_handle_t*)env->GetLongField(o, id_handle_ptr);
    if (h) {
        bream_unregister_handle(g_bream_vm, h);
        delete h;
        env->SetLongField(o, id_handle_ptr, 0);
    }
}
