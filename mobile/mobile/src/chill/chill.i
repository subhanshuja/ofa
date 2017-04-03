// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2014 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

%include "app/native_interface.i"
%include "app/tester_mode.i"

%include "browser/gurl.i"
%include "browser/time.i"
%include "browser/password_manager/passwordform.i"

%include "common/types.i"
%include "browser/download/download_item.i"
%include "browser/download/download_manager.i"

%include "browser/bookmarks.i"
%include "../common/favorites/favorites.i"
%include "browser/old_bookmarks.i"

%include "browser/favorites/speed_dial_data_fetcher.i"
%include "browser/favorites/thumbnail_request_interceptor.i"

%include "browser/gpu_info.i"
%include "browser/gpu_data_manager.i"

%include "browser/analytics/url_visit_listener.i"
%include "browser/authentication_dialog.i"
%include "browser/builtin_suggestion_provider.i"
%include "browser/certificates/certificate_utils.i"
%include "browser/context_menu_helper.i"
%include "browser/chromium_tab_delegate.i"
%include "browser/download/download_helper.i"
%include "browser/download/download_item_observer.i"
%include "browser/download/download_manager_observer.i"
%include "browser/download/download_target_arguments.i"
%include "browser/download/opera_download_manager_delegate.i"
%include "browser/find_in_page.i"
%include "browser/history/history.i"
%include "browser/javascript_dialog_manager_delegate.i"
%include "browser/migration/obml_migration_callback_arguments.i"
%include "browser/migration/obml_migration_runner.i"
%include "browser/navigation_history.i"
%include "browser/notifications/notifications.i"
%include "browser/password_manager/password_manager_delegate.i"
%include "browser/permissions/permissions.i"
%include "browser/persistent_tab_helper.i"
%include "browser/popup_blocking_helper.i"
%include "browser/opera_resource_dispatcher_host_delegate.i"
%include "browser/save_page_helper.i"
%include "browser/chromium_tab.i"
%include "browser/net/turbo_constants.i"
%include "browser/net/tld_validity.i"
%include "browser/net/turbo_delegate.i"
%include "browser/ui/compositor/compositor_ui.i"
%include "browser/ui/tabs/bitmap_sink.i"
%include "browser/ui/tabs/tab_selector_renderer.i"
%include "browser/ui/tabs/thumbnail_cache.i"
%include "browser/ui/tabs/thumbnail_feeder.i"
%include "browser/ui/tabs/thumbnail_store.i"

%include "browser/registry_controlled_domain.i"

%include "browser/opera_browser_context.i"
%include "common/cpu_util.i"
%include "common/logcat_util.i"
%include "common/opera_content_client.i"

%include "browser/suggestion_manager_factory.i"
%include "browser/system_properties.i"

%include "browser/webapps/add_to_homescreen_data_fetcher.i"

%include "browser/banners/app_banner_manager.i"

%include "browser/web_context_menu_data.i"

%include "net/filename_util.i"
