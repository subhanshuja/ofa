// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2016 Opera Software AS.  All rights reserved.
//
// This file is an original work developed by Opera Software.

#ifndef CHILL_BROWSER_OPERA_BROWSER_CONTEXT_KEYED_SERVICE_FACTORIES_H_
#define CHILL_BROWSER_OPERA_BROWSER_CONTEXT_KEYED_SERVICE_FACTORIES_H_

namespace opera {

// Ensures the existence of any BrowserContextKeyedServiceFactory. This must be
// done before any preference registry is set in the global
// BrowserContextDependencyManager, as the dependency graph is built from the
// constructors in factory instances.
void EnsureBrowserContextKeyedServiceFactoriesBuilt();

}  // namespace opera

#endif  // CHILL_BROWSER_OPERA_BROWSER_CONTEXT_KEYED_SERVICE_FACTORIES_H_
